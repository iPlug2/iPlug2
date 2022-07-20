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
 * 
 * MAYBE:
 *   glxSwapIntervalEXT
 */
#include "xcbt.h"

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xfixes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

// for dynamic loading
#include <dlfcn.h>

// glad_gl
#include <glad/glad.h>

// use GLX loaded by GLAD, linking with glx librariy is required otherwise
#define USE_GLAD_GLX 1

#ifdef USE_GLAD_GLX
#include <glad/glad_glx.h>
#else
#include <GL/glx.h>
#endif

// comment the whole thing in release
#ifdef TRACE
#undef TRACE
#endif

#ifdef _DEBUG
#define TRACE printf
#else
#define TRACE(...)
#endif

struct _xcbt_window;


/**
 * Small utility for non-intrusive list of pointers/
 */
struct _ptrlist {
  void** data;
  int    size;
};
typedef struct _ptrlist ptrlist_t;

ptrlist_t ptrlist_new()
{
  ptrlist_t pr = { NULL, 0 };
  return pr;
}

void ptrlist_free(ptrlist_t* pl)
{
  free(pl->data);
  pl->data = NULL;
  pl->size = 0;
}

void** _ptrlist_resize(ptrlist_t* pl, int new_size)
{
  if (pl->size == new_size)
  {
    return pl->size ? pl->data : NULL;
  }
  else
  {
    pl->data = realloc(pl->data, sizeof(void*) * new_size);
    pl->size = new_size;
    return pl->data;
  }
}

#define ptrlist_resize(pl, new_size) _ptrlist_resize((ptrlist_t*)(pl), (new_size))
#define ptrlist_of(_typ) struct { _typ** data; int size; }


/*
 * Private
 */
typedef struct _xcbt_timer {
  struct _xcbt_timer    *next;
  time_t tv_sec; // when this timer should be triggered (absolute)
  int    tv_msec;
  int                   id; // id (user defined) of the timer (positive)
  
  xcbt_timer_cb cb;
  void         *udata;
} _xcbt_timer;

// real xcbt structure
typedef struct {
  // this part should match struct xcbt
  xcb_connection_t *conn;       // xcb connection
  int               def_screen; // default screen
  xcb_atom_t        catoms[XCBT_COMMON_ATOMS_COUNT]; // common atoms 

  // XLib/XCB mode only
  Display *dpy; // indicate XLib/XCB combo is used (GL ready)

  // List of xcb_screen_t structures so we have access to root windows
  ptrlist_of(xcb_screen_t) screens;

  // List of xcbt windows with xcb windows behind (for event handling)
  struct _xcbt_window *windows;

  // all currently active timers (time ordered)
  _xcbt_timer         *timers;
  int                  timer_changed; // set to 1 where there are changes in the nearest timer

  xcbt_embed          *embed; // not null in embedded mode

  // Clipboard data
  char*         clipboard_data; // Clipboard data (currently UTF-8 only)
  int           clipboard_length; // clipboard data length
  xcb_window_t  clipboard_owner; // ID of window that owns clipboard data or 0 if not one of ours

  // Xlib
  void    *xlib;     // handle for libX11.so
  Display *(*XOpenDisplay)(char *dpy_name);
  int      (*XCloseDisplay)(Display *dpy);
  int      (*(*XSetErrorHandler)(int (*handler)(Display *, XErrorEvent *)))();
  int     (*xlibOldErrorHandler)(Display *, XErrorEvent *);
  
  // Xlib-xcb
  void    *xlib_xcb; // handle for libX11-xcb.so
  xcb_connection_t *(*XGetXCBConnection)(Display *dpy);
  void (*XSetEventQueueOwner)(Display *dpy, enum XEventQueueOwner owner);
  
} _xcbt;



typedef struct _xcbt_window {
  // should match .h
  _xcbt *x;
  xcb_window_t wnd;
  int screen;  
  xcb_window_t x_prt;
  xcbt_rect pos; // position inside parent
  int  mapped; // bool, logical state indicating our expectation...
  int  xmapped; // bool, driven by server status

  // internal
  struct _xcbt_window *x_next; // the list of known XCB windows, for events dispatching

  // inheritance
  void                     *udata;    // user data
  xcbt_window_handler       uhandler; // user event handler
  
  xcb_colormap_t cmap;

  int indraw; // controlled by draw_begin/stop/end_paint, to allow reentrant calls

  // for GL window
  GLXContext  ctx;
  GLXDrawable glwnd;
  
  // for all windows
  xcb_gcontext_t gc; // simple context with B/W bg/fg, created when some context is needed
  
} _xcbt_window;


/*
 * Monolitic time in uSec resolution
 * 
 * Parameters:
 *   tv - current time is returned there
 */
static void xcbt_time(time_t *tv_sec, int *tv_msec){
  if(tv_sec && tv_msec){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    *tv_sec = ts.tv_sec;
    *tv_msec = ts.tv_nsec/1000000;
  }
}

static int timespec_cmp(const struct timespec* lhs, const struct timespec* rhs)
{
  long d = lhs->tv_sec - rhs->tv_sec;
  if (d != 0)
    return (int)d;
  d = lhs->tv_nsec - rhs->tv_nsec;
  return (int)d;
}

/*
 * Manipulate timers
 * 
 * Parameters:
 *   pxw - window for which timer should be set
 *   id  - id of the timer, should be positive, -1 means remove all currently defined timers for specified window (usec is ignored)
 *   usec - time in uSec from now when the timer should be triggerer, when <0 remove specified timer(s)
 */
void xcbt_timer_set(xcbt px, int timer_id, int msec, xcbt_timer_cb cb, void *udata){
  _xcbt *x = (_xcbt *)px;
  _xcbt_timer *t, **pt;
  if(x){
    if(timer_id > 0){ // one timer
      for(pt = &x->timers; *pt && (timer_id != (*pt)->id); pt = &(*pt)->next);
      if((t = *pt)){
        // remove it first
        if(x->timers == t)
          x->timer_changed = 1;
        *pt = t->next;
      }
      if(msec < 0) { // remove
        if(t){
          free(t);
        }
      } else { // add/change
        if(!t){
          t = (_xcbt_timer *)malloc(sizeof(*t));
          t->next = NULL;
          t->id = timer_id;
        }
        if(t){
          t->cb = cb;
          t->udata = udata;
          xcbt_time(&t->tv_sec, &t->tv_msec);
          t->tv_msec += msec;
          if(t->tv_msec >= 1000){ // normilize
            t->tv_sec += t->tv_msec / 1000;
            t->tv_msec %= 1000;
          }
          // ordered add timer 
          for(pt = &x->timers; 
              *pt && (((*pt)->tv_sec < t->tv_sec) || (((*pt)->tv_sec == t->tv_sec) && ((*pt)->tv_msec < t->tv_msec)));
              pt = &(*pt)->next);
          t->next = *pt;
          *pt = t;
          if(x->timers == t)
            x->timer_changed = 1;
        }
      }
    } else if(timer_id < 0){ // remove all timers
      while((t = x->timers)){
        x->timers = t->next;
        free(t);
        x->timer_changed = 1;
      }
    }
  }      
}  

/*
 * GL/GLX use global function pointers.
 * So the whole thing works in case there is just one set of GL libraries required
 * 
 * In addition GLAD dlclose used libraries, that can lead to real unloading in case
 * they are not referenced. So just check we have tried to load it already.
 * That introduce reference "leak", so GL/GLX will not be unloaded till the program exit.
 * But I have no better idea at the moment.
 * 
 * NOT THREAD SAFE.
 */
#ifndef USE_GLAD_GLX
  #ifndef GLX_CONTEXT_MAJOR_VERSION_ARB
    #define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
    #define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
  #endif
  typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
  static glXCreateContextAttribsARBProc glXCreateContextAttribsARB;
#endif
static int xcbt_glad_glx_load(_xcbt *x){
  static int tried = 0; // have we tried already?
  static int loaded = 0; // could it be loaded?
  if(!tried){
    tried = 1;
#ifdef USE_GLAD_GLX
    if(x && x->dpy){
      if(!dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL))
        dlopen("libGL.so", RTLD_NOW | RTLD_GLOBAL);
      loaded = gladLoadGLX(x->dpy, x->def_screen) ? 1 : 0;
#ifdef _DEBUG
      if(loaded){
        int major, minor;
        if(glXQueryVersion && glXQueryVersion(x->dpy, &major, &minor)){
          TRACE("GLX v%d.%d\n", major, minor);
        }
      }
#endif
    }
#else
    glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
      glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" ); // failures are not critical
    loaded = 1;
#endif
  }
  return loaded;
}

