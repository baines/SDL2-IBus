/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2014 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#ifdef HAVE_IBUS_IBUS_H
#include "SDL.h"
#include "SDL_ibus.h"
#include "SDL_dbus.h"
#include "../../video/SDL_sysvideo.h"
#include "../../events/SDL_keyboard_c.h"

static const char IBUS_SERVICE[]         = "org.freedesktop.IBus";
static const char IBUS_PATH[]            = "/org/freedesktop/IBus";
static const char IBUS_INTERFACE[]       = "org.freedesktop.IBus";
static const char IBUS_INPUT_INTERFACE[] = "org.freedesktop.IBus.InputContext";

static char *input_ctx_path = NULL;
static SDL_Rect ibus_cursor_rect = {0};
static DBusConnection *ibus_conn = NULL;
static char *ibus_addr_file = NULL;

static Uint32
IBus_ModState(void)
{
    Uint32 ibus_mods = 0;
    SDL_Keymod sdl_mods = SDL_GetModState();
    
    /* Not sure about MOD3, MOD4 and HYPER mappings */
    if(sdl_mods & KMOD_LSHIFT) ibus_mods |= IBUS_SHIFT_MASK;
    if(sdl_mods & KMOD_CAPS)   ibus_mods |= IBUS_LOCK_MASK;
    if(sdl_mods & KMOD_LCTRL)  ibus_mods |= IBUS_CONTROL_MASK;
    if(sdl_mods & KMOD_LALT)   ibus_mods |= IBUS_MOD1_MASK;
    if(sdl_mods & KMOD_NUM)    ibus_mods |= IBUS_MOD2_MASK;
    if(sdl_mods & KMOD_MODE)   ibus_mods |= IBUS_MOD5_MASK;
    if(sdl_mods & KMOD_LGUI)   ibus_mods |= IBUS_SUPER_MASK;
    if(sdl_mods & KMOD_RGUI)   ibus_mods |= IBUS_META_MASK;

    return ibus_mods;
}

static const char *
IBus_GetVariantText(DBusConnection *conn, DBusMessageIter *iter, SDL_DBusContext *dbus)
{
    /* The text we need is nested weirdly, use dbus-monitor to see the structure better */
    const char *text = NULL;
    DBusMessageIter sub1, sub2;

    if(dbus->message_iter_get_arg_type(iter) != DBUS_TYPE_VARIANT){
        return NULL;
    }
    
    dbus->message_iter_recurse(iter, &sub1);
    
    if(dbus->message_iter_get_arg_type(&sub1) != DBUS_TYPE_STRUCT){
        return NULL;
    }
    
    dbus->message_iter_recurse(&sub1, &sub2);
    
    if(dbus->message_iter_get_arg_type(&sub2) != DBUS_TYPE_STRING){
        return NULL;
    }
    
    const char *struct_id = NULL;
    dbus->message_iter_get_basic(&sub2, &struct_id);
    if(!struct_id || SDL_strncmp(struct_id, "IBusText", sizeof("IBusText")) != 0){
        return NULL;
    }
    
    dbus->message_iter_next(&sub2);
    dbus->message_iter_next(&sub2);
    
    if(dbus->message_iter_get_arg_type(&sub2) != DBUS_TYPE_STRING){
        return NULL;
    }
    
    dbus->message_iter_get_basic(&sub2, &text);
    
    return text;
}

