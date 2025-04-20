 /*
 ==============================================================================
 
  MIT License

  iPlug2 WebView Library
  Copyright (c) 2024 Oliver Larkin

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
 
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

void* WebViewEditorDelegate::OpenWindow(void* pParent)
{
  mView = OpenWebView(pParent, 0.0f, 0.0f,
    static_cast<float>(GetEditorWidth()),
    static_cast<float>(GetEditorHeight()), 1.0f);

  return mView;
}

void WebViewEditorDelegate::SetScreenScale(float scale)
{
  int w = static_cast<int>(GetEditorWidth() * scale);
  int h = static_cast<int>(GetEditorHeight() * scale);

  EditorResizeFromUI(w, h, true);
}

void WebViewEditorDelegate::Resize(int width, int height)
{
  SetWebViewBounds(0, 0, static_cast<float>(width), static_cast<float>(height));
  EditorResizeFromUI(width, height, true);
}

void WebViewEditorDelegate::OnParentWindowResize(int width, int height)
{
  SetWebViewBounds(0, 0, static_cast<float>(width), static_cast<float>(height));
  EditorResizeFromUI(width, height, false);
}

bool WebViewEditorDelegate::OnKeyDown(const IKeyPress& key)
{
  #ifdef OS_WIN
  if (key.VK == VK_SPACE)
  {
    PostMessage((HWND)mView, WM_KEYDOWN, VK_SPACE, 0);
    return true;
  }
  #endif
  return false;
}

bool WebViewEditorDelegate::OnKeyUp(const IKeyPress& key)
{

  return true;
}