/*
 * GL can be loaded only after GLContext is created and current.
 */
static int xcbt_glad_gl_load(_xcbt *x){
  static int tried = 0; // have we tried already?
  static int loaded = 0; // could it be loaded?
  if(!tried){
    tried = 1;
    if(x && x->dpy){
      loaded = gladLoadGL() ? 1 : 0;
    }
  }
  return loaded;
}


/*
 * Find and return screen number for xcb <window>.
 * Returns -1 in case of errors, the <window> is not found or its screen is not known
 */
static int32_t xcbt_xcb_window_screen(_xcbt *x, xcb_window_t window){
  if(x && window){
    xcb_query_tree_reply_t *reply = xcb_query_tree_reply(x->conn, xcb_query_tree(x->conn, window), NULL);
    if(reply){
      xcb_window_t root = reply->root;
      free(reply);
      const xcb_setup_t *setup = xcb_get_setup(x->conn);
      int32_t i, screen_count = xcb_setup_roots_length(setup);
      xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
      for(i = 0; i < screen_count; ++i){
        if(iter.data->root == root){
          return i;
        }
        xcb_screen_next(&iter);
      }
    }
  }
  return -1;
}

void xcbt_disconnect(xcbt px){
  _xcbt *x = (_xcbt *)px;
  if(x){
    xcbt_embed_set(px, NULL);
    if(!xcb_connection_has_error(x->conn))
      xcbt_sync(px);  // TODO: wait till all known windows get XCB_DESTROY_NOTIFY }
    if(x->dpy){
      x->XCloseDisplay(x->dpy);
      x->XSetErrorHandler(x->xlibOldErrorHandler); // warning... it is global...
    } else {
      //TRACE("XCBT %p is Disconnected\n", x->conn);
      xcb_disconnect(x->conn);
    }
    if(x->xlib){
      dlclose(x->xlib);
    }
    if(x->xlib_xcb){
      dlclose(x->xlib_xcb);
    }
    while(x->timers){
      _xcbt_timer *t = x->timers;
      x->timers = t->next;
      free(t);
    }
    memset(x, 0, sizeof(*x));
    free(x);
  }
}

int xcbt_sync(xcbt px){
  _xcbt *x = (_xcbt *)px;
  if(x){
    xcbt_flush(x);
    // as suggsted by NEWS, without xcb_aux...
    xcb_get_input_focus_reply_t *fr = xcb_get_input_focus_reply(x->conn, xcb_get_input_focus(x->conn), NULL);
    if(fr){
      free(fr);
      return 1;
    }
  }
  return 0;
}

void xcbt_sync_dbg(xcbt px){
  xcbt_sync(px);
  for(int i =0; i<100; ++i){
    xcbt_process(px);
    usleep(1000);
  }
}

/*
 * Ignore XLib errors for now
 */
static int xcbt_XErrorHandler(Display *dpy, XErrorEvent *ev ){
    TRACE("XLib error\n");
    return 0;
}


/*
 * Load some common atoms
 */
static void xcbt_load_atoms(_xcbt *x){
  static const char *common_atom_names[XCBT_COMMON_ATOMS_COUNT] = {
    "WM_PROTOCOLS",
    "WM_DELETE_WINDOW",
    "_XEMBED_INFO",
    "CLIPBOARD",
    "UTF8_STRING",
    "XSEL_DATA",
    "STRING",
    "TEXT",
    "TARGETS",
  };
  if(x && !x->catoms[0]){
    xcb_intern_atom_cookie_t ck[XCBT_COMMON_ATOMS_COUNT];
    xcb_intern_atom_reply_t *ar;
    int i;
    //TRACE("Loading atoms\n");
    for(i = 0; i < XCBT_COMMON_ATOMS_COUNT; ++i){
      ck[i] = xcb_intern_atom(x->conn, 0, strlen(common_atom_names[i]), common_atom_names[i]);
    }
    for(i = 0; i < XCBT_COMMON_ATOMS_COUNT; ++i){
      ar = xcb_intern_atom_reply(x->conn, ck[i], NULL);
      if(ar){
        x->catoms[i] = ar->atom;
        //TRACE(" [%d] %s: 0x%x\n", i, common_atom_names[i], ar->atom);
      } else {
        TRACE("ERROR: could not get atom '%s'\n", common_atom_names[i]);
        x->catoms[i] = 0;
      }
    }
  }
}

/**
 * Initialize the xcb connection regardless of if it's Xlib or xcb.
 */
static int xcbt_connect_init(_xcbt *x, uint32_t flags)
{
  if ((flags & XCBT_INIT_ATOMS))
    xcbt_load_atoms(x);
  
  // Iterate through screens
  {
    xcb_screen_iterator_t iter;
    int size = 0;

    iter = xcb_setup_roots_iterator(xcb_get_setup(x->conn));
    // Default capacity to 8
    ptrlist_resize(&x->screens, 8);
    for (; iter.rem; size++, xcb_screen_next(&iter))
    {
      x->screens.data[size] = iter.data;
      if (size + 1 == x->screens.size)
      {
        ptrlist_resize(&x->screens, x->screens.size * 2);
      }
    }
    // Don't bother changing the capacity, just set the size
    x->screens.size = size;
  }

  // Load xfixes extension
  {
    xcb_xfixes_query_version_cookie_t cookie = xcb_xfixes_query_version(x->conn, 4, 0);
    xcbt_flush(x);
    xcb_generic_error_t* err;
    xcb_xfixes_query_version_reply_t* reply = xcb_xfixes_query_version_reply(x->conn, cookie, &err);
    if (!reply)
    {
      printf("Unable to load xfixes extension: codes %d,%d\n", err->major_code, err->minor_code);
      free(err);
      return 0;
    }
    free(reply);
  }

  return 1;
}

xcbt xcbt_connect(uint32_t flags){
  _xcbt *x = (_xcbt *)calloc(1, sizeof(_xcbt));

  // Initialize fields
  x->clipboard_data = NULL;
  x->clipboard_length = 0;
  x->clipboard_owner = 0;

  if(x){
    if(flags & XCBT_USE_GL){
      dlerror(); // clear errors
      if(!(x->xlib = dlopen("libX11.so.6", RTLD_NOW | RTLD_GLOBAL)))
        x->xlib = dlopen("libX11.so", RTLD_NOW | RTLD_GLOBAL);
      if(!(x->xlib_xcb = dlopen("libX11-xcb.so.1", RTLD_NOW | RTLD_GLOBAL)))
        x->xlib_xcb = dlopen("libX11-xcb.so", RTLD_NOW | RTLD_GLOBAL);
      if(x->xlib && x->xlib_xcb){
        if((*((void **)&x->XOpenDisplay) = dlsym(x->xlib, "XOpenDisplay")) &&
          (*((void **)&x->XCloseDisplay) = dlsym(x->xlib, "XCloseDisplay")) &&
          (*((void **)&x->XSetErrorHandler) = dlsym(x->xlib, "XSetErrorHandler")) &&
          (*((void **)&x->XGetXCBConnection) = dlsym(x->xlib_xcb, "XGetXCBConnection")) &&
          (*((void **)&x->XSetEventQueueOwner) = dlsym(x->xlib_xcb, "XSetEventQueueOwner"))
          ){
          if((x->dpy = x->XOpenDisplay(NULL))){
            x->xlibOldErrorHandler = x->XSetErrorHandler(&xcbt_XErrorHandler); // warning... it is global...
            x->def_screen = DefaultScreen(x->dpy);
            if((x->conn = x->XGetXCBConnection(x->dpy))){
              if(xcbt_glad_glx_load(x)){
                if (xcbt_connect_init(x, flags)) {
                  TRACE("INFO: Xlib/XCB FD: %d\n", xcb_get_file_descriptor(x->conn));
                  return (xcbt)x;
                } else {
                  TRACE("XCBT setup failed\n");
                }
              } else {
                TRACE("Could not load GLX\n");
              }
            } else {
              TRACE("Could not get XCB connection for X display\n");
            }
          } else {
            TRACE("Could not open X display\n");
          }
        } else {
          TRACE("ERROR: can not find required libX11 function(s)\n");
        }
      } else {
        TRACE("ERROR: can not load libX11/libX11-xcb: %s\n", dlerror());
      }
    }
    if(x->dpy){
      x->XCloseDisplay(x->dpy);
      x->dpy = NULL; // close XLib display, even if we manage to open it
    }
    if((x->conn = xcb_connect(NULL, &x->def_screen)) && (x->def_screen >= 0)){
      TRACE("INFO: XCB only\n");
      xcbt_connect_init(x, flags);
      //TRACE("XCBT %p is Connected\n", x->conn);
      return (xcbt)x;
    } else {
      TRACE("Could not open X connection\n");
    }
  }
  xcbt_disconnect((xcbt)x);
  return NULL;
};

