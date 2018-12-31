/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef __IGRAPHICS_SRC_INC__
#define __IGRAPHICS_SRC_INC__

/**
 * @file IGraphics_include_in_plug_hdr.h
 * @brief IGraphics source include
 * Include this file in the main cpp file if using IGraphics outside a plugin context
 */

#include "IPlugPlatform.h"

#ifndef NO_IGRAPHICS

 #if defined OS_WIN
  extern HINSTANCE gHINSTANCE;

  IGraphics* MakeGraphics(IGEditorDelegate& dlg, int w, int h, int fps = 0, float scale = 1.)
  {
    IGraphicsWin* pGraphics = new IGraphicsWin(dlg, w, h, fps, scale);
    pGraphics->SetWinModuleHandle(gHINSTANCE);
    return pGraphics;
  }
  #elif defined OS_MAC
  IGraphics* MakeGraphics(IGEditorDelegate& dlg, int w, int h, int fps = 0, float scale = 1.)
  {
    IGraphicsMac* pGraphics = new IGraphicsMac(dlg, w, h, fps, scale);
    pGraphics->SetBundleID(BUNDLE_ID);
    
    return pGraphics;
  }
  #elif defined OS_IOS
  IGraphics* MakeGraphics(IGEditorDelegate& dlg, int w, int h, int fps = 0, float scale = 1.)
  {
    IGraphicsIOS* pGraphics = new IGraphicsIOS(dlg, w, h, fps, scale);
    pGraphics->SetBundleID(BUNDLE_ID);

    return pGraphics;
  }
  #elif defined OS_WEB
  #include <emscripten.h>

  IGraphicsWeb* gGraphics = nullptr;

  IGraphics* MakeGraphics(IGEditorDelegate& dlg, int w, int h, int fps = 0, float scale = 1.)
  {
    gGraphics = new IGraphicsWeb(dlg, w, h, fps, scale);
    return gGraphics;
  }

  void StartMainLoopTimer()
  {
    emscripten_set_main_loop(gGraphics->OnMainLoopTimer, 0 /*gGraphics->FPS()*/, 1);
  }
  #else
    #error "No OS defined!"
  #endif

#endif //NO_IGRAPHICS

#endif //__IGRAPHICS_SRC_INC__
