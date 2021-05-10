 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugWebViewEditorDelegate.h"

using namespace iplug;

WebViewEditorDelegate::WebViewEditorDelegate(int nParams)
  : IEditorDelegate(nParams)
  , IWebView()
{
}

WebViewEditorDelegate::~WebViewEditorDelegate()
{
  CloseWindow();
}

#ifdef IGRAPHICS_QUANTISE_SCREENSCALE
extern int GetScaleForHWND(HWND hWnd);
#else
extern float GetScaleForHWND(HWND hWnd);
#endif

void* WebViewEditorDelegate::OpenWindow(void* pParent)
{
  auto scale = GetScaleForHWND((HWND) pParent);
  return OpenWebView(pParent, 0., 0., static_cast<float>((GetEditorWidth()) / scale), static_cast<float>((GetEditorHeight()) / scale), scale);
}

void WebViewEditorDelegate::Resize(int width, int height)
{
  SetWebViewBounds(0, 0, static_cast<float>(width), static_cast<float>(height));
  EditorResizeFromUI(width, height, true);
}