/* For line alignment (so unsigned):
 *    generic is ((x + (r - 1)) / r) * r;
 *    power of 2 is (x + r - 1) & -r;
 *     masked (m = r - 1) power of 2 is (x + m) & ~m;
 */

int xcbt_get_img_prop(xcbt px, unsigned depth, xcbt_img_prop *prop){
  _xcbt *x = (_xcbt *)px;
  if(x && prop){
    const xcb_setup_t *setup = xcb_get_setup(x->conn);
    if(setup){
      xcb_format_iterator_t it = xcb_setup_pixmap_formats_iterator(setup);
      while(it.rem){
        if(it.data->depth == depth){
          prop->depth = depth;
          prop->bpp = it.data->bits_per_pixel;
          prop->line_align = it.data->scanline_pad;
          prop->byte_order = setup->image_byte_order;
          return 1;
        }
        xcb_format_next(&it);
      }
    }
  }
  return 0;
}


/**
 * Choose GLX FB Config (for GL window)
 * 
 * At the moment it searchs best MSAA.
 * 
 * Parameters:
 *   fbc_best - where should we store the index of best config
 * 
 * Returns:
 *   array of all supported configs, should be freed
 * 
 * Note: I am not sure list of configs can be freed before using one of configs. So I return the list (which should be freed) and
 *       the index of proposed config to use
 */
static GLXFBConfig *xcbt_window_gl_choose_fbconfig(_xcbt_window *xw, int *fbc_best){
  static int attr[] = {
    GLX_X_RENDERABLE    , True,             // we want X able draw here
    GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,   // we are going to draw a window
    GLX_RENDER_TYPE     , GLX_RGBA_BIT,     // RGBA 8bit per color/alpha
    GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
    GLX_RED_SIZE        , 8,
    GLX_GREEN_SIZE      , 8,
    GLX_BLUE_SIZE       , 8,
    GLX_ALPHA_SIZE      , 8,
    GLX_DEPTH_SIZE      , 24,
    GLX_STENCIL_SIZE    , 8,                // in case we want use stencil (requirement for NanoVG)
    GLX_DOUBLEBUFFER    , True,             // we want double buffer
    //GLX_SAMPLE_BUFFERS  , 1,              // that is nice to have, but not critical. GLX 1.3 could have GLX_ARB_MULTISAMPLE, otherwise had no such attributes.
    //GLX_SAMPLES         , 4,
    None
  };
  
  GLXFBConfig *fbc;
  Display *dpy;
  int          fbc_count, i , sb, samples, samples_best = -1;
#ifdef USE_GLAD_GLX
  if(!glXChooseFBConfig || !glXGetFBConfigAttrib){
    TRACE("GLX does not have required API\n");
    return NULL;
  }
#endif
  if(xw && (dpy = xw->x->dpy) && fbc_best && (fbc = glXChooseFBConfig(dpy, xw->screen, attr, &fbc_count))){
    TRACE("Found %d usable FB configs, choosing best for MSAA\n", fbc_count);
    *fbc_best = -1;
    for(i=0; i < fbc_count; ++i){
      if(glXGetFBConfigAttrib(dpy, fbc[i], GLX_SAMPLE_BUFFERS, &sb) == Success){
        if(!sb || (glXGetFBConfigAttrib(dpy, fbc[i], GLX_SAMPLES, &samples) != Success))
          samples = 0;
        if(*fbc_best < 0 || (sb && samples > samples_best)){
          *fbc_best = i;
          samples_best = samples;
        }
      }
    }
    if(*fbc_best <= 0){
      *fbc_best = 0;
      TRACE("  Using FB Config without MSAA\n");
    } else {
      TRACE("  Using FB Config with %d samples\n", samples_best);
    }
    return fbc;
  } else {
    TRACE("Not reasonable FB Configs found\n");
  }
  return NULL;
}

/***
 * Create GL context for window
 * 
 * Parameters:
 *   fbc - GLX FB Config to use
 *   major_version, minor_version - version of GL
 *   debug - set to non zero to enable debugging
 * 
 * Return:
 *   non zero when the context could be created
 * 
 * It is a bit unclear with versions for me. It seems like latest version (3.2+) can be returned without ARB,
 * but I have seen comments that for 2.0 it is better use ARB and request 1.0 to avoid compatibility issues. A bit confusing.
 */
static int xcbt_window_gl_create_context(_xcbt_window *xw, GLXFBConfig fbc, int major_version, int minor_version, int debug){
  int attr[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, major_version,
    GLX_CONTEXT_MINOR_VERSION_ARB, minor_version,
    GLX_CONTEXT_FLAGS_ARB, /* GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB */ None,
    None

    // GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB | GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB, for 3.x+
  };
  Display *dpy;
  if(!xw || !(dpy = xw->x->dpy) || !fbc || !major_version){
    return 0;
  }
  if(major_version > 2 || (major_version == 2 && minor_version >= 0)){
    // per GLX_ARB_create_context.txt section 3.3.7.1
    if(glXCreateContextAttribsARB){
      if(debug)
        attr[5] |= GLX_CONTEXT_DEBUG_BIT_ARB;
      else
        attr[4] = None;
      /** can be used to get build-in error messages... and may be more optimized execution ;)
      if(major_version >= 3)
        attr[5] |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
        */
      xw->ctx = glXCreateContextAttribsARB(dpy, fbc, 0, True, attr);
      if(xw->ctx){
        TRACE("GLX: ARB context creation succeeded for %d.%d\n", major_version, minor_version);
        return 1;
      } else {
        TRACE("GLX: ARB context creation failed\n");
      }
    }
  }
  // That can work for 3.2+
  xw->ctx = glXCreateNewContext(dpy, fbc, GLX_RGBA_TYPE, 0, True);
  if(xw->ctx){
    TRACE("GLX: Legacy context creation succeeded\n");
    return 1;
  }
  return 0;
}


/**
 * Register in xcbt
 */
static void xcbt_window_register(_xcbt_window *xw){
  if(xw && xw->wnd){
    _xcbt *x = xw->x;
    _xcbt_window **windows = &x->windows;
    while(*windows && (*windows != xw)){
      windows = &(*windows)->x_next;
    }
    if(*windows){
      TRACE("BUG: attempt to register already registered window\n");
      return;
    }
    xw->x_next = x->windows;
    x->windows = xw;
  }
}

/**
 * Unregister in xcbt
 */
static void xcbt_window_unregister(_xcbt_window *xw){
  if(xw){ // note that wnd can already be reset
    _xcbt *x = xw->x;
    _xcbt_window **windows = &x->windows;
    while(*windows && (*windows != xw)){
      windows = &((*windows)->x_next);
    }
    if(!*windows){
      TRACE("BUG: attempt to unregister unknown window\n");
      return;
    }
    *windows = xw->x_next;
    xw->x_next = NULL;
  }
}

void xcbt_window_destroy(xcbt_window pxw){
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(xw){
    Display *dpy = xw->x->dpy;
    xcb_connection_t *conn = xw->x->conn;
    if(dpy){
      if(xw->ctx){
        if(glXGetCurrentContext() == xw->ctx){
          /* TODO: If we use it in other thread, that is going to be the smallest resulting problem...
           * the whole thing I can not reset it at the end of painting for not yet understood reason.
           */
          if(!glXMakeContextCurrent(xw->x->dpy, None, None, NULL)){
            TRACE("XCBT:BUG: reseting context does not work\n");
          }
        }
        glXDestroyContext(dpy, xw->ctx);
        xw->ctx = NULL;
      }
      if(xw->glwnd){
        glXDestroyWindow(dpy, xw->glwnd);
        xw->glwnd = 0;
      }
    }
    if(xw->wnd){
      xcb_destroy_window(conn, xw->wnd);
      xw->wnd = 0;
    }
    if(xw->cmap){
      xcb_free_colormap(conn, xw->cmap);
      xw->cmap = 0;
    }
    if(xw->gc){
      xcb_free_gc(conn, xw->gc);
      xw->gc = 0;
    }
    xcbt_window_unregister(xw);
    xw->uhandler(pxw, NULL, xw->udata);
    memset(xw, 0, sizeof(*xw));
    free(xw);
  }
}

