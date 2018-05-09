#ifndef __IGRAPHICS_SRC_INC__
#define __IGRAPHICS_SRC_INC__

#include "IPlugPlatform.h"

#ifndef NO_IGRAPHICS

 #if defined OS_WIN
  IGraphics* MakeGraphics(IDelegate& dlg, int w, int h, int fps = 0)
  {
    IGraphicsWin* pGraphics = new IGraphicsWin(dlg, w, h, fps);
    pGraphics->SetPlatformInstance(gHInstance);
    return pGraphics;
  }
  #elif defined OS_MAC
  IGraphics* MakeGraphics(IDelegate& dlg, int w, int h, int fps = 0)
  {
    IGraphicsMac* pGraphics = new IGraphicsMac(dlg, w, h, fps);
    pGraphics->SetBundleID(BUNDLE_ID);
  #ifdef IGRAPHICS_NANOVG
    pGraphics->CreateMetalLayer();
  #endif
    return pGraphics;
  }
  #elif defined OS_IOS
  IGraphics* MakeGraphics(IDelegate& dlg, int w, int h, int fps = 0)
  {
    IGraphicsIOS* pGraphics = new IGraphicsIOS(dlg, w, h, fps);
    pGraphics->SetBundleID(BUNDLE_ID);
    pGraphics->CreateMetalLayer();
    return pGraphics;
  }
  #elif defined OS_WEB
  #include <emscripten.h>

  IGraphics* gGraphics = nullptr;

  IGraphics* MakeGraphics(IDelegate& dlg, int w, int h, int fps = 0)
  {
    IGraphicsWeb* pGraphics = new IGraphicsWeb(dlg, w, h, fps);
    return pGraphics;
  }

  void StartMainLoopTimer()
  {
    emscripten_set_main_loop(dynamic_cast<IGraphicsWeb*>(gGraphics)->OnMainLoopTimer, gGraphics->FPS(), 1);
  }
  #else
    #error "No OS defined!"
  #endif

#endif //NO_IGRAPHICS

#endif //__IGRAPHICS_SRC_INC__
