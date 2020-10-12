/*
 * Copyright (C) Alexey Zhelezov, 2019 www.azslow.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */
/*
 * XCB based toolkit.
 */
#ifndef __XCBT_H
#define __XCBT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <xcb/xcb.h>

// it should match defines and common_atom_names in the code
#define XCBT_COMMON_ATOMS_COUNT 9

/**
 * Some atoms, require XCBT_INIT_ATOMS during connection
 * 
 * WARNING: no checks for x validity and can return 0 when is(could) not initialize(d)
 */
#define XCBT_WM_PROTOCOLS(x) ((x)->catoms[0])
#define XCBT_WM_DELETE_WINDOW(x) ((x)->catoms[1])
#define XCBT_XEMBED_INFO(x) ((x)->catoms[2])
#define XCBT_ATOM_CLIPBOARD(x) ((x)->catoms[3])
#define XCBT_ATOM_UTF8_STRING(x) ((x)->catoms[4])
#define XCBT_ATOM_XSEL_DATA(x) ((x)->catoms[5])
#define XCBT_ATOM_STRING(x) ((x)->catoms[6])
#define XCBT_ATOM_TEXT(x) ((x)->catoms[7])
#define XCBT_ATOM_TARGETS(x) ((x)->catoms[8])


/**
 * Rectangle
 */
typedef struct {
  int16_t x;
  int16_t y;
  int16_t w;
  int16_t h;
} xcbt_rect;

/**
 * Connection is opaque structure. Do not use directly.
 */
typedef struct xcbt_ {
  xcb_connection_t *conn;       // xcb connection
  int               def_screen; // default screen
  xcb_atom_t        catoms[XCBT_COMMON_ATOMS_COUNT]; // common atoms
  
  void             *xlib_dpy;
} *xcbt;

/**
 * Window is opaque structure. Do not use directly.
 */
typedef struct xcbt_window_ {
  xcbt x;
  xcb_window_t wnd;
  int screen;  
  xcb_window_t prt;
  xcbt_rect pos; // position inside parent (always, top window will get spos set)
  int mapped; // bool
} *xcbt_window;

/**
 * Specify how image data should be prepared to be transferred directly into drawable.
 */
typedef struct xcbt_img_prop_ {
  unsigned depth;       // color bits per pixel, f.e. RGB is 24
  unsigned bpp;         // bits per pixel to store it, f.e. RGB is normally 32
  unsigned line_align;  // line size IN BITS should be alinged to that number, f.e. RGB is normally 32
  enum {
    XCBT_LSB_FIRST = 0,
    XCBT_MSB_FIRST
  } byte_order;         // byte order, f.e. XCBT_LSB_FIRST with RGB means 0x000000ff on x86 is "blue".
} xcbt_img_prop;

/**
 * Event handler. If chained, user code is responsible for saving previous handler and user data
 * in the chain and call them when required.
 * 
 * Parameters:
 *   evt - event to process, 
 *         When NULL, the window is about to be destroyed (X window is already destroyed).
 *         In this special case, previous handler must be called.
 *   udata - the data set with xcbt_window_set_handler call
 */
typedef void (*xcbt_window_handler)(xcbt_window xw, xcb_generic_event_t *evt, void *udata);

/**
 * Flags for xcbt_connect
 */
typedef enum {
  XCBT_USE_GL = 1, // allow GL rendering
  XCBT_INIT_ATOMS = 2, // ascure common atoms during connect
} XCBT_CONNECT_FLAGS;

/**
 * Flags for xcbt_move_cursor
 */
typedef enum {
  XCBT_WINDOW   = 0xff00,
  XCBT_RELATIVE = 0xff01,
  XCBT_ABSOLUTE = 0xff02,
} XCBT_MOUSE_FLAGS;

/**
 * Return XCB connection
 *
 * Return:
 *   XCB connection on NULL
 */
#define xcbt_conn(x) ((x)?(x)->conn:(xcb_connection_t *)NULL)

/**
 * Returns default screen
 * 
 * Return:
 *   screen number or -1
 */
