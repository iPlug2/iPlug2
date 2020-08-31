/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

/*
 * AZ TODO:
 *   * VST3
 *   * mouse input
 *   * keyboard input
 *   * LICE
 *   * FONTS_USE_FREETYPE and SKIA
 */

#include <wdlutf8.h>

#include "IPlugParameter.h"
#include "IGraphicsLinux.h"
#include "IPopupMenuControl.h"
#include "IPlugPaths.h"

#ifdef OS_LINUX
#ifdef IGRAPHICS_GL
#endif
  #include <fontconfig/fontconfig.h>
#endif

#define CHECK  printf("%s: need check\n", __FUNCTION__);
#define NOTIMP printf("%s: not implemented\n", __FUNCTION__);


using namespace iplug;
using namespace igraphics;

#define IPLUG_TIMER_ID 2

class IGraphicsLinux::Font : public PlatformFont
{
public:
  Font(WDL_String &fileName) : PlatformFont(false), mFileName(fileName) { }
  IFontDataPtr GetFontData() override;

private:
  WDL_String mFileName;
};

void IGraphicsLinux::Paint()
{
  IRECT ir = {0, 0, static_cast<float>(WindowWidth()), static_cast<float>(WindowHeight())};
  IRECTList rects;
  rects.Add(ir);
  // DBGMSG("Paint\n");
  void *ctx = xcbt_window_draw_begin(mPlugWnd);

  if(ctx){
    Draw(rects);
    xcbt_window_draw_end(mPlugWnd);
  }
}

void IGraphicsLinux::DrawResize()
{
  void *ctx = xcbt_window_draw_begin(mPlugWnd);
  if(ctx){
    IGRAPHICS_DRAW_CLASS::DrawResize();
    xcbt_window_draw_stop(mPlugWnd); // WARNING: in CAN BE reentrant!!! (f.e. it is called from SetScreenScale during initialization)
  }
  // WARNING: IPlug call it on resize, but at the end. When should we call Paint() ?
  // In Windows version "Update window" is called from PlatformResize, so BEFORE DrawResize...
}


inline IMouseInfo IGraphicsLinux::GetMouseInfo(int16_t x, int16_t y, int16_t state)
{
  IMouseInfo info;
  info.x = mCursorX = x / (GetDrawScale() * GetScreenScale());
  info.y = mCursorY = y / (GetDrawScale() * GetScreenScale());
  info.ms = IMouseMod((state & XCB_BUTTON_MASK_1), (state & XCB_BUTTON_MASK_3), // Note "2" is the middle button 
    (state & XCB_KEY_BUT_MASK_SHIFT), (state & XCB_KEY_BUT_MASK_CONTROL), (state & XCB_KEY_BUT_MASK_MOD_1) // shift, ctrl, alt
  );
  // info.ms.DBGPrint();
  return info;
}

inline IMouseInfo IGraphicsLinux::GetMouseInfoDeltas(float& dX, float& dY, int16_t x, int16_t y, int16_t state)
{
  float oldX = mCursorX;
  float oldY = mCursorY;
      
  IMouseInfo info = GetMouseInfo(x, y, state);
  
  dX = info.x - oldX;
  dY = info.y - oldY;
    
  return info;
}


void IGraphicsLinux::TimerHandler(int timer_id){
  if(timer_id == IPLUG_TIMER_ID)
  {
    IRECTList rects;
    if(IsDirty(rects))
    {
      Paint();
      SetAllControlsClean();
    }
    xcbt_timer_set(mX, IPLUG_TIMER_ID, 20, (xcbt_timer_cb)TimerHandlerProxy, this);
  }
}