void xcbt_window_parent_destroyed(xcbt_window pxw){
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(xw){
    xw->wnd = 0;
    xw->glwnd = 0; // from expirience, it is also destroyed
    xw->mapped = 0;
  }
}

static void xcbt_window_default_handler(_xcbt_window *xw, xcb_generic_event_t *evt, void *unused);

xcbt_window xcbt_window_gl_create(xcbt px, xcb_window_t prt, const xcbt_rect *pos, int gl_major, int gl_minor, int debug){
  _xcbt *x = (_xcbt *)px;
  _xcbt_window *xw;
  Display *dpy;
  GLXFBConfig *fbcs = NULL;
  if(!x || !(dpy = x->dpy) || !prt || !pos || (pos->w <= 0) || (pos->h <= 0)){
    return NULL;
  }
  if((xw = (_xcbt_window *)calloc(1, sizeof(*xw)))){
    xw->x = x;
    xw->uhandler = (xcbt_window_handler)xcbt_window_default_handler;
    xw->x_prt  = prt;
    memcpy(&xw->pos, pos, sizeof(xcbt_rect));
    xw->screen = xcbt_xcb_window_screen(x, prt);
    int fbc_idx;
    if((xw->screen >= 0) && (fbcs = xcbt_window_gl_choose_fbconfig(xw, &fbc_idx))){
      XID vid;
      GLXFBConfig fbc = fbcs[fbc_idx];
      if(glXGetFBConfigAttrib(dpy, fbc, GLX_VISUAL_ID, (int *)&vid) == Success){
        TRACE("Choosen visual: 0x%x\n", (unsigned)vid);
        if(xcbt_window_gl_create_context(xw, fbc, gl_major, gl_minor, debug)){
          uint32_t eventmask = 
                  XCB_EVENT_MASK_EXPOSURE | // we want to know when we need to redraw
                  XCB_EVENT_MASK_STRUCTURE_NOTIFY | // get varius notification messages like configure, reparent, etc.
                  XCB_EVENT_MASK_PROPERTY_CHANGE | // useful when something will change our property
                  XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE  |  // mouse clicks
                  XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE  |      // keyboard is questionable according to XEMBED
                  XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW |   // mouse entering/leaving
                  XCB_EVENT_MASK_POINTER_MOTION // mouse motion
                  ;
          uint32_t wa[] = { eventmask, 0, 0 };
          xw->wnd = xcb_generate_id(x->conn);
          wa[1] = xw->cmap = xcb_generate_id(x->conn);
          xcb_create_colormap(x->conn, XCB_COLORMAP_ALLOC_NONE, xw->cmap, prt, vid);
          xcb_create_window(x->conn, XCB_COPY_FROM_PARENT, xw->wnd, prt, pos->x, pos->y, pos->w, pos->h, 0,
                            XCB_WINDOW_CLASS_INPUT_OUTPUT, vid, XCB_CW_EVENT_MASK | XCB_CW_COLORMAP, wa);
          // note that at this moment server has no idea we have created the window...
          xw->glwnd = glXCreateWindow(dpy, fbc, xw->wnd, NULL);
          if(xw->glwnd){
            TRACE("GL window 0x%x is created\n", xw->wnd);
            free(fbcs);
            xcbt_window_register(xw);
            return (xcbt_window)xw;
          }
          TRACE("Could not create GL window\n");
        }
      } else {
        TRACE("BUG: best FB config has no Visual\n");
      }
    }
  }
  if(fbcs)
    free(fbcs);
  xcbt_window_destroy((xcbt_window)xw);
  return NULL;
}

xcbt_window xcbt_window_top_create(xcbt px, int screen, const char *title, const xcbt_rect *pos){
  uint32_t eventmask = 
    XCB_EVENT_MASK_EXPOSURE | // we want to know when we need to redraw
    XCB_EVENT_MASK_STRUCTURE_NOTIFY | // get varius notification messages like configure, reparent, etc.
    XCB_EVENT_MASK_PROPERTY_CHANGE | // useful when something will change our property
    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE  |  // mouse clicks
    XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE  |      // keyboard is questionable accordung to XEMBED
    XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW |   // mouse entering/leaving
    XCB_EVENT_MASK_POINTER_MOTION // mouse motion
    ;
  uint32_t wa[] = { eventmask, 0, 0 };
  _xcbt *x = (_xcbt *)px;
  _xcbt_window *xw;
  xcb_screen_t *si;
  xcb_atom_t wm_protocols[1];
  if(!x || (screen < 0) || !pos || (pos->w <= 0) || (pos->h <= 0) || !(si = xcbt_screen_info(px, screen))){
    return NULL;
  }
  xcbt_load_atoms(x); // make sure WM atoms are loaded for top windows
  if((xw = (_xcbt_window *)calloc(1, sizeof(*xw)))){
    xw->x = x;
    xw->uhandler = (xcbt_window_handler)xcbt_window_default_handler;
    xw->x_prt  = si->root;
    memcpy(&xw->pos, pos, sizeof(xcbt_rect));
    xw->screen = screen;
    xw->wnd = xcb_generate_id(x->conn);
    xcb_create_window(x->conn, XCB_COPY_FROM_PARENT, xw->wnd, xw->x_prt, pos->x, pos->y, pos->w, pos->h, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, si->root_visual, XCB_CW_EVENT_MASK, wa);
    xcbt_window_register(xw);

    // WM hints    
    if(title){
      xcb_change_property(x->conn, XCB_PROP_MODE_REPLACE, xw->wnd, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, strlen(title), title);
      xcb_change_property(x->conn, XCB_PROP_MODE_REPLACE, xw->wnd, XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8, strlen(title), title);
    }
    // TODO: WM_TAKE_FOCUS and _NET staff
    wm_protocols[0] = XCBT_WM_DELETE_WINDOW(x);
    xcb_change_property(x->conn, XCB_PROP_MODE_REPLACE, xw->wnd, XCBT_WM_PROTOCOLS(x), XCB_ATOM_ATOM, 32, 1, wm_protocols);

    return (xcbt_window)xw;
  }
  return NULL;
}


xcbt_window xcbt_window_create(xcbt px, xcb_window_t prt, const xcbt_rect *pos){
  _xcbt *x = (_xcbt *)px;
  _xcbt_window *xw;
  xcb_screen_t *si;
  if(!x || !prt || !pos || (pos->w <= 0) || (pos->h <= 0)){
    return NULL;
  }
  if((xw = (_xcbt_window *)calloc(1, sizeof(*xw)))){
    xw->x = x;
    xw->uhandler = (xcbt_window_handler)xcbt_window_default_handler;
    xw->x_prt  = prt;
    memcpy(&xw->pos, pos, sizeof(xcbt_rect));
    xw->screen = xcbt_xcb_window_screen(x, prt);
    if((xw->screen >= 0) && (si = xcbt_screen_info(px, xw->screen))){
      uint32_t eventmask = 
                  XCB_EVENT_MASK_EXPOSURE | // we want to know when we need to redraw
                  XCB_EVENT_MASK_STRUCTURE_NOTIFY | // get varius notification messages like configure, reparent, etc.
                  XCB_EVENT_MASK_PROPERTY_CHANGE | // useful when something will change our property
                  XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE  |  // mouse clicks
                  XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE  |      // keyboard is questionable accordung to XEMBED
                  XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW |   // mouse entering/leaving
                  XCB_EVENT_MASK_POINTER_MOTION // mouse motion
                  ;
      uint32_t wa[] = { eventmask, 0, 0 };
      xcb_visualtype_t *vt = xcbt_visual_type(px, si->root_visual);
      if(vt && (vt->_class == XCB_VISUAL_CLASS_TRUE_COLOR) && (vt->red_mask == 0xff0000) && (vt->green_mask == 0xff00) && (vt->blue_mask == 0xff)){
        xw->wnd = xcb_generate_id(x->conn);
        wa[1] = xw->cmap = xcb_generate_id(x->conn); // it works without, but at least not in REAPER when switching effects...
        xcb_create_colormap(x->conn, XCB_COLORMAP_ALLOC_NONE, xw->cmap, prt, si->root_visual);
        xcb_create_window(x->conn, XCB_COPY_FROM_PARENT, xw->wnd, prt , pos->x, pos->y, pos->w, pos->h, 0,
                          XCB_WINDOW_CLASS_INPUT_OUTPUT, si->root_visual, XCB_CW_EVENT_MASK | XCB_CW_COLORMAP, wa);
        xcbt_window_register(xw);
        TRACE("INFO: A window is created with 24bit RGB visual\n");
        return (xcbt_window)xw;
      } else {
        TRACE("NOT IMPLEMENTED: screen visual is not 24bit RGB, not supported\n");
      }
    } else {
      TRACE("ERROR: problems with suggested parent window 0x%x\n", prt);
    } 
  }
  xcbt_window_destroy((xcbt_window)xw);
  return NULL;
}