#define xcbt_default_screen(x) ((x)?(x)->def_screen:-1)


/**
 * Can be used to check the connect is XLib (and so GL) compatible
 * 
 * Return:
 *   XLib display (as void *), if connection is initiated by XLib
 */
#define xcbt_display(x) ((x)?(x)->xlib_dpy:NULL) 

/**
 * xcb_flush wrapper
 * 
 */
#define xcbt_flush(x) xcb_flush((xcb_connection_t *)x);

/**
 * Initialize xcbt
 * 
 * Parameters:
 *   flags   - bit set of XCBT_CONNECT_FLAGS
 * 
 * Return:
 *   Initialized xcbt or NULL in case of errors.
 */
xcbt xcbt_connect(uint32_t flags);

/**
 * Finalize xcbt
 * 
 * Parameters:
 *   x - xcbt to disconnect (can be NULL)
 */
void xcbt_disconnect(xcbt x);

/**
 * Sync with X server by requesting current focus.
 * Note: does flush internally.
 */
int xcbt_sync(xcbt x);

/*
 * For debugging only. Sync and process incoming events for ~100ms
 */
void xcbt_sync_dbg(xcbt px);


/**
 * Fill properties for specified depth
 * 
 * Return not zero on success
 */
int xcbt_get_img_prop(xcbt px, unsigned depth, xcbt_img_prop *prop);

/**
 * Return connection for window
 *
 * Return:
 *   XCB connection on NULL
 */
#define xcbt_window_conn(xw) ((xw)?(xw)->x->conn:NULL)

/**
 * Return XID of window
 *
 * Return:
 *   X window on 0
 */
#define xcbt_window_xwnd(xw) ((xw)?(xw)->wnd:(xcb_window_t)0)

/**
 * Return connection for window
 *
 * Return:
 *   X window on 0
 */
#define xcbt_window_x(xw) ((xw)?(xw)->x:NULL)


/**
 * Return screen for window
 *
 * Return:
 *   XCB connection on NULL
 */
#define xcbt_window_screen(xw) ((xw)?(xw)->screen:-1)

/**
 * Return XCB parent window
 *
 * Return:
 *   XCB connection on NULL
 */
#define xcbt_window_xprt(xw) ((xw)?(xw)->prt:(xcb_window_t)0)

/**
 * xcb_change_window_attributes wrapper
 *
 */
#define xcbt_window_change_attributes(xw, vm, vl) xcb_change_window_attributes(xcbt_window_conn(xw), xcbt_window_xwnd(xw), vm, vl)

/**
 * Set _XEMBED_INFO property, reflecting known mapping state
 */
void xcbt_window_set_xembed_info(xcbt_window pxw);

/**
 * Finalize window.
 * That always should be done explictly, the structure is not automatically destoyed with X window,
 * but X window is also destroyed during this call.
 * 
 *   X Server is asked to destroy the window, all created resources are also freed.
 * 
 * Parameters:
 *   xw - window to destroy
 */
void xcbt_window_destroy(xcbt_window xw);

/**
 * When we destroy X parent, X child windows will be destroyed in the background.
 * The process is async, so children are not yet informed about the fact from X server side
 * right after we send this request. Attempts to use not existing XID will fail.
 */
void xcbt_window_parent_destroyed(xcbt_window xw);

/**
 * Create GL window
 * 
 * Parameters:
 *   prt - parent, should not be root since only child GL windows are supported
 *   pos - position within parent
 *   gl_major, gl_minor - GL version to support (2.1, 3.0, etc.)
 *   debug - set to non zero to enable debugging
 * 
 * Return:
 *   created window or NULL
 */
xcbt_window xcbt_window_gl_create(xcbt x, xcb_window_t prt, const xcbt_rect *pos, int gl_major, int gl_minor, int debug);

/**
 * Create top level window
 * 
 * Parameters:
 *   screen - screen for this window
 *   title - title for the window (ASCII) or NULL
 *   pos - position within parent
 * 
 * Return:
 *   created window or NULL
 */