void IGraphicsLinux::WindowHandler(xcb_generic_event_t *evt){
  static struct timeval pt = {0}, ct;
  if(!evt) {
    mBaseWindowHandler(mPlugWnd, NULL, mBaseWindowData);
    mPlugWnd = nullptr;
  } else {
    switch(evt->response_type & ~0x80)
    {
      case XCB_EXPOSE:
        {
          xcb_expose_event_t *ee = (xcb_expose_event_t *)evt;
          if(!ee->count) // MAYBE: can collect and use invalidated areas
          {
            Paint();
          }
        }
        break;
      case XCB_BUTTON_PRESS:
        {
          xcb_button_press_event_t *bp = (xcb_button_press_event_t *)evt;
          if(bp->detail == 1){ // check for double-click
            if(!mLastLeftClickStamp)
            {
              mLastLeftClickStamp = bp->time;
            } else
            {
              if ((bp->time - mLastLeftClickStamp) < 500) // MAYBE: somehow find user settings
              {
                IMouseInfo info = GetMouseInfo(bp->event_x, bp->event_y, bp->state | XCB_BUTTON_MASK_1); // convert button to state mask

                if (OnMouseDblClick(info.x, info.y, info.ms))
                {
                  // TODO: SetCapture(hWnd);
                }
                mLastLeftClickStamp = 0;
                xcbt_flush(mX);
                break;
              }
              mLastLeftClickStamp = bp->time;
            }
          } else
          {
            mLastLeftClickStamp = 0;
          }
          // TODO: hide tooltips
          // TODO: end parameter editing (if in progress, and return then)
          // TODO: set focus
          
          // TODO: detect double click
          
          // TODO: set capture (or after capture...) (but check other buttons first)
          if((bp->detail == 1) || (bp->detail == 3)){ // left/right
            uint16_t state = bp->state | (0x80<<bp->detail); // merge state before with pressed button
            IMouseInfo info = GetMouseInfo(bp->event_x, bp->event_y, state); // convert button to state mask
            std::vector<IMouseInfo> list{ info };
            OnMouseDown(list);
          } else if((bp->detail == 4) || (bp->detail == 5)){ // wheel
            IMouseInfo info = GetMouseInfo(bp->event_x, bp->event_y, bp->state);
            OnMouseWheel(info.x, info.y, info.ms, bp->detail == 4 ? 1. : -1);
          }
          xcbt_flush(mX);
          break;
        }
      case XCB_BUTTON_RELEASE:
        {
          xcb_button_release_event_t *br = (xcb_button_release_event_t *)evt;
          // TODO: release capture (but check other buttons first...)
          if((br->detail == 1) || (br->detail == 3)){ // we do not process other buttons, at least not yet
            uint16_t state = br->state & ~(0x80<<br->detail); // merge state before with released button
            IMouseInfo info = GetMouseInfo(br->event_x, br->event_y, state); // convert button to state mask
            std::vector<IMouseInfo> list{ info };
            OnMouseUp(list);
          }
          xcbt_flush(mX);
          break;
        }
      case XCB_MOTION_NOTIFY:
        {
          xcb_motion_notify_event_t *mn = (xcb_motion_notify_event_t *)evt;
          mLastLeftClickStamp = 0;
          if(mn->same_screen && (mn->event == xcbt_window_xwnd(mPlugWnd))){
            // can use event_x/y
            if(!(mn->state & (XCB_BUTTON_MASK_1 | XCB_BUTTON_MASK_3))) // Not left/rightn drag
            {
              //DBGMSG("Move\n");
              IMouseInfo info = GetMouseInfo(mn->event_x, mn->event_y, mn->state);
              if (OnMouseOver(info.x, info.y, info.ms))
              {
                // TODO: tracking and tooltips
              }

            } else {
              float dX, dY;
              IMouseInfo info = GetMouseInfoDeltas(dX, dY, mn->event_x, mn->event_y, mn->state); //TODO: clean this up
              if (dX || dY)
              {
                info.dX = dX;
                info.dY = dY;
                std::vector<IMouseInfo> list{ info };

                OnMouseDrag(list);
                /* TODO:
                if (MouseCursorIsLocked())
                  MoveMouseCursor(pGraphics->mHiddenCursorX, pGraphics->mHiddenCursorY);
                  */
              }
            }
          }
          xcbt_flush(mX);
          break;
        }
      case XCB_PROPERTY_NOTIFY:
        {
          xcb_property_notify_event_t *pn = (xcb_property_notify_event_t *)evt;
          if(pn->atom == XCBT_XEMBED_INFO(mX)){
            // TODO: check we really have to, but getting XEMBED_MAPPED and compare with current mapping status
            xcbt_window_map(mPlugWnd);
          }
          break;
        }
      default:
        break;
    }
  }
  mBaseWindowHandler(mPlugWnd, evt, mBaseWindowData);
}