void xcbt_window_set_xembed_info(xcbt_window pxw){
  uint32_t info[] = { 0, 0 }; // version 0, not mapped
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(xw){
    xcbt_load_atoms(xw->x); // make sure XEMBED atoms are loaded
    if(xw->mapped){
      info[1] = 1; // XEMBED_MAPPED
    }
    xcb_change_property(xw->x->conn, XCB_PROP_MODE_REPLACE, xw->wnd, XCBT_XEMBED_INFO(xw->x), XCBT_XEMBED_INFO(xw->x), 32, 2, info);
  }
}

void xcbt_window_map(xcbt_window pxw){
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(xw && !xw->mapped){
    if(!xw->xmapped)
      xcb_map_window(xw->x->conn, xw->wnd);
    xw->mapped = 1;
  }
}

void xcbt_window_unmap(xcbt_window pxw){
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(xw && xw->mapped){
    if(xw->xmapped)
      xcb_unmap_window(xw->x->conn, xw->wnd);
    xw->mapped = 0;
  }
}

void *xcbt_window_draw_begin(xcbt_window pxw){
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(xw){    
    if(xw->ctx){
      if(++xw->indraw > 1){
        GLXContext old = glXGetCurrentContext();
        if(old && (old != xw->ctx)){
          TRACE("XCBT:BUG: reentrant draw, but different GL context\n");
        }
        return old; // can be NULL if gl could not be loaded, not a separate error
      }
      glXMakeContextCurrent(xw->x->dpy, xw->glwnd, xw->glwnd, xw->ctx);
      if(xcbt_glad_gl_load(xw->x))
        return xw->ctx;
      glXMakeContextCurrent(xw->x->dpy, None, None, NULL);
      TRACE("XCBT: could not set context\n");
      return NULL;
    }
    // TODO: not GL window context?
    ++xw->indraw;
    return pxw;
  }
  TRACE("Not exist\n");
  return NULL;
}

int xcbt_window_draw_end(xcbt_window pxw){
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(xw){
    if(xw->indraw){
      if(--xw->indraw)
        return 0;
      if(xw->ctx){
        glXSwapBuffers(xw->x->dpy, xw->glwnd);
        //glXSwapBuffers(xw->x->dpy, xw->wnd);
        /* TODO: WARNING:
         *   The following does not produce an error or visible bugs, but mouse events "disappear".
         *   That can be related to Interval... usleep(200000) before draw_end "eat" events completely,
         *   but only in case this call is made.
         * 
         *   glFinish(), sync, usleep DO NOT HELP
        if(!glXMakeContextCurrent(xw->x->dpy, None, None, NULL)){
          TRACE("XCBT:BUG: reseting context does not work\n");
        }
        */
      } else {
        xcb_flush(xw->x->conn);
      }
    } else {
      TRACE("XCBT:BUG: draw end without begin\n");
    }
  }
  return 1;
}

int xcbt_window_draw_stop(xcbt_window pxw){
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(xw){
    if(xw->indraw){
      if(--xw->indraw)
        return 0;
      if(xw->ctx){
        /* see comments in draw_end
        glXMakeContextCurrent(xw->x->dpy, None, None, NULL);
        */
      }
    } else {
      TRACE("XCBT:BUG: draw end without begin\n");
    }
  }
  return 1;
}

void xcbt_window_get_client_size(xcbt_window pxw, xcbt_rect *pr){
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(pr){
    pr->x = pr->y = 0;
    if(xw){
      pr->w = xw->pos.w;
      pr->h = xw->pos.h;
    }
  }
}

xcb_screen_t *xcbt_screen_info(xcbt px, int screen){
  _xcbt *x = (_xcbt *)px;
  if(x){
    const xcb_setup_t *setup = xcb_get_setup(x->conn);
    int i;
    if(setup && (xcb_setup_roots_length(setup) > screen)){
      xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
      for(i = 0; i < screen; ++i){
        xcb_screen_next(&iter);
      }
      return iter.data;
    }
  }
  return NULL;
}

xcb_visualtype_t *xcbt_visual_type(xcbt px, xcb_visualid_t vid){
  _xcbt *x = (_xcbt *)px;
  if(x){
    xcb_screen_iterator_t sit = xcb_setup_roots_iterator(xcb_get_setup(x->conn));
    for(; sit.rem; xcb_screen_next(&sit)){
      xcb_depth_iterator_t dit = xcb_screen_allowed_depths_iterator(sit.data);
      for(; dit.rem; xcb_depth_next(&dit)){
        xcb_visualtype_iterator_t vit = xcb_depth_visuals_iterator(dit.data);
        for(; vit.rem; xcb_visualtype_next(&vit)){
          if(vid == vit.data->visual_id){
            return vit.data;
          }
        }
      }
    }
  }
  return NULL;
}

int xcbt_window_set_handler(xcbt_window pxw, xcbt_window_handler new_handler, void *new_data, xcbt_window_handler *old_handler, void **old_data){
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(xw && new_handler && old_handler && old_data){
    *old_handler = xw->uhandler;
    *old_data = xw->udata;
    xw->uhandler = new_handler;
    xw->udata = new_data;
    return 1;
  } else {
    TRACE("BUG: bad parameters for window_set_handler\n");
  }
  return 0;
}

int xcbt_window_wait_map(xcbt_window pxw){
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(xw){
    if(!xw->mapped)
      return 0; // hm... we have not requested mapping
    if(xw->xmapped){
      TRACE("XCBT:MBB: instantly mapped...\n");
      return 1; // already mapped
    }
    xcbt_event_loop(xcbt_window_x(pxw), &xw->xmapped); // TODO: dangerous in case connection broke, window can not be mapped, etc.
  }
  return xw->xmapped;
}

int xcbt_clipboard_set_utf8(xcbt_window pxw, const char* str)
{
  _xcbt_window *xw = (_xcbt_window*) pxw;
  _xcbt *x = xw->x;
/*
  This probably needs to be done on a root window, not embedded.

  xcb_void_cookie_t cookie1 = xcb_set_selection_owner_checked(
      x->conn, xw->wnd, XCBT_ATOM_CLIPBOARD(x), XCB_CURRENT_TIME);
  xcb_void_cookie_t cookie2 = xcb_set_selection_owner_checked(
      x->conn, xw->wnd, XCB_ATOM_PRIMARY, XCB_CURRENT_TIME);
  xcbt_flush(x);
  xcb_generic_error_t* err1 = xcb_request_check(x->conn, cookie1);
  xcb_generic_error_t* err2 = xcb_request_check(x->conn, cookie2);
  int ok = !err1 && !err2;
  if (ok)
  {
    // Data length includes terminating null
    x->clipboard_length = strlen(str) + 1;
    x->clipboard_data = calloc(1, x->clipboard_length);
    memcpy(x->clipboard_data, str, x->clipboard_length);
    x->clipboard_owner = xw->wnd;
  }
  free(err1);
  free(err2);
  return ok;
  */

  // For now we implement this using xclip.
  FILE* fd = popen("xclip -i -selection c", "w");
  if (fd)
  {
    fwrite(str, 1, strlen(str), fd);
    pclose(fd);
    return 1;
  }
  else
  {
    return 0;
  }
}