xcbt_window xcbt_window_top_create(xcbt x, int screen, const char *title, const xcbt_rect *pos);

/**
 * Create child window
 * 
 * Parameters:
 *   screen - screen for this window
 *   prt - parent window
 *   pos - position within parent
 * 
 * Return:
 *   created window or NULL
 */
xcbt_window xcbt_window_create(xcbt x, xcb_window_t prt, const xcbt_rect *pos);

/**
 * Map (show) the window)
 * 
 * Parameters:
 *   xw - window to show
 */
void xcbt_window_map(xcbt_window xw);

/**
 * Unmap (hide) the window)
 * 
 * Parameters:
 *   xw - window to show
 */
void xcbt_window_unmap(xcbt_window xw);

/**
 * Wait till the window is mapped.
 * Note: that is waiting call, potentially dangerous at the moment. 
 *   To use just after mapping for embeded windows, before reporting XID to embedder.
 */
int xcbt_window_wait_map(xcbt_window xw);

/**
 * Set the contents of the clipboard.
 * Note that if the given window is closed, the clipboard contents will be cleared.
 * @param xw XCBT window
 * @param str Clipboard text or NULL to clear the clipboard
 * @return 1 on success, 0 on failure
 */
int xcbt_clipboard_set_utf8(xcbt_window xw, const char* str);

/**
 * Returns the contents of the clipboard as a UTF-8 string,
 * or a null pointer if the contents are unavailable or in an invalid format.
 */
const char* xcbt_clipboard_get_utf8(xcbt_window xw, int* length);

/**
 * Set the cursor position relative to the window.
 * @param flags A single one of XCBT_WINDOW, XCBT_RELATIVE, XCBT_ABSOLUTE which
 *              will position the mouse relative to the top-left corner of the window, relative
 *              to its current position, or relative to the top-left corner of the screen
 *              respectively.
 */
void xcbt_move_cursor(xcbt_window xw, XCBT_MOUSE_FLAGS flag, int x, int y);

/**
 * Get the position of the window in the screen space.
 */
void xcbt_window_get_screen_pos(xcbt_window xw, xcbt_rect* pos);

/**
 * For GL window set context
 * 
 * Parameter:
 *   xw - window
 * 
 * Return:
 *   activated GL context or NULL
 */
void *xcbt_window_draw_begin(xcbt_window xw);

/**
 * Finish GL draw and show the drawing.
 * 
 * Parameter:
 *   xw - window
 * 
 * Return:
 *   non zero if draw was really stopped (was not called inside another paint)
 * 
 */
int xcbt_window_draw_end(xcbt_window xw);

/**
 * Finish GL draw, but discard the drawing.
 * 
 * Parameter:
 *   xw - window
 * 
 * Return:
 *   non zero if draw was really stopped (was not called inside another paint)
 */
int xcbt_window_draw_stop(xcbt_window xw);

/**
 * Get actual window client size
 * 
 * Parameters:
 *  pr - client size (x and y will be 0)
 */
void xcbt_window_get_client_size(xcbt_window xw, xcbt_rect *pr);

/**
 * Draw specifiend by <data> image with dimentions <w>x<h> at position <x>,<y> of the window.
 * WARNING: data should be prepared according to xcbt_img_prop for specified depth, no checkes are done here
 */
int xcbt_window_draw_img(xcbt_window xw, unsigned depth, unsigned w, unsigned h, int x, int y, unsigned data_length, uint8_t *data);

/**
 * Get screen information for specified screen
 * 
 * Return:
 *   screen information (connection live time, should not be freed) or NULL
 */
xcb_screen_t *xcbt_screen_info(xcbt px, int screen);

/**
 * Get visual type for specified visual id
 * 
 */
xcb_visualtype_t *xcbt_visual_type(xcbt px, xcb_visualid_t vid);

/**
 * Set event handler and user data, old hander and user data are returned
 * (should be preserved and called when needed).
 * 
 * Parameters:
 *   new_handler, new_data - new hander and data (data can be NULL, handler does not)
 *   old_handler, old_data - place to store current hander and data, should not be NULL
 * 
 * Return:
 *   non zero on success
 */