static DBusHandlerResult
IBus_MessageFilter(DBusConnection *conn, DBusMessage *msg, void *user_data)
{
    SDL_DBusContext *dbus = (SDL_DBusContext *)user_data;
        
    if(dbus->message_is_signal(msg, IBUS_INPUT_INTERFACE, "CommitText")){
        DBusMessageIter iter;
        dbus->message_iter_init(msg, &iter);
        
        const char *text = IBus_GetVariantText(conn, &iter, dbus);
        if(text && *text){
            SDL_SendKeyboardText(text);
        }
        
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    if(dbus->message_is_signal(msg, IBUS_INPUT_INTERFACE, "UpdatePreeditText")){
        DBusMessageIter iter;
        dbus->message_iter_init(msg, &iter);
        const char *text = IBus_GetVariantText(conn, &iter, dbus);
        
        if(!dbus->message_iter_next(&iter)){
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        
        if(!dbus->message_iter_get_arg_type(&iter) == DBUS_TYPE_UINT32){
            return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
        }
        
        Uint32 cursor_pos = 0;
        
        dbus->message_iter_get_basic(&iter, &cursor_pos);
        
        if(text){
            SDL_SendEditingText(text, cursor_pos, SDL_strlen(text) /*FIXME: utf8 length? */);
        }
        
        SDL_IBus_UpdateTextRect(NULL);
        
        return DBUS_HANDLER_RESULT_HANDLED;
    }
    
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static char *
IBus_GetAddressFromFile(const char *file_path)
{
    FILE *addr_file = fopen(file_path, "r");
    if(!addr_file){
        return NULL;
    }

    char addr_buf[1024];
    SDL_bool success = SDL_FALSE;

    while(fgets(addr_buf, sizeof(addr_buf), addr_file)){
        if(SDL_strncmp(addr_buf, "IBUS_ADDRESS=", sizeof("IBUS_ADDRESS=")-1) == 0){
            size_t sz = SDL_strlen(addr_buf);
            if(addr_buf[sz-1] == '\n') addr_buf[sz-1] = 0;
            if(addr_buf[sz-2] == '\r') addr_buf[sz-2] = 0;
            success = SDL_TRUE;
            break;
        }
    }

    fclose(addr_file);

    if(success){
        char *result = SDL_strdup(addr_buf + (sizeof("IBUS_ADDRESS=") - 1));
        return result;
    } else {
        return NULL;
    }
}

static char *
IBus_GetDBusAddress(void)
{
    if(ibus_addr_file){
        return IBus_GetAddressFromFile(ibus_addr_file);
    }
    
    SDL_DBusContext *dbus = SDL_DBus_GetContext();
    
    if(!dbus){
        return NULL;
    }
    
    /* Use this environment variable if it exists. */
    const char *addr = SDL_getenv("IBUS_ADDRESS");
    if(addr && *addr){
        return SDL_strdup(addr);
    }
    
    /* Otherwise, we have to get the hostname, display, machine id, config dir
       and look up the address from a filepath using all those bits, eek. */
    const char *disp_env = SDL_getenv("DISPLAY");
    char *display = NULL;
    
    if(!disp_env || !*disp_env){
        display = SDL_strdup(":0.0");
    } else {
        display = SDL_strdup(disp_env);
    }
    
    const char *host = display;
    char *disp_num   = SDL_strrchr(display, ':'), 
         *screen_num = SDL_strrchr(display, '.');
    
    if(!disp_num){
        SDL_free(display);
        return NULL;
    }
    
    *disp_num = 0;
    disp_num++;
    
    if(screen_num){
        *screen_num = 0;
    }
    
    if(!*host){
        host = "unix";
    }
        
    char config_dir[PATH_MAX];
    SDL_memset(config_dir, 0, sizeof(config_dir));
    
    const char *conf_env = SDL_getenv("XDG_CONFIG_HOME");
    if(conf_env && *conf_env){
        SDL_strlcpy(config_dir, conf_env, sizeof(config_dir));
    } else {
        const char *home_env = SDL_getenv("HOME");
        if(!home_env || !*home_env){
            SDL_free(display);
            return NULL;
        }
        SDL_snprintf(config_dir, sizeof(config_dir), "%s/.config", home_env);
    }
    
    char *key = dbus->get_local_machine_id();
    
    char file_path[PATH_MAX];
    SDL_memset(file_path, 0, sizeof(file_path));
    SDL_snprintf(file_path, sizeof(file_path), "%s/ibus/bus/%s-%s-%s", 
                                               config_dir, key, host, disp_num);
    dbus->free(key);
    SDL_free(display);
    
    ibus_addr_file = SDL_strdup(file_path);
    
    /* Now actually read the address from the file */
    return IBus_GetAddressFromFile(file_path);
}

SDL_bool
SDL_IBus_Init(void)
{
    SDL_bool result = SDL_FALSE;
    SDL_DBusContext *dbus = SDL_DBus_GetContext();
    char *path = NULL;

    if(dbus){
        char *addr = IBus_GetDBusAddress();
        if(!addr){
            return SDL_FALSE;
        }

        ibus_conn = dbus->connection_open_private(addr, NULL);

        SDL_free(addr);

        if(!ibus_conn){
            return SDL_FALSE;
        }

        dbus->connection_flush(ibus_conn);
        
        if(!dbus->bus_register(ibus_conn, NULL)){
            ibus_conn = NULL;
            return SDL_FALSE;
        }
        
        dbus->connection_flush(ibus_conn);

        DBusMessage *msg = dbus->message_new_method_call(IBUS_SERVICE,
                                                         IBUS_PATH,
                                                         IBUS_INTERFACE,
                                                         "CreateInputContext");
        if(msg){
            const char *client_name = "SDL2_Application";
            dbus->message_append_args(msg,
                                      DBUS_TYPE_STRING, &client_name,
                                      DBUS_TYPE_INVALID);
        }
        
        if(msg){
            DBusMessage *reply;
            
            reply = dbus->connection_send_with_reply_and_block(ibus_conn, msg, 1000, NULL);
            if(reply){
                if(dbus->message_get_args(reply, NULL,
                                           DBUS_TYPE_OBJECT_PATH, &path,
                                           DBUS_TYPE_INVALID)){
                    input_ctx_path = SDL_strdup(path);
                    result = SDL_TRUE;                          
                }
                dbus->message_unref(reply);
            }
            dbus->message_unref(msg);
        }
    }

    if(result){
        DBusMessage *msg = dbus->message_new_method_call(IBUS_SERVICE,
                                                         input_ctx_path,
                                                         IBUS_INPUT_INTERFACE,
                                                         "SetCapabilities");
        if(msg){
            Uint32 caps = IBUS_CAP_FOCUS | IBUS_CAP_PREEDIT_TEXT;
            dbus->message_append_args(msg,
                                      DBUS_TYPE_UINT32, &caps,
                                      DBUS_TYPE_INVALID);
        }
        
        if(msg){
            if(dbus->connection_send(ibus_conn, msg, NULL)){
                dbus->connection_flush(ibus_conn);
            }
            dbus->message_unref(msg);
        }
        
        dbus->bus_add_match(ibus_conn, "type='signal',interface='org.freedesktop.IBus.InputContext'", NULL);
        dbus->connection_add_filter(ibus_conn, &IBus_MessageFilter, dbus, NULL);
    }

    if(SDL_GetFocusWindow()){
        SDL_IBus_SetFocus(SDL_TRUE);
    }
    
    return result;
}

void
SDL_IBus_Quit(void)
{
    if(input_ctx_path){
        SDL_free(input_ctx_path);
        input_ctx_path = NULL;
    }
    
    if(ibus_addr_file){
        SDL_free(ibus_addr_file);
        ibus_addr_file = NULL;
    }
    
    SDL_DBusContext *dbus = SDL_DBus_GetContext();
    
    if(dbus && ibus_conn){
        dbus->connection_close(ibus_conn);
        dbus->connection_unref(ibus_conn);
    }
    
    SDL_memset(&ibus_cursor_rect, 0, sizeof(ibus_cursor_rect));
}

static void
IBus_SimpleMessage(const char *method)
{
    if(!input_ctx_path){
        if(!SDL_IBus_Init()) return;
    }
    
    SDL_DBusContext *dbus = SDL_DBus_GetContext();
    
    if(dbus && ibus_conn){
        DBusMessage *msg = dbus->message_new_method_call(IBUS_SERVICE,
                                                         input_ctx_path,
                                                         IBUS_INPUT_INTERFACE,
                                                         method);
        if(msg){
            if(dbus->connection_send(ibus_conn, msg, NULL)){
                dbus->connection_flush(ibus_conn);
            }
            dbus->message_unref(msg);
        }
    }
}

void
SDL_IBus_SetFocus(SDL_bool focused)
{ 
    const char *method = focused ? "FocusIn" : "FocusOut";
    IBus_SimpleMessage(method);
}

void
SDL_IBus_Reset(void)
{
    IBus_SimpleMessage("Reset");
}

SDL_bool
SDL_IBus_ProcessKeyEvent(Uint32 keysym, Uint32 keycode)
{
    if(!input_ctx_path){
        if(!SDL_IBus_Init()) return SDL_FALSE;
    }
 
    SDL_bool result = SDL_FALSE;   
    SDL_DBusContext *dbus = SDL_DBus_GetContext();
    
    if(dbus && ibus_conn){
        DBusMessage *msg = dbus->message_new_method_call(IBUS_SERVICE,
                                                         input_ctx_path,
                                                         IBUS_INPUT_INTERFACE,
                                                         "ProcessKeyEvent");
        if(msg){
            Uint32 mods = IBus_ModState();
            dbus->message_append_args(msg,
                                      DBUS_TYPE_UINT32, &keysym,
                                      DBUS_TYPE_UINT32, &keycode,
                                      DBUS_TYPE_UINT32, &mods,
                                      DBUS_TYPE_INVALID);
        }
        
        if(msg){
            DBusMessage *reply;
            
            reply = dbus->connection_send_with_reply_and_block(ibus_conn, msg, 300, NULL);
            if(reply){
                if(!dbus->message_get_args(reply, NULL,
                                           DBUS_TYPE_BOOLEAN, &result,
                                           DBUS_TYPE_INVALID)){
                    result = SDL_FALSE;                         
                }
                dbus->message_unref(reply);
            }
            dbus->message_unref(msg);
        }
        
    }
    
    return result;
}

void
SDL_IBus_UpdateTextRect(SDL_Rect *rect)
{
    if(rect){
        SDL_memcpy(&ibus_cursor_rect, rect, sizeof(ibus_cursor_rect));
    }
    
    SDL_Window *focused_win = SDL_GetFocusWindow();

    if(!focused_win) return;

    int x = 0, y = 0;
    SDL_GetWindowPosition(focused_win, &x, &y);
    x += ibus_cursor_rect.x;
    y += ibus_cursor_rect.y;
    
    if(!input_ctx_path){
        if(!SDL_IBus_Init()) return;
    }
    
    SDL_DBusContext *dbus = SDL_DBus_GetContext();
    
    if(dbus && ibus_conn){
        DBusMessage *msg = dbus->message_new_method_call(IBUS_SERVICE,
                                                         input_ctx_path,
                                                         IBUS_INPUT_INTERFACE,
                                                         "SetCursorLocation");
        if(msg){
            dbus->message_append_args(msg,
                                      DBUS_TYPE_INT32, &x,
                                      DBUS_TYPE_INT32, &y,
                                      DBUS_TYPE_INT32, &ibus_cursor_rect.w,
                                      DBUS_TYPE_INT32, &ibus_cursor_rect.h,
                                      DBUS_TYPE_INVALID);
        }
        
        if(msg){
            if(dbus->connection_send(ibus_conn, msg, NULL)){
                dbus->connection_flush(ibus_conn);
            }
            dbus->message_unref(msg);
        }
    }
}

void
SDL_IBus_PumpEvents(void)
{
    SDL_DBusContext *dbus = SDL_DBus_GetContext();
    
    if(dbus && ibus_conn){
        dbus->connection_read_write(ibus_conn, 0);
    
        while(dbus->connection_dispatch(ibus_conn) == DBUS_DISPATCH_DATA_REMAINS){
            /* Do nothing, actual work happens in IBus_MessageFilter */
        }
    }
}

#endif
