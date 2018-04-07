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
  #elif defined OS_WEB
  IGraphics* MakeGraphics(IDelegate& dlg, int w, int h, int fps = 0)
  {
    IGraphicsWeb* pGraphics = new IGraphicsWeb(dlg, w, h, fps);
    return pGraphics;
  }
  #else
    #error "No OS defined!"
  #endif
#endif //NO_IGRAPHICS