const char* xcbt_clipboard_get_utf8(xcbt_window pxw, int* length)
{
  _xcbt_window *xw = (_xcbt_window*) pxw;
  _xcbt *x = xw->x;

  // For some reason we don't always receive XCB_SELECTION_CLEAR events
  // so assume we need to request the clipboard content every time.
  // if (x->clipboard_owner != 0)
  // {
  //   _xcbt_window **windows = &x->windows;
  //   while(*windows && ((*windows)->wnd != x->clipboard_owner)){
  //     windows = &(*windows)->x_next;
  //   }
  //   // If we found the clipboard owner, then just return the clipboard data
  //   if (*windows)
  //   {
  //     *length = x->clipboard_length;
  //     return x->clipboard_data;
  //   }
  // }

  // Either we don't own the window with the clipboard, or we don't know who does
  x->clipboard_owner = 0;
  x->clipboard_length = 0;
  free(x->clipboard_data);
  x->clipboard_data = NULL;

  xcb_convert_selection(x->conn, xw->wnd,
      XCBT_ATOM_CLIPBOARD(x), XCBT_ATOM_UTF8_STRING(x), XCBT_ATOM_CLIPBOARD(x), XCB_CURRENT_TIME);
  xcbt_flush(x);
  
  struct timespec now;
  struct timespec until;
  clock_gettime(CLOCK_MONOTONIC_RAW, &now);
  until = now;
  // Default timeout is 1 second
  until.tv_sec += 1;

  // We have a timeout because getting the clipboard might fail.
  // https://jtanx.github.io/2016/08/19/a-cross-platform-clipboard-library/#linux
  while (x->clipboard_length == 0 && timespec_cmp(&now, &until) < 0)
  {
    xcbt_process((xcbt)x);
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
  }
  if (x->clipboard_length > 0)
  {
    *length = x->clipboard_length;
    return x->clipboard_data;
  }
  else
  {
    *length = 0;
    return NULL;
  }
}

void xcbt_move_cursor(xcbt_window pxw, XCBT_MOUSE_FLAGS flag, int cx, int cy)
{
  _xcbt_window* xw = (_xcbt_window*)pxw;
  _xcbt* x = (_xcbt*)xw->x;
  xcb_screen_t* screen = x->screens.data[xw->screen];
  int16_t cx16 = (int16_t)cx;
  int16_t cy16 = (int16_t)cy;
  if (flag == XCBT_WINDOW)
  {
    // Get the window position relative to the screen root.
    xcbt_rect re;
    xcbt_window_get_screen_pos(pxw, &re);
    xcb_warp_pointer_checked(x->conn, XCB_NONE, screen->root, 0, 0, 0, 0, re.x + cx16, re.y + cy16);
  }
  else if (flag == XCBT_RELATIVE)
  {
    xcb_warp_pointer_checked(x->conn, XCB_NONE, XCB_NONE, 0, 0, 0, 0, cx16, cy16);
  }
  else if (flag == XCBT_ABSOLUTE)
  {
    xcb_warp_pointer_checked(x->conn, XCB_NONE, screen->root, 0, 0, 0, 0, cx16, cy16);
  }
  //xcbt_flush(x);
}

void xcbt_window_get_screen_pos(xcbt_window pxw, xcbt_rect* rect)
{
  _xcbt_window* xw = (_xcbt_window*)pxw;
  _xcbt* x = (_xcbt*)xw->x;

  // Get the window position relative to the screen root.
  xcb_query_tree_cookie_t cookie1 = xcb_query_tree(x->conn, xw->wnd);
  xcb_query_tree_reply_t* tree = xcb_query_tree_reply(x->conn, cookie1, NULL);
  if (!tree) {
    return;
  }

  xcb_translate_coordinates_cookie_t cookie2 = xcb_translate_coordinates(x->conn,
      xw->wnd, tree->root, xw->pos.x, xw->pos.y);
  xcb_translate_coordinates_reply_t* trans = xcb_translate_coordinates_reply(x->conn, cookie2, NULL);
  if (!trans) {
    free(tree);
    return;
  }

  rect->x = trans->dst_x;
  rect->y = trans->dst_y;
  free(tree);
  free(trans);
}

/*
 * Event 0 is an error, print details  
 */
static void _xcbt_event_process_error(_xcbt *x, xcb_value_error_t *err) {
	TRACE("XCB ERROR:: code %d, sequence %d, value %d, opcode %d:%d\n", err->error_code, err->sequence, err->bad_value, err->minor_opcode, err->major_opcode);
}

/*
 * Some events are for windows, other not...
 */
static void xcbt_event_process(_xcbt *x, xcb_generic_event_t *evt){
  xcb_window_t wnd;
  _xcbt_window *xw = x->windows;
  switch(evt->response_type & ~0x80){
    case 0:
      _xcbt_event_process_error(x, (xcb_value_error_t *)evt);
      free(evt);
      return;
    case XCB_MAP_NOTIFY:
    case XCB_UNMAP_NOTIFY:
    case XCB_EXPOSE:
    case XCB_CLIENT_MESSAGE:
    case XCB_CONFIGURE_NOTIFY:
    case XCB_REPARENT_NOTIFY:
    case XCB_PROPERTY_NOTIFY:
      // TODO: all events with window in the first position
      wnd = (xcb_window_t)evt->pad[0];
      break;
    case XCB_KEY_PRESS:
    case XCB_KEY_RELEASE:  
    case XCB_MOTION_NOTIFY:
    case XCB_ENTER_NOTIFY:
    case XCB_LEAVE_NOTIFY:
    case XCB_BUTTON_PRESS:
    case XCB_BUTTON_RELEASE:
      {
        /*
        if((evt->response_type & ~0x80) == XCB_BUTTON_PRESS)
          printf("->\n");
        if((evt->response_type & ~0x80) == XCB_BUTTON_RELEASE)
          printf("<-\n");
          */
        xcb_motion_notify_event_t *mn = (xcb_motion_notify_event_t *)evt;
        wnd = mn->event;
        // TODO: check when happenes in case of grabs 
        break;
      }
    // TODO: all events with window in other position
    default:
      TRACE("NI: Event %d\n", evt->response_type);
      free(evt);
      return;
  }
  while(xw && (xw->wnd != wnd)){
    xw = xw->x_next;
  }
  if(!xw){
    TRACE("Event %d for unkown window\n", evt->response_type);
  } else {
    xw->uhandler((xcbt_window)xw, evt, xw->udata);
  }
  free(evt);
}

/*
 * Execute timers when it is time to do so.
 * 
 * Returns in milliseconds the eariest timer or -1 in case there is no timers.
 */
static int _xcbt_process_timers(_xcbt *x){
  _xcbt_timer *t;
  time_t tv_sec;
  int    tv_msec;
  if(x && x->timers){
    while((t = x->timers)){
      xcbt_time(&tv_sec, &tv_msec);
      if((t->tv_sec > tv_sec) || ((t->tv_sec == tv_sec) && (t->tv_msec > tv_msec))){ // not yet
        return (t->tv_sec - tv_sec)*1000 + t->tv_msec - tv_msec;
      }
      x->timers = t->next;
      x->timer_changed = 1;
      t->cb((xcbt)x, t->id, t->udata); // it can modify x->timers!
      free(t);
    }
  }
  return -1;
}


int xcbt_process(xcbt px){
  _xcbt *x = (_xcbt *)px;
  int msec = -1;
  xcb_generic_event_t *evt;
  if(x){
    xcb_flush(x->conn);
    while((evt = xcb_poll_for_event(x->conn))){
      //printf("%p- %x (%d)\n", x->conn, evt->response_type, xcb_get_file_descriptor(x->conn));
      xcbt_event_process(x, evt);
    }
    msec = _xcbt_process_timers(x);
    if(x->timer_changed){
      if(x->embed)
        x->embed->set_timer(x->embed, msec);
      x->timer_changed = 0;
    }
  }
  return msec;
}


/**
 * Event loop (blocking)
 * 
 * Parameters:
 *   exit_cond - a pointer to variable, when this variable is not zero the loop exit. When NULL, only currently pending events are processed.
 *
 */
void xcbt_event_loop(xcbt px, int *exit_cond){
  _xcbt *x = (_xcbt *)px;
  int msec;
  if(x){
    if(exit_cond){
      int fd = xcb_get_file_descriptor(x->conn);
      msec = xcbt_process(px);
      while(!*exit_cond){
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        int ret;
        if(msec > 0){
          struct timeval tv;
          tv.tv_sec  = msec/1000;
          tv.tv_usec = (msec%1000)*1000;
          ret = select(fd + 1, &rfds, NULL, NULL, &tv);
        } else {
          ret = select(fd + 1, &rfds, NULL, NULL, NULL);
        }
        if((ret < 0) && (errno != EINTR)){
          break;
        }
        msec = xcbt_process(px);
      }
    } else {
      xcbt_process(px);
    }
  }
}

