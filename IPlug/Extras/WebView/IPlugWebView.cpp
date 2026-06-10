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

#include "IPlugWebView.h"

#include <memory>

using namespace iplug;

IWebView::IWebView(bool opaque, bool enableDevTools, const char* customUrlScheme)
: mpImpl(std::make_unique<IWebViewImpl>(this))
, mOpaque(opaque)
{
  SetCustomUrlScheme(customUrlScheme);
  SetEnableDevTools(enableDevTools);
}

IWebView::~IWebView() = default;

void* IWebView::OpenWebView(void* pParent, float x, float y, float w, float h, float scale)
{
  return mpImpl->OpenWebView(pParent, x, y, w, h, scale);
}

void IWebView::CloseWebView()
{
  mpImpl->CloseWebView();
}

void IWebView::HideWebView(bool hide)
{
  mpImpl->HideWebView(hide);
}

void IWebView::LoadHTML(const char* html)
{
  mpImpl->LoadHTML(html);
}

void IWebView::LoadURL(const char* url)
{
  mpImpl->LoadURL(url);
}

void IWebView::LoadFile(const char* fileName, const char* bundleID)
{
  mpImpl->LoadFile(fileName, bundleID);
}

void IWebView::ReloadPageContent()
{
  mpImpl->ReloadPageContent();
}

void IWebView::EvaluateJavaScript(const char* scriptStr, completionHandlerFunc func)
{
  mpImpl->EvaluateJavaScript(scriptStr, func);
}

void IWebView::EnableScroll(bool enable)
{
  mpImpl->EnableScroll(enable);
}

void IWebView::EnableInteraction(bool enable)
{
  mpImpl->EnableInteraction(enable);
}

void IWebView::SetWebViewBounds(float x, float y, float w, float h, float scale)
{
  mpImpl->SetWebViewBounds(x, y, w, h, scale);
}

void IWebView::OnGetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath)
{
  // the default implementation here calls a Pimpl function
  mpImpl->GetLocalDownloadPathForFile(fileName, localPath);
}

void IWebView::GetWebRoot(WDL_String& path) const
{
  mpImpl->GetWebRoot(path);
}
