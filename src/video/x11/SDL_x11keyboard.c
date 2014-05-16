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

#if SDL_VIDEO_DRIVER_X11

#include "SDL_x11video.h"

#include "../../events/SDL_keyboard_c.h"
#include "../../events/scancodes_darwin.h"
#include "../../events/scancodes_xfree86.h"

#include <X11/keysym.h>

#include "imKStoUCS.h"

/* *INDENT-OFF* */
static const struct {
    KeySym keysym;
    SDL_Scancode scancode;
} KeySymToSDLScancode[] = {
    { XK_Return, SDL_SCANCODE_RETURN },
    { XK_Escape, SDL_SCANCODE_ESCAPE },
    { XK_BackSpace, SDL_SCANCODE_BACKSPACE },
    { XK_Tab, SDL_SCANCODE_TAB },
    { XK_Caps_Lock, SDL_SCANCODE_CAPSLOCK },
    { XK_F1, SDL_SCANCODE_F1 },
    { XK_F2, SDL_SCANCODE_F2 },
    { XK_F3, SDL_SCANCODE_F3 },
    { XK_F4, SDL_SCANCODE_F4 },
    { XK_F5, SDL_SCANCODE_F5 },
    { XK_F6, SDL_SCANCODE_F6 },
    { XK_F7, SDL_SCANCODE_F7 },
    { XK_F8, SDL_SCANCODE_F8 },
    { XK_F9, SDL_SCANCODE_F9 },
    { XK_F10, SDL_SCANCODE_F10 },
    { XK_F11, SDL_SCANCODE_F11 },
    { XK_F12, SDL_SCANCODE_F12 },
    { XK_Print, SDL_SCANCODE_PRINTSCREEN },
    { XK_Scroll_Lock, SDL_SCANCODE_SCROLLLOCK },
    { XK_Pause, SDL_SCANCODE_PAUSE },
    { XK_Insert, SDL_SCANCODE_INSERT },
    { XK_Home, SDL_SCANCODE_HOME },
    { XK_Prior, SDL_SCANCODE_PAGEUP },
    { XK_Delete, SDL_SCANCODE_DELETE },
    { XK_End, SDL_SCANCODE_END },
    { XK_Next, SDL_SCANCODE_PAGEDOWN },
    { XK_Right, SDL_SCANCODE_RIGHT },
    { XK_Left, SDL_SCANCODE_LEFT },
    { XK_Down, SDL_SCANCODE_DOWN },
    { XK_Up, SDL_SCANCODE_UP },
    { XK_Num_Lock, SDL_SCANCODE_NUMLOCKCLEAR },
    { XK_KP_Divide, SDL_SCANCODE_KP_DIVIDE },
    { XK_KP_Multiply, SDL_SCANCODE_KP_MULTIPLY },
    { XK_KP_Subtract, SDL_SCANCODE_KP_MINUS },
    { XK_KP_Add, SDL_SCANCODE_KP_PLUS },
    { XK_KP_Enter, SDL_SCANCODE_KP_ENTER },
    { XK_KP_Delete, SDL_SCANCODE_KP_PERIOD },
    { XK_KP_End, SDL_SCANCODE_KP_1 },
    { XK_KP_Down, SDL_SCANCODE_KP_2 },
    { XK_KP_Next, SDL_SCANCODE_KP_3 },
    { XK_KP_Left, SDL_SCANCODE_KP_4 },
    { XK_KP_Begin, SDL_SCANCODE_KP_5 },
    { XK_KP_Right, SDL_SCANCODE_KP_6 },
    { XK_KP_Home, SDL_SCANCODE_KP_7 },
    { XK_KP_Up, SDL_SCANCODE_KP_8 },
    { XK_KP_Prior, SDL_SCANCODE_KP_9 },
    { XK_KP_Insert, SDL_SCANCODE_KP_0 },
    { XK_KP_Decimal, SDL_SCANCODE_KP_PERIOD },
    { XK_KP_1, SDL_SCANCODE_KP_1 },
    { XK_KP_2, SDL_SCANCODE_KP_2 },
    { XK_KP_3, SDL_SCANCODE_KP_3 },
    { XK_KP_4, SDL_SCANCODE_KP_4 },
    { XK_KP_5, SDL_SCANCODE_KP_5 },
    { XK_KP_6, SDL_SCANCODE_KP_6 },
    { XK_KP_7, SDL_SCANCODE_KP_7 },
    { XK_KP_8, SDL_SCANCODE_KP_8 },
    { XK_KP_9, SDL_SCANCODE_KP_9 },
    { XK_KP_0, SDL_SCANCODE_KP_0 },
    { XK_KP_Decimal, SDL_SCANCODE_KP_PERIOD },
    { XK_Hyper_R, SDL_SCANCODE_APPLICATION },
    { XK_KP_Equal, SDL_SCANCODE_KP_EQUALS },
    { XK_F13, SDL_SCANCODE_F13 },
    { XK_F14, SDL_SCANCODE_F14 },
    { XK_F15, SDL_SCANCODE_F15 },
    { XK_F16, SDL_SCANCODE_F16 },
    { XK_F17, SDL_SCANCODE_F17 },
    { XK_F18, SDL_SCANCODE_F18 },
    { XK_F19, SDL_SCANCODE_F19 },
    { XK_F20, SDL_SCANCODE_F20 },
    { XK_F21, SDL_SCANCODE_F21 },
    { XK_F22, SDL_SCANCODE_F22 },
    { XK_F23, SDL_SCANCODE_F23 },
    { XK_F24, SDL_SCANCODE_F24 },
    { XK_Execute, SDL_SCANCODE_EXECUTE },
    { XK_Help, SDL_SCANCODE_HELP },
    { XK_Menu, SDL_SCANCODE_MENU },
    { XK_Select, SDL_SCANCODE_SELECT },
    { XK_Cancel, SDL_SCANCODE_STOP },
    { XK_Redo, SDL_SCANCODE_AGAIN },
    { XK_Undo, SDL_SCANCODE_UNDO },
    { XK_Find, SDL_SCANCODE_FIND },
    { XK_KP_Separator, SDL_SCANCODE_KP_COMMA },
    { XK_Sys_Req, SDL_SCANCODE_SYSREQ },
    { XK_Control_L, SDL_SCANCODE_LCTRL },
    { XK_Shift_L, SDL_SCANCODE_LSHIFT },
    { XK_Alt_L, SDL_SCANCODE_LALT },
    { XK_Meta_L, SDL_SCANCODE_LGUI },
    { XK_Super_L, SDL_SCANCODE_LGUI },
    { XK_Control_R, SDL_SCANCODE_RCTRL },
    { XK_Shift_R, SDL_SCANCODE_RSHIFT },
    { XK_Alt_R, SDL_SCANCODE_RALT },
    { XK_Meta_R, SDL_SCANCODE_RGUI },
    { XK_Super_R, SDL_SCANCODE_RGUI },
    { XK_Mode_switch, SDL_SCANCODE_MODE },
};

