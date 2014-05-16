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

#ifndef _SDL_x11video_h
#define _SDL_x11video_h

#include "SDL_keycode.h"

#include "../SDL_sysvideo.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#if SDL_VIDEO_DRIVER_X11_XCURSOR
#include <X11/Xcursor/Xcursor.h>
#endif
#if SDL_VIDEO_DRIVER_X11_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#if SDL_VIDEO_DRIVER_X11_XINPUT2
#include <X11/extensions/XInput2.h>
#endif
#if SDL_VIDEO_DRIVER_X11_XRANDR
#include <X11/extensions/Xrandr.h>
#endif
#if SDL_VIDEO_DRIVER_X11_XSCRNSAVER
#include <X11/extensions/scrnsaver.h>
#endif
#if SDL_VIDEO_DRIVER_X11_XSHAPE
#include <X11/extensions/shape.h>
#endif
#if SDL_VIDEO_DRIVER_X11_XVIDMODE
#include <X11/extensions/xf86vmode.h>
#endif

#ifdef HAVE_DBUS_DBUS_H
#define SDL_USE_LIBDBUS 1
#include <dbus/dbus.h>
#endif

#ifdef HAVE_IBUS_IBUS_H
#define SDL_USE_IBUS 1
#include <ibus-1.0/ibus.h>
#endif

#include "SDL_x11dyn.h"

#include "SDL_x11clipboard.h"
#include "SDL_x11events.h"
#include "SDL_x11keyboard.h"
#include "SDL_x11modes.h"
#include "SDL_x11mouse.h"
#include "SDL_x11opengl.h"
#include "SDL_x11window.h"

/* Private display data */

#ifdef HAVE_IBUS_IBUS_H
typedef struct SDL_IBusHandler
{
    IBusBus* bus;
    IBusInputContext* context;
    GMainContext* glib_main_context;
    SDL_bool active;
    SDL_bool initialized;
    SDL_Rect cursor_rect;
    int init_attempts;
    
    void *libibus_handle, *libgobject_handle, *libglib_handle;
    
    /* Dynamic function prototypes */
    
    void (*ibus_init)(void);
    IBusBus *(*ibus_bus_new)(void);
    gboolean (*ibus_bus_is_connected)(IBusBus* bus);
    IBusInputContext *(*ibus_bus_create_input_context)(IBusBus *bus, 
        const gchar *client_name);
    void (*ibus_input_context_set_capabilities)(IBusInputContext *, 
        guint32 capabilities);
    void (*ibus_input_context_set_cursor_location)(IBusInputContext *, 
        gint32 x, gint32 y, gint32 w, gint32 h);
    gboolean (*ibus_input_context_process_key_event)(IBusInputContext *, 
        guint32 keyval, guint32 keycode, guint32 state);
    void (*ibus_input_context_reset)(IBusInputContext *);
    void (*ibus_input_context_focus_in)(IBusInputContext *);
    void (*ibus_input_context_focus_out)(IBusInputContext *);
    const gchar *(*ibus_text_get_text)(IBusText *);
    guint (*ibus_text_get_length)(IBusText *);
    void (*ibus_proxy_destroy)(IBusProxy *);
    gboolean (*ibus_bus_exit)(IBusBus *, gboolean);
    
    gulong (*g_signal_connect_data)(gpointer, const gchar *, GCallback,
        gpointer, GClosureNotify, GConnectFlags);
    guint (*g_signal_handlers_disconnect_matched)(gpointer, GSignalMatchType,
        guint , GQuark detail, GClosure	*, gpointer, gpointer);
    void (*g_object_unref)(gpointer);
    
    GMainContext *(*g_main_context_new)(void);
    void (*g_main_context_push_thread_default)(GMainContext *);
    void (*g_main_context_pop_thread_default)(GMainContext *);
    gboolean (*g_main_context_iteration)(GMainContext *, gboolean);
    void (*g_main_context_unref)(GMainContext *);
    
} SDL_IBusHandler;
#endif

typedef struct SDL_VideoData
{
    Display *display;
    char *classname;
    pid_t pid;
    XIM im;
    Uint32 screensaver_activity;
    int numwindows;
    SDL_WindowData **windowlist;
    int windowlistlength;

    /* This is true for ICCCM2.0-compliant window managers */
    SDL_bool net_wm;

    /* Useful atoms */
    Atom WM_PROTOCOLS;
    Atom WM_DELETE_WINDOW;
    Atom _NET_WM_STATE;
    Atom _NET_WM_STATE_HIDDEN;
    Atom _NET_WM_STATE_FOCUSED;
    Atom _NET_WM_STATE_MAXIMIZED_VERT;
    Atom _NET_WM_STATE_MAXIMIZED_HORZ;
    Atom _NET_WM_STATE_FULLSCREEN;
    Atom _NET_WM_ALLOWED_ACTIONS;
    Atom _NET_WM_ACTION_FULLSCREEN;
    Atom _NET_WM_NAME;
    Atom _NET_WM_ICON_NAME;
    Atom _NET_WM_ICON;
    Atom _NET_WM_PING;
    Atom _NET_ACTIVE_WINDOW;
    Atom UTF8_STRING;
    Atom PRIMARY;
    Atom XdndEnter;
    Atom XdndPosition;
    Atom XdndStatus;
    Atom XdndTypeList;
    Atom XdndActionCopy;
    Atom XdndDrop;
    Atom XdndFinished;
    Atom XdndSelection;

    SDL_Scancode key_layout[256];
    SDL_bool selection_waiting;
    
#if SDL_USE_IBUS
    SDL_IBusHandler ibus;
#endif

#if SDL_USE_LIBDBUS
    DBusConnection *dbus;
#endif
} SDL_VideoData;

extern SDL_bool X11_UseDirectColorVisuals(void);

SDL_bool SDL_dbus_screensaver_inhibit(_THIS);
void SDL_dbus_screensaver_tickle(_THIS);

#endif /* _SDL_x11video_h */

/* vi: set ts=4 sw=4 expandtab: */