static void xcbt_receive_clipboard(_xcbt_window *xw, xcb_atom_t property)
{
  _xcbt* x = xw->x;
  xcb_icccm_get_text_property_reply_t prop;
  xcb_get_property_cookie_t cookie = xcb_icccm_get_text_property(
      x->conn, xw->wnd, property);
  if (xcb_icccm_get_text_property_reply(x->conn, cookie, &prop, NULL))
  {
    // Allocate space for the null ptr
    x->clipboard_length = prop.name_len + 1;
    x->clipboard_data = realloc(x->clipboard_data, x->clipboard_length);
    memcpy(x->clipboard_data, prop.name, x->clipboard_length);
    x->clipboard_owner = 0;
    x->clipboard_data[x->clipboard_length - 1] = 0;
    xcb_icccm_get_text_property_reply_wipe(&prop);
    xcb_delete_property(x->conn, xw->wnd, property); 
  }
}

static void xcbt_window_default_handler(_xcbt_window *xw, xcb_generic_event_t *evt, void *unused){
  if (!xw || !evt)
  {
    return; // required actions are handled by window_destroy
  }
  _xcbt* x = xw->x;
  switch(evt->response_type & ~0x80){
    case XCB_EXPOSE:
      break;
    case XCB_MAP_NOTIFY:
      {
        xcb_map_notify_event_t *mn = (xcb_map_notify_event_t *)evt;
        if(mn->event == mn->window){
          if(!xw->xmapped){
            xw->xmapped = 1;
            if(!xw->mapped){ 
              // TODO: while waiting for map confirmation, we could decided to unmap, but with XEmbed we are mapped by embedder...
              // xcb_unmap_window(xw->x->conn, xw->wnd);
            }
          }
        } // else comes from SubstructureNotify, can be interesting for specialy cases only
        break;
      } 
    case XCB_UNMAP_NOTIFY:
      {
        xcb_unmap_notify_event_t *mn = (xcb_unmap_notify_event_t *)evt;
        if(mn->event == mn->window){
          if(xw->xmapped){
            xw->xmapped = 0;
            // if xw->mapped we have not requested unmapping, but that could be WM, so leave it unmapped
          }
        } // else comes from SubstructureNotify, can be interesting for specialy cases only
        break;
      } 
    case XCB_CONFIGURE_NOTIFY:
      {
        xcb_configure_notify_event_t *cn = (xcb_configure_notify_event_t *)evt;
        if(cn->event == cn->window){
          if( !(evt->response_type & 0x80) ){ // ignore synthetic, they are from WM about screen position
            if((cn->width != xw->pos.w) || (cn->height != xw->pos.h)){
              TRACE("Size is changed to %ux%u\n", cn->width, cn->height);
              xw->pos.w = cn->width;
              xw->pos.h = cn->height;
            }
          }
        } // else comes from SubstructureNotify, can be interesting for specialy cases only
        break;
      } 
      /*
      // can f.e. try to change position
      {
        const static uint32_t values[] = { 100, 100 };
        xcb_configure_window (main_win->con, main_win->win, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
        xcb_flush(main_win->con);
      } */
      break;
    case XCB_REPARENT_NOTIFY:
    case XCB_MOTION_NOTIFY:
    case XCB_ENTER_NOTIFY:
    case XCB_LEAVE_NOTIFY:
    case XCB_BUTTON_PRESS:
    case XCB_BUTTON_RELEASE:
      break;
    case XCB_CLIENT_MESSAGE:
      // TODO: may be process some...
      break;
    case XCB_PROPERTY_NOTIFY:
    {
      xcb_property_notify_event_t* e = (xcb_property_notify_event_t*) evt;
      // This is another way in which we can receive clipboard data.
      // Maybe because of Xwayland?
      if (e->atom == XCBT_ATOM_CLIPBOARD(x))
      {
        xcbt_receive_clipboard(xw, XCBT_ATOM_CLIPBOARD(x));
      }
    }
    case XCB_SELECTION_CLEAR:
    {
      // A window has been asked to not be the clipboard owner anymore.
      // We don't always receive this event, maybe due to Xwayland?
      xcb_selection_clear_event_t* ec = (xcb_selection_clear_event_t*) evt;
      if (ec->owner == x->clipboard_owner)
      {
        x->clipboard_owner = 0;
        free(x->clipboard_data);
        x->clipboard_data = NULL;
        x->clipboard_length = 0;
      }
      // Now we want to know who the new owner is.
      break;
    }
    case XCB_SELECTION_REQUEST:
    {
      // This is a request for the clipboard content.
      xcb_selection_request_event_t* ec = (xcb_selection_request_event_t*) evt;
      if (ec->property == XCB_NONE)
      {
        ec->property = ec->target;
      }

      int valid = 1;
      if (ec->target == XCBT_ATOM_UTF8_STRING(x)
          || ec->target == XCBT_ATOM_STRING(x)
          || ec->target == XCBT_ATOM_TEXT(x))
      {
        xcb_change_property(x->conn, XCB_PROP_MODE_REPLACE, ec->requestor, ec->property,
            ec->target, 8, x->clipboard_length, x->clipboard_data);
      }
      else if (ec->target == XCBT_ATOM_TARGETS(x))
      {
        // Request to see what type of targets we support
        xcb_atom_t targets[] = {
          XCBT_ATOM_TARGETS(x),
          XCBT_ATOM_UTF8_STRING(x),
          XCBT_ATOM_STRING(x),
          XCBT_ATOM_TEXT(x),
        };
        xcb_change_property(x->conn, XCB_PROP_MODE_REPLACE, ec->requestor,
            ec->property, XCB_ATOM_ATOM, sizeof(xcb_atom_t),
            sizeof(targets) / sizeof(xcb_atom_t), targets);
      }
      else
      {
        valid = 0;
      }

      xcb_selection_notify_event_t notify = {0};
      notify.response_type = XCB_SELECTION_NOTIFY;
      notify.time = XCB_CURRENT_TIME;
      notify.requestor = ec->requestor;
      notify.selection = ec->selection;
      notify.target = ec->target;
      notify.property = valid ? ec->property : XCB_NONE;
      xcb_send_event(x->conn, 0, ec->requestor, XCB_EVENT_MASK_PROPERTY_CHANGE, (char*)&notify);
      xcbt_flush(x);

      break;
    }
    case XCB_SELECTION_NOTIFY:
    {
      // We requested clipoard data and it has arrived.
      // In some cases 
      xcb_selection_notify_event_t* e_notify = (xcb_selection_notify_event_t*) evt;
      if (e_notify->selection == XCBT_ATOM_CLIPBOARD(x) && e_notify->property != XCB_NONE)
      {
        xcb_icccm_get_text_property_reply_t prop;
        xcb_get_property_cookie_t cookie = xcb_icccm_get_text_property(
            x->conn, e_notify->requestor, e_notify->property);
        if (xcb_icccm_get_text_property_reply(x->conn, cookie, &prop, NULL))
        {
          x->clipboard_length = prop.name_len + 1;
          x->clipboard_data = realloc(x->clipboard_data, x->clipboard_length);
          memcpy(x->clipboard_data, prop.name, x->clipboard_length);
          x->clipboard_owner = 0;
          xcb_icccm_get_text_property_reply_wipe(&prop);
          xcb_delete_property(x->conn, e_notify->requestor, e_notify->property); 
        }
      }
      break;
    }
    default:
      TRACE("Win NI: Event %d (%s)\n", evt->response_type & ~0x80, evt->response_type & 0x80 ? "Synthetic" : "Native");
      return;
  }
}

xcb_gcontext_t xcbt_window_create_gc(xcbt_window pxw, uint32_t value_mask, const uint32_t *value_list){
  if(pxw){
    xcb_gcontext_t gc = xcb_generate_id(xcbt_window_conn(pxw));
    xcb_create_gc(xcbt_window_conn(pxw), gc, xcbt_window_xwnd(pxw), value_mask, value_list);
    return gc;
  }
  return 0;
}

static xcb_gcontext_t _xcbt_window_default_gc(_xcbt_window *xw){
  if(xw){
    if(xw->gc)
      return xw->gc;
    else {
      xcb_screen_t *si = xcbt_screen_info((xcbt)xw->x, xw->screen);
      if(si){
        uint32_t value_list[2] = { si->white_pixel, si->black_pixel };
        return xw->gc = xcbt_window_create_gc((xcbt_window)xw, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, value_list);
      }
    }
  }
  return 0;
} 

int xcbt_window_draw_img(xcbt_window pxw, unsigned depth, unsigned w, unsigned h, int x, int y, unsigned data_length, uint8_t *data){
  _xcbt_window *xw = (_xcbt_window *)pxw;
  if(xw){
    xcb_put_image(xw->x->conn, XCB_IMAGE_FORMAT_Z_PIXMAP, xw->wnd, _xcbt_window_default_gc(xw), w, h, x, y, 0, depth, data_length, data);
    return 1;
  }
  return 0;
}
  