void IGraphicsLinux::SetIntegration(void *mainLoop){
  xcbt_embed *e = static_cast<xcbt_embed *>(mainLoop);
  if(!e)
  {
    if(mEmbed)
    {
      if(mX)
      {
        // DBGMSG("asked to unset embedding, but X is still active\n"); that in fact how it goes, frame is unset before CloseWindow TODO: check why
        xcbt_embed_set(mX, nullptr);
      }
      xcbt_embed_dtor(mEmbed);
      mEmbed = nullptr;
    }
  } else
  {
    if(mEmbed)
    {
      DBGMSG("BUG: embed is already set\n");
    } else
    {
      mEmbed = e;
    }
  }
}

void* IGraphicsLinux::OpenWindow(void* pParent)
{
  xcbt_rect r = {0, 0, static_cast<int16_t>(WindowWidth()), static_cast<int16_t>(WindowHeight())};
  xcb_window_t xprt = (intptr_t)pParent;
  
#ifdef APP_API
  if(!mEmbed)
  {
    SetIntegration(xcbt_embed_glib());
  }
#endif

  if(!mEmbed){
    DBGMSG("BUG: embed is not defined\n");
    return NULL;
  }

#ifdef IGRAPHICS_GL
  mX = xcbt_connect(XCBT_USE_GL|XCBT_INIT_ATOMS);
#else
  mX = xcbt_connect(0);
#endif
  if (!mX)
  {
    return NULL;
  }

#ifdef LV2_API
  if (!xprt)
  {
    // LV2 UI is created without parent by default, it may be found and even required with ui:parent feature, but the documentation
    // say that is not a good idea.
    xcb_screen_t *si = xcbt_screen_info(mX, xcbt_default_screen(mX));
    if (si)
    {
      xprt = si->root;
    }
  }
#endif

  // NOTE: In case plug-in report REAPER extension in REAPER, pParent is NOT XID (SWELL HWND? I have not checked yet)

#ifdef IGRAPHICS_GL
#ifdef IGRAPHICS_GL2
  mPlugWnd = xcbt_window_gl_create(mX, xprt, &r, 2, 1, 0);
#elif defined IGRAPHICS_GL3
  mPlugWnd = xcbt_window_gl_create(mX, xprt, &r, 3, 0, 0);
#else
  #error "Unsupported GL version"
#endif
#else

  mPlugWnd = xcbt_window_create(mX, xprt, &r);
#endif
  if (!mPlugWnd)
  {
    xcbt_disconnect(mX);
    mX = NULL;
    return NULL;
  }

  xcbt_window_set_handler(mPlugWnd, (xcbt_window_handler)WindowHandlerProxy, this, &mBaseWindowHandler, &mBaseWindowData);

  if(mEmbed && !xcbt_embed_set(mX, mEmbed))
  {
    DBGMSG("Could not embed into main event loop\n");
    xcbt_window_destroy(mPlugWnd);
    mPlugWnd = NULL;
    xcbt_disconnect(mX);
    mX = NULL;
    return NULL;
  }

  if(xcbt_window_draw_begin(mPlugWnd))
  {  // GL context set
    OnViewInitialized( nullptr );
    SetScreenScale(1); // resizes draw context, calls DrawResize

    GetDelegate()->LayoutUI(this);
    SetAllControlsDirty();
    GetDelegate()->OnUIOpen();

    xcbt_window_draw_stop(mPlugWnd);
  }

  xcbt_timer_set(mX, IPLUG_TIMER_ID, 20, (xcbt_timer_cb)TimerHandlerProxy, this);

#ifdef APP_API
  xcbt_window_map(mPlugWnd);
#elif defined VST2_API
  xcbt_window_set_xembed_info(mPlugWnd);
#elif defined VST3_API
  xcbt_window_set_xembed_info(mPlugWnd);
#elif defined LV2_API
  xcbt_window_set_xembed_info(mPlugWnd);
#else
  #error "Map or not to map... that is the question"
#endif
  xcbt_sync(mX); // make sure everything is ready before reporting it is
  return reinterpret_cast<void *>(xcbt_window_xwnd(mPlugWnd));
}

void IGraphicsLinux::CloseWindow()
{
  if (mPlugWnd)
  {
    OnViewDestroyed();

    SetPlatformContext(nullptr); // I do not set it, but Cairo does...

    xcbt_window_destroy(mPlugWnd);
    mPlugWnd = NULL;
    xcbt_disconnect(mX);
    mX = NULL;
  }
  mEmbed = nullptr; // TODO: memory leak!
}

