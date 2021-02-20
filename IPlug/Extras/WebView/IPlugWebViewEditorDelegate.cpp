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

extern float GetScaleForHWND(HWND hWnd);

void* WebViewEditorDelegate::OpenWindow(void* pParent)
{
  RECT r;
  HWND hWnd = (HWND) pParent;
  GetClientRect(hWnd, &r);
  float scale = static_cast<float>(GetScaleForHWND(hWnd));

  return OpenWebView(pParent, static_cast<float>(r.left / scale),
                              static_cast<float>(r.top / scale),
                              static_cast<float>((r.right - r.left) / scale),
                              static_cast<float>((r.bottom - r.top) / scale), scale);
}
