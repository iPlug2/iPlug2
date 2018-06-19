#ifndef __IGRAPHICS_SRC_INC__
#define __IGRAPHICS_SRC_INC__

#include "IPlugPlatform.h"

#ifndef NO_IGRAPHICS

 #if defined OS_WIN
  extern HINSTANCE gHINSTANCE;

  IGraphics* MakeGraphics(IEditorDelegate& dlg, int w, int h, int fps = 0, float scale = 1.)
  {
    IGraphicsWin* pGraphics = new IGraphicsWin(dlg, w, h, fps, scale);
    pGraphics->SetPlatformInstance(gHINSTANCE);
    return pGraphics;
  }
  #elif defined OS_MAC
  IGraphics* MakeGraphics(IEditorDelegate& dlg, int w, int h, int fps = 0, float scale = 1.)
  {
    IGraphicsMac* pGraphics = new IGraphicsMac(dlg, w, h, fps, scale);
    pGraphics->SetBundleID(BUNDLE_ID);
  #ifdef IGRAPHICS_NANOVG
    pGraphics->CreateMetalLayer();
  #endif
    return pGraphics;
  }
  #elif defined OS_IOS
  IGraphics* MakeGraphics(IEditorDelegate& dlg, int w, int h, int fps = 0, float scale = 1.)
  {
    IGraphicsIOS* pGraphics = new IGraphicsIOS(dlg, w, h, fps, scale);
    pGraphics->SetBundleID(BUNDLE_ID);
    return pGraphics;
  }
  #elif defined OS_WEB
  #include <emscripten.h>

  IGraphicsWeb* gGraphics = nullptr;

  IGraphics* MakeGraphics(IEditorDelegate& dlg, int w, int h, int fps = 0, float scale = 1.)
  {
    gGraphics = new IGraphicsWeb(dlg, w, h, fps);
    return gGraphics;
  }

  void StartMainLoopTimer()
  {
    emscripten_set_main_loop(gGraphics->OnMainLoopTimer, gGraphics->FPS(), 1);
  }
  #else
    #error "No OS defined!"
  #endif

#endif //NO_IGRAPHICS

#endif //__IGRAPHICS_SRC_INC__