EMsgBoxResult IGraphicsLinux::ShowMessageBox(const char* text, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler)
{
  NOTIMP;

  return kNoResult;
}

void IGraphicsLinux::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* extensions)
{
  if (!WindowIsOpen())
  {
    fileName.Set("");
    return;
  }

  NOTIMP;
  fileName.Set("");
}

void IGraphicsLinux::PromptForDirectory(WDL_String& dir){
  NOTIMP;
}

void IGraphicsLinux::PlatformResize(bool parentHasResized)
{
  if (WindowIsOpen())
  {
    xcb_connection_t *conn = xcbt_conn(mX);
    xcb_window_t      w    = xcbt_window_xwnd(mPlugWnd);
    uint32_t values[] = { static_cast<uint32_t>(WindowWidth() * GetScreenScale()), static_cast<uint32_t>(WindowHeight() * GetScreenScale()) };
    xcb_configure_window(conn, w, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
    DBGMSG("INFO: resized to %ux%u\n", values[0], values[1]);
    if(!parentHasResized)
    {
      DBGMSG("WARNING: parent is not resized, but I (should) have no control on it on X... XEMBED?\n");
      xcb_window_t prt = xcbt_window_xprt(mPlugWnd);
      if( prt )
      {
        xcb_configure_window(conn, prt, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);        
      }
    }
    xcbt_flush(mX);
  }
}

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

IFontDataPtr IGraphicsLinux::Font::GetFontData()
{
  IFontDataPtr pData;
  int file = open(mFileName.Get(), O_RDONLY);
  if (file >= 0)
  {
    struct stat sb;
    if (fstat(file, &sb) == 0)
    {
      int fontSize = static_cast<int>(sb.st_size);
      void *pFontMem = mmap(NULL, fontSize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, file, 0);
      if (pFontMem != MAP_FAILED)
      {
        pData = std::make_unique<IFontData>(pFontMem, fontSize, 0);
        munmap(pFontMem, fontSize);
      }
    }
    close(file);
  }
  return pData;
}


PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  WDL_String fullPath;
  const EResourceLocation fontLocation = LocateResource(fileNameOrResID, "ttf", fullPath, GetBundleID(), GetWinModuleHandle(), nullptr);

  if ((fontLocation == kNotFound) || (fontLocation != kAbsolutePath) )
  {
    return nullptr;
  }

  return PlatformFontPtr(new Font(fullPath));
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  WDL_String fullPath;
  const char *styleString;
  switch ( style )
  {
    case ETextStyle::Bold: styleString = "bold"; break;
    case ETextStyle::Italic: styleString = "italic"; break;
    default: styleString = "regular";
  }

  FcConfig  *config = FcInitLoadConfigAndFonts(); // TODO: init/fini for plug-in lifetime
  FcPattern *pat = FcPatternBuild(nullptr, FC_FAMILY, FcTypeString, fontName, FC_STYLE, FcTypeString, styleString, nullptr);
  FcConfigSubstitute(config, pat, FcMatchPattern);
  FcResult result;
  FcPattern *font = FcFontMatch(config, pat, &result);
  if ( font )
  {
    FcChar8 *file;
    if ( FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch )
    {
      fullPath.Set((const char *)file);
    }
    FcPatternDestroy( font );
  }

  FcPatternDestroy(pat);
  FcConfigDestroy(config);

  return PlatformFontPtr(fullPath.Get()[0] ? new Font(fullPath) : nullptr);
}



IGraphicsLinux::IGraphicsLinux(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
  : IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
  //FcInit();
}

IGraphicsLinux::~IGraphicsLinux()
{
  CloseWindow();
  xcbt_embed_dtor(mEmbed);
  // FcFini();
}


#ifndef NO_IGRAPHICS
#if defined IGRAPHICS_AGG
  #include "IGraphicsAGG.cpp"
#elif defined IGRAPHICS_CAIRO
  #include "IGraphicsCairo.cpp"
#elif defined IGRAPHICS_LICE
  #include "IGraphicsLice.cpp"
#elif defined IGRAPHICS_SKIA
  #include "IGraphicsSkia.cpp"
#elif defined IGRAPHICS_NANOVG
  #include "IGraphicsNanoVG.cpp"
#ifdef IGRAPHICS_FREETYPE
#define FONS_USE_FREETYPE
#endif
  #include "nanovg.c"
#else
  #error
#endif
#endif
