 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugWebViewEditorDelegate.h"
#include "winuser.h"

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

  int x = 0, y = 0, w = GetEditorWidth(), h = GetEditorHeight();

  mScale =  GetScaleForHWND((HWND)pParent);
  float width = w * mScale;
  float height = h * mScale;

  if (mScale > 1.)
    EditorResizeFromUI(width, height, true);
 
  auto retval = OpenWebView((HWND)pParent, 0, 0, (float)w, (float)h, mScale);
  
  return retval;
}

void WebViewEditorDelegate::Resize(int width, int height)
{
  SetWebViewBounds(0, 0, static_cast<float>(width), static_cast<float>(height));
  EditorResizeFromUI(width, height, true);
}