static const struct
{
    SDL_Scancode const *table;
    int table_size;
} scancode_set[] = {
    { darwin_scancode_table, SDL_arraysize(darwin_scancode_table) },
    { xfree86_scancode_table, SDL_arraysize(xfree86_scancode_table) },
    { xfree86_scancode_table2, SDL_arraysize(xfree86_scancode_table2) },
};
/* *INDENT-OFF* */

#ifdef SDL_USE_IBUS
#ifdef SDL_IBUS_DYNAMIC_IBUS
    #include "SDL_loadso.h"
#endif

static int IME_IBusLoadSyms(SDL_IBusHandler* ibus)
{       
    SDL_bool ok = SDL_TRUE;

/* Dynamically load libraries? */
#ifdef SDL_IBUS_DYNAMIC_IBUS
    void* libibus    = SDL_LoadObject(SDL_IBUS_DYNAMIC_IBUS);
    void* libgobject = SDL_LoadObject(SDL_IBUS_DYNAMIC_GOBJECT);
    void* libglib    = SDL_LoadObject(SDL_IBUS_DYNAMIC_GLIB);
    
    if(!(libibus && libgobject && libglib)) return SDL_FALSE;
    
    #define SDL_IBUS_SYM(handle, name) \
        ibus->name = SDL_LoadFunction(handle, #name); \
        ok = ok && ibus->name
#else
    #define SDL_IBUS_SYM(handle, name) \
        ibus->name = &name

#endif

    SDL_IBUS_SYM(libibus, ibus_init);
    SDL_IBUS_SYM(libibus, ibus_bus_new);
    SDL_IBUS_SYM(libibus, ibus_bus_is_connected);
    SDL_IBUS_SYM(libibus, ibus_bus_create_input_context);
    SDL_IBUS_SYM(libibus, ibus_input_context_set_capabilities);
    SDL_IBUS_SYM(libibus, ibus_input_context_set_cursor_location);
    SDL_IBUS_SYM(libibus, ibus_input_context_process_key_event);
    SDL_IBUS_SYM(libibus, ibus_input_context_reset);
    SDL_IBUS_SYM(libibus, ibus_input_context_focus_in);
    SDL_IBUS_SYM(libibus, ibus_input_context_focus_out);
    SDL_IBUS_SYM(libibus, ibus_text_get_text);
    SDL_IBUS_SYM(libibus, ibus_text_get_length);
    SDL_IBUS_SYM(libibus, ibus_proxy_destroy);
    SDL_IBUS_SYM(libibus, ibus_bus_exit);
    
    SDL_IBUS_SYM(libgobject, g_signal_connect_data);
    SDL_IBUS_SYM(libgobject, g_signal_handlers_disconnect_matched);
    SDL_IBUS_SYM(libgobject, g_object_unref);
    
    SDL_IBUS_SYM(libglib, g_main_context_new);
    SDL_IBUS_SYM(libglib, g_main_context_push_thread_default);
    SDL_IBUS_SYM(libglib, g_main_context_pop_thread_default);
    SDL_IBUS_SYM(libglib, g_main_context_iteration);
    SDL_IBUS_SYM(libglib, g_main_context_unref);
    
    #undef SDL_IBUS_SYM
    
#ifdef SDL_IBUS_DYNAMIC_IBUS
    if(ok){
        ibus->libibus_handle = libibus;
        ibus->libgobject_handle = libgobject;
        ibus->libglib_handle = libglib;
    } else {
        SDL_UnloadObject(libibus);
        SDL_UnloadObject(libgobject);
        SDL_UnloadObject(libglib);
    }
#endif
 
    return ok;
}

static void IME_IBusTextCommitted(IBusInputContext *c, IBusText *t, SDL_IBusHandler* ibus)
{
    const char* text = ibus->ibus_text_get_text(t);
    SDL_SendKeyboardText(text);
}

static void IME_IBusTextEdit(IBusInputContext *c, IBusText *t, guint cursor_pos, 
                                                  gboolean visible, SDL_IBusHandler* ibus)
{
    if(visible){
        /* Reposition the candidate list */
        SDL_Window* focused_win = SDL_GetFocusWindow();
        if(focused_win){
            int x = 0, y = 0;
            
            SDL_GetWindowPosition(focused_win, &x, &y);
            SDL_Rect* rect = &ibus->cursor_rect;
            
           ibus->ibus_input_context_set_cursor_location(
                ibus->context, rect->x + x, rect->y + y, rect->w, rect->h);
        }
        
        const char* text = ibus->ibus_text_get_text(t);
        int num_chars = ibus->ibus_text_get_length(t);
            
        SDL_SendEditingText(text, cursor_pos, num_chars);
    }
}

static void
IME_IBusSetupConnection(SDL_IBusHandler* ibus)
{
    ibus->g_main_context_push_thread_default(ibus->glib_main_context);
        
    ibus->context = ibus->ibus_bus_create_input_context(ibus->bus, "SDL-2");
    ibus->ibus_input_context_set_capabilities(ibus->context, IBUS_CAP_PREEDIT_TEXT | IBUS_CAP_FOCUS );
    
    ibus->g_signal_connect(ibus->context, "commit-text", G_CALLBACK(IME_IBusTextCommitted), ibus);
    ibus->g_signal_connect(ibus->context, "update-preedit-text", G_CALLBACK(IME_IBusTextEdit), ibus);
    
    ibus->g_main_context_pop_thread_default(ibus->glib_main_context);
}

static void
IME_IBusConnectCallback(IBusBus* unused, SDL_IBusHandler *ibus)
{
    IME_IBusSetupConnection(ibus);
}

static void
IME_Init(SDL_VideoData *videodata)
{
    if(!videodata->ibus.initialized){
        SDL_IBusHandler* ibus = &videodata->ibus;
        
        if(!IME_IBusLoadSyms(ibus)) return;
        
        ibus->ibus_init();
        
        ibus->initialized = SDL_TRUE;
        ibus->glib_main_context = ibus->g_main_context_new();
        ibus->bus = ibus->ibus_bus_new();

        if (ibus->ibus_bus_is_connected(ibus->bus)) {
            IME_IBusSetupConnection(ibus);
        } else {      
            ibus->g_main_context_push_thread_default(ibus->glib_main_context);
            ibus->g_signal_connect(ibus->bus, "connected", G_CALLBACK(IME_IBusConnectCallback), ibus);
            ibus->g_main_context_pop_thread_default(ibus->glib_main_context);
        }
    }
}

static void IME_Quit(SDL_VideoData *videodata)
{
    SDL_IBusHandler *ibus = &videodata->ibus;
    
    if(ibus->initialized){
/* Unload libraries */
#ifdef SDL_IBUS_DYNAMIC_IBUS
    
        if(ibus->libibus_handle)    SDL_UnloadObject(ibus->libibus_handle);
        if(ibus->libgobject_handle) SDL_UnloadObject(ibus->libgobject_handle);
        if(ibus->libglib_handle)    SDL_UnloadObject(ibus->libglib_handle);

#endif

        ibus->g_main_context_push_thread_default(ibus->glib_main_context);

        ibus->g_signal_handlers_disconnect_matched(ibus->context, 
            G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, ibus);
            
        /* FIXME: More cleanup is required for bus, input context e.t.c. */
        
        ibus->g_main_context_pop_thread_default(ibus->glib_main_context);
        ibus->g_main_context_unref(ibus->glib_main_context);
    }
    
     memset(ibus, 0, sizeof(*ibus));
}
#endif

/* This function only works for keyboards in US QWERTY layout */
static SDL_Scancode
X11_KeyCodeToSDLScancode(Display *display, KeyCode keycode)
{
    KeySym keysym;
    int i;

#if SDL_VIDEO_DRIVER_X11_HAS_XKBKEYCODETOKEYSYM
    keysym = X11_XkbKeycodeToKeysym(display, keycode, 0, 0);
#else
    keysym = XKeycodeToKeysym(display, keycode, 0);
#endif
    if (keysym == NoSymbol) {
        return SDL_SCANCODE_UNKNOWN;
    }

    if (keysym >= XK_A && keysym <= XK_Z) {
        return SDL_SCANCODE_A + (keysym - XK_A);
    }

    if (keysym >= XK_0 && keysym <= XK_9) {
        return SDL_SCANCODE_0 + (keysym - XK_0);
    }

    for (i = 0; i < SDL_arraysize(KeySymToSDLScancode); ++i) {
        if (keysym == KeySymToSDLScancode[i].keysym) {
            return KeySymToSDLScancode[i].scancode;
        }
    }
    return SDL_SCANCODE_UNKNOWN;
}

static Uint32
X11_KeyCodeToUcs4(Display *display, KeyCode keycode)
{
    KeySym keysym;

#if SDL_VIDEO_DRIVER_X11_HAS_XKBKEYCODETOKEYSYM
    keysym = X11_XkbKeycodeToKeysym(display, keycode, 0, 0);
#else
    keysym = XKeycodeToKeysym(display, keycode, 0);
#endif
    if (keysym == NoSymbol) {
        return 0;
    }

    return X11_KeySymToUcs4(keysym);
}

int
X11_InitKeyboard(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int i = 0;
    int j = 0;
    int min_keycode, max_keycode;
    struct {
        SDL_Scancode scancode;
        KeySym keysym;
        int value;
    } fingerprint[] = {
        { SDL_SCANCODE_HOME, XK_Home, 0 },
        { SDL_SCANCODE_PAGEUP, XK_Prior, 0 },
        { SDL_SCANCODE_UP, XK_Up, 0 },
        { SDL_SCANCODE_LEFT, XK_Left, 0 },
        { SDL_SCANCODE_DELETE, XK_Delete, 0 },
        { SDL_SCANCODE_KP_ENTER, XK_KP_Enter, 0 },
    };
    int best_distance;
    int best_index;
    int distance;

    X11_XAutoRepeatOn(data->display);

    /* Try to determine which scancodes are being used based on fingerprint */
    best_distance = SDL_arraysize(fingerprint) + 1;
    best_index = -1;
    X11_XDisplayKeycodes(data->display, &min_keycode, &max_keycode);
    for (i = 0; i < SDL_arraysize(fingerprint); ++i) {
        fingerprint[i].value =
            X11_XKeysymToKeycode(data->display, fingerprint[i].keysym) -
            min_keycode;
    }
    for (i = 0; i < SDL_arraysize(scancode_set); ++i) {
        /* Make sure the scancode set isn't too big */
        if ((max_keycode - min_keycode + 1) <= scancode_set[i].table_size) {
            continue;
        }
        distance = 0;
        for (j = 0; j < SDL_arraysize(fingerprint); ++j) {
            if (fingerprint[j].value < 0
                || fingerprint[j].value >= scancode_set[i].table_size) {
                distance += 1;
            } else if (scancode_set[i].table[fingerprint[j].value] != fingerprint[j].scancode) {
                distance += 1;
            }
        }
        if (distance < best_distance) {
            best_distance = distance;
            best_index = i;
        }
    }
    if (best_index >= 0 && best_distance <= 2) {
#ifdef DEBUG_KEYBOARD
        printf("Using scancode set %d, min_keycode = %d, max_keycode = %d, table_size = %d\n", best_index, min_keycode, max_keycode, scancode_set[best_index].table_size);
#endif
        SDL_memcpy(&data->key_layout[min_keycode], scancode_set[best_index].table,
                   sizeof(SDL_Scancode) * scancode_set[best_index].table_size);
    }
    else {
        SDL_Keycode keymap[SDL_NUM_SCANCODES];

        printf
            ("Keyboard layout unknown, please send the following to the SDL mailing list (sdl@libsdl.org):\n");

        /* Determine key_layout - only works on US QWERTY layout */
        SDL_GetDefaultKeymap(keymap);
        for (i = min_keycode; i <= max_keycode; ++i) {
            KeySym sym;
#if SDL_VIDEO_DRIVER_X11_HAS_XKBKEYCODETOKEYSYM
            sym = X11_XkbKeycodeToKeysym(data->display, i, 0, 0);
#else
            sym = XKeycodeToKeysym(data->display, i, 0);
#endif
            if (sym != NoSymbol) {
                SDL_Scancode scancode;
                printf("code = %d, sym = 0x%X (%s) ", i - min_keycode,
                       (unsigned int) sym, X11_XKeysymToString(sym));
                scancode = X11_KeyCodeToSDLScancode(data->display, i);
                data->key_layout[i] = scancode;
                if (scancode == SDL_SCANCODE_UNKNOWN) {
                    printf("scancode not found\n");
                } else {
                    printf("scancode = %d (%s)\n", scancode, SDL_GetScancodeName(scancode));
                }
            }
        }
    }

    X11_UpdateKeymap(_this);

    SDL_SetScancodeName(SDL_SCANCODE_APPLICATION, "Menu");

    return 0;
}

void
X11_UpdateKeymap(_THIS)
{
    SDL_VideoData *data = (SDL_VideoData *) _this->driverdata;
    int i;
    SDL_Scancode scancode;
    SDL_Keycode keymap[SDL_NUM_SCANCODES];

    SDL_GetDefaultKeymap(keymap);
    for (i = 0; i < SDL_arraysize(data->key_layout); i++) {
        Uint32 key;

        /* Make sure this is a valid scancode */
        scancode = data->key_layout[i];
        if (scancode == SDL_SCANCODE_UNKNOWN) {
            continue;
        }

        /* See if there is a UCS keycode for this scancode */
        key = X11_KeyCodeToUcs4(data->display, (KeyCode)i);
        if (key) {
            keymap[scancode] = key;
        }
    }
    SDL_SetKeymap(0, keymap, SDL_NUM_SCANCODES);
}

void
X11_QuitKeyboard(_THIS)
{
#ifdef SDL_USE_IBUS
    IME_Quit((SDL_VideoData*)_this->driverdata);
#endif
}

void
X11_StartTextInput(_THIS)
{
#ifdef SDL_USE_IBUS
    SDL_VideoData *videodata = (SDL_VideoData *)_this->driverdata;

    if(videodata->ibus.init_attempts == 0){
        /* This function will set ibus.initialized to SDL_TRUE if successful */
        IME_Init(videodata);
        videodata->ibus.init_attempts++;
    }

    if(videodata->ibus.initialized){
        videodata->ibus.active = SDL_TRUE;
    }
#endif
}

void
X11_StopTextInput(_THIS)
{
#ifdef SDL_USE_IBUS
    SDL_VideoData *videodata = (SDL_VideoData *)_this->driverdata;
    
    if(videodata->ibus.initialized){
        videodata->ibus.ibus_input_context_reset(videodata->ibus.context);
    }
    videodata->ibus.active = SDL_FALSE;
#endif
}

void
X11_SetTextInputRect(_THIS, SDL_Rect *rect)
{
#ifdef SDL_USE_IBUS
    if (!rect) {
        SDL_InvalidParamError("rect");
        return;
    }
    SDL_VideoData *videodata = (SDL_VideoData *)_this->driverdata;
    
    if (videodata->ibus.init_attempts == 0) {
        IME_Init(videodata);
        videodata->ibus.init_attempts++;
    }
    
    if(videodata->ibus.initialized){
        /* Save the rect so the cursor can be updated when the window moves */
        SDL_memcpy(&videodata->ibus.cursor_rect, rect, sizeof(SDL_Rect));    
        
        int x, y;
        SDL_Window* focused_win = SDL_GetFocusWindow();
        if(focused_win){
            SDL_GetWindowPosition(focused_win, &x, &y);
        
            videodata->ibus.ibus_input_context_set_cursor_location(
                videodata->ibus.context, rect->x + x, rect->y + y, rect->w, rect->h);
        }
   }
#endif
}

#endif /* SDL_VIDEO_DRIVER_X11 */

/* vi: set ts=4 sw=4 expandtab: */