int xcbt_window_set_handler(xcbt_window xw, xcbt_window_handler new_handler, void *new_data, xcbt_window_handler *old_handler, void **old_data);

/**
 * Create new GC for the window
 * 
 * Parameters:
 *   as in xcb_create_gc
 * 
 * 
 * Returns:
 *   new context or 0
 */
xcb_gcontext_t xcbt_window_create_gc(xcbt_window xw, uint32_t value_mask, const uint32_t *value_list);

/**
 * Process all pending events and timers, without waiting for new events
 * 
 * Return the nearest timer or -1 (so any returned value except 0 is not a error)
 */
int xcbt_process(xcbt px);

/**
 * Event loop.
 * 
 * Parameters:
 *   exit_cond - a pointer to variable, when this variable is not zero the loop exit. When NULL, only currently pending events are processed.
 *
 */
void xcbt_event_loop(xcbt px, int *exit_cond);

/**
 *
 * User defined timer callback.
 * 
 * Parameters:
 *   id - timer id
 *   udata - given by xcbt_timer_set
 */

typedef void (*xcbt_timer_cb)(xcbt px, int timer_id, void *udata);

/**
 * Set/unset timer. Timer is removed when triggered.
 * 
 * Parameters:
 *   id  - id (positive) for the timer, -1 means remove all currently defined timers for specified window (msec is ignored)
 *   msec - time in milliseconds from now when the timer should be triggerer, when <0 remove specified by id timer
 */
void xcbt_timer_set(xcbt px, int timer_id, int msec, xcbt_timer_cb cb, void *udata);

/**
 * 
 * "Main loop" is processing events and timers. There is no general main loop on Linux, so xcbt support 3 modes of operations:
 *  * own main loop
 *      Use (blocking) xcbt_event_loop for the application live time.
 *  * integrating into external main loop using polling. 
 *      Call (non blocking) xcbt_process periodically, more then 50ms can produce visible "lag" for the user. Also clearly limit
 *      any animation speed.
 *  * integrating using (up to) 2 monitoring file descriptors and 1 (one shot) timer.
 *      xcbt_embed_set should be called, xcbt_process still can be used when desired. 
 *
 * 
 *  set_timer : should schedule timer after around specified milliseconds (no precision expected) or remove it
 *  watch     : monitor the input of specified fd, user code should support at least 2 different fds. Remove all monitoring if fd is negaive.
 *  
 *    in both cases (timer or input available) user code should call xcbt_process.
 */
typedef struct xcbt_embed_ {
  void (*dtor)(struct xcbt_embed_ *e);
  int  (*set_x)(struct xcbt_embed_ *e, xcbt x);
  int  (*set_timer)(struct xcbt_embed_ *e, int msec); // milliseconds
  int  (*watch)(struct xcbt_embed_ *e, int fd);
} xcbt_embed;


/**
 * call dtor for embed
 */
#define xcbt_embed_dtor(e) ({if(e) (e)->dtor(e);})

/**
 * Set or unset embed main loop processing. 
 * 
 * Parameters:
 *  e - embed interface to use, it should stay alive till xcbt is destroyed or its embed is replaces
 * 
 * Return zero in case of immediate errors (so the user knows the app can not work as desired) 
 */
int xcbt_embed_set(xcbt px, xcbt_embed *e);


/**
 * GLib (GDK/GTK) embedding factory
 */
xcbt_embed *xcbt_embed_glib();

/**
 * When UI thread can periodically call onIdle(), that can be used for main loop integration.
 */
xcbt_embed *xcbt_embed_idle();

/**
 * Shoud be all in onIdle.
 * WARNING: it will produce unpredictable result if pe was not created by xcbt_embed_idle.
 */
void xcbt_embed_idle_cb(xcbt_embed *pe);

#ifdef __cplusplus
};
#endif
#endif  /* __XCBT_H */