int xcbt_embed_set(xcbt px, xcbt_embed *e){
  _xcbt *x = (_xcbt *)px;
  if(!x)
    return 0;
  if(x->embed){
    x->embed->set_timer(x->embed, -1);
    x->embed->watch(x->embed, -1);
    x->embed->set_x(x->embed, NULL);
    x->embed = NULL;
  }
  if((x->embed = e)){
    if(x->embed->set_x(x->embed, px)){
      if(x->embed->watch(x->embed, xcb_get_file_descriptor(x->conn))){
        x->timer_changed = 1;
        xcbt_process(px); // to arm real timer
        return 1;
      }
      x->embed->set_x(x->embed, NULL);
    }
    x->embed = NULL;
  }
  return 0;
}

////////////////////////////////////////7
//
// Glib embedding

// I guess it never change, but...
#define G_IO_IN 1

typedef void GIOChannel;

#define _XCBT_EMBED_WATCH_COUNT 16

typedef struct {
  xcbt_embed embed;

  xcbt x;

  struct {
    int         tag;
    int         fd;
  } watch[_XCBT_EMBED_WATCH_COUNT];
  int watch_count;
  
  int  timer_tag;
    
  void    *(*g_io_channel_unix_new)(int fd);
  unsigned (*g_io_add_watch)(GIOChannel *channel, int condition, void *func, void *user_data);
  void     (*g_io_channel_unref)(GIOChannel *channel);
  int      (*g_source_remove)(int tag);
  int      (*g_timeout_add)(int interval, void *func, void *data);
} _xcbt_embed_glib;


static int xcbt_embed_glib_timer_cb(GIOChannel *source, int condition, _xcbt_embed_glib *eg){
  // I am not really sure that is required..
  if(eg->timer_tag){
    eg->g_source_remove(eg->timer_tag);
    eg->timer_tag = 0;
  }
  xcbt_process(eg->x);
  return 0;
}

static int xcbt_embed_glib_watch_cb(GIOChannel *source, int condition, _xcbt_embed_glib *eg){
  xcbt_process(eg->x);
  return 1;
}

static int xcbt_embed_glib_set_timer(xcbt_embed *pe, int msec){
  _xcbt_embed_glib *eg = (_xcbt_embed_glib *)pe;
  if(!eg)
    return 0;
  if(eg->timer_tag){
    eg->g_source_remove(eg->timer_tag);
  }
  if(msec > 0){
    eg->timer_tag = eg->g_timeout_add(msec, xcbt_embed_glib_timer_cb, eg);
  } else {
    eg->timer_tag = 0;
    return 1;
  }
  return eg->timer_tag != 0;
}

static int xcbt_embed_glib_watch(xcbt_embed *pe, int fd){
  _xcbt_embed_glib *eg = (_xcbt_embed_glib *)pe;
  int i;
  GIOChannel *ioch;
  if(!eg)
    return 0;
  if(fd < 0){
    for(i = 0; i < eg->watch_count; ++i){
      eg->g_source_remove(eg->watch[i].tag);
    }
    eg->watch_count = 0;
    return 1;
  }
  for(i = 0; (i < eg->watch_count) && (eg->watch[i].fd != fd); ++i);
  if(i < eg->watch_count){
    TRACE("BUG: file descriptor is already in the watch list\n");
    return 1;
  }
  if(eg->watch_count < _XCBT_EMBED_WATCH_COUNT){
    i = eg->watch_count;
    if((ioch = eg->g_io_channel_unix_new(fd))){
      if((eg->watch[i].tag = eg->g_io_add_watch(ioch, G_IO_IN, xcbt_embed_glib_watch_cb, eg))){
        eg->g_io_channel_unref(ioch);
        ++eg->watch_count;
        return 1;
      }
      eg->g_io_channel_unref(ioch);
    }
  }
  return 0;
}

static void xcbt_embed_glib_dtor(xcbt_embed *pe){
  _xcbt_embed_glib *eg = (_xcbt_embed_glib *)pe;
  if(eg){
    if(eg->x){
      TRACE("XCBT:BUG: embed is still in use\n");
    } else {
      if( eg->watch_count || eg->timer_tag ){
        TRACE("XCBT:BUG: embed is still in use\n");
        xcbt_embed_glib_watch(pe, -1);
        xcbt_embed_glib_set_timer(pe, -1);
      }
      free(eg);
    }
  }
}

static int xcbt_embed_glib_set_x(xcbt_embed *pe, xcbt x){
  _xcbt_embed_glib *eg = (_xcbt_embed_glib *)pe;
  if(eg){
    if(!eg->x){
      eg->x = x;
      return 1;
    } else if(!x){
      xcbt_embed_glib_watch(pe, -1);
      xcbt_embed_glib_set_timer(pe, -1);
      eg->x = NULL;
    } else {
      TRACE("XCBT:BUG: embed is in use by other X\n");
    }
  }
  return 0;
}

xcbt_embed *xcbt_embed_glib(){
  _xcbt_embed_glib *eg = (_xcbt_embed_glib *)calloc(1, sizeof(*eg));
  void *h = NULL;
  eg->embed.dtor = xcbt_embed_glib_dtor;
  eg->embed.set_x = xcbt_embed_glib_set_x;
  eg->embed.set_timer = xcbt_embed_glib_set_timer;
  eg->embed.watch = xcbt_embed_glib_watch;

  if(!(eg->g_io_channel_unix_new = dlsym(h, "g_io_channel_unix_new"))){
    TRACE("INFO: compatible GLib is not found, attempt to load GLib\n");
    // I try to open the latest first, in hope it is used in the app...
    if((h = dlopen("libglib-2.0.so", RTLD_LAZY)) || (h = dlopen("libglib-2.0.so.0", RTLD_LAZY))){
      TRACE("INFO: GLib is loaded explicitly\n");
    }
  }
  
  if(!(eg->g_io_channel_unix_new = dlsym(h, "g_io_channel_unix_new")) ||
     !(eg->g_io_add_watch = dlsym(h, "g_io_add_watch")) ||
     !(eg->g_io_channel_unref = dlsym(h, "g_io_channel_unref")) ||
     !(eg->g_source_remove = dlsym(h, "g_source_remove")) ||
     !(eg->g_timeout_add = dlsym(h, "g_timeout_add"))
     ){
    
    TRACE("%p\n", eg->g_timeout_add);
    TRACE("FATAL: compatible GLib is not loaded\n");
    free(eg);
    return NULL;
  }
  eg->x = NULL;
  return &eg->embed;
}

///////////////////////////////////
//
// onIdle embedding

typedef struct {
  xcbt_embed embed;

  xcbt x;
} _xcbt_embed_idle;


void xcbt_embed_idle_cb(xcbt_embed *pe){
  _xcbt_embed_idle *ei = (_xcbt_embed_idle *)pe;
  if(ei)
    xcbt_process(ei->x);
}

static int xcbt_embed_idle_set_timer(xcbt_embed *pe, int msec){
  return 1;
}

static int xcbt_embed_idle_watch(xcbt_embed *pe, int fd){
  return 1;
}

static void xcbt_embed_idle_dtor(xcbt_embed *pe){
  _xcbt_embed_idle *ei = (_xcbt_embed_idle *)pe;
  if(ei){
    if(ei->x){
      TRACE("XCBT:BUG: embed is still in use\n");
    } else {
      free(ei);
    }
  }
}

static int xcbt_embed_idle_set_x(xcbt_embed *pe, xcbt x){
  _xcbt_embed_idle *ei = (_xcbt_embed_idle *)pe;
  if(ei){
    if(!ei->x){
      ei->x = x;
      return 1;
    } else if(!x){
      ei->x = NULL;
    } else {
      TRACE("XCBT:BUG: embed is in use by other X\n");
    }
  }
  return 0;
}

xcbt_embed *xcbt_embed_idle(){
  _xcbt_embed_idle *ei = (_xcbt_embed_idle *)calloc(1, sizeof(*ei));
  ei->embed.dtor = xcbt_embed_idle_dtor;
  ei->embed.set_x = xcbt_embed_idle_set_x;
  ei->embed.set_timer = xcbt_embed_idle_set_timer;
  ei->embed.watch = xcbt_embed_idle_watch;
  return &ei->embed;
}
