/*
 ==============================================================================

 iPlug2 WebView Library
 Copyright (c) 2026 Oliver Larkin

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

#include <emscripten.h>
#include <emscripten/bind.h>

#include <cstdint>
#include <string>

BEGIN_IPLUG_NAMESPACE

class IWebViewImpl
{
public:
  IWebViewImpl(IWebView* owner);
  ~IWebViewImpl();

  void* OpenWebView(void* pParent, float x, float y, float w, float h, float scale);
  void CloseWebView();
  void HideWebView(bool hide);
  void LoadHTML(const char* html);
  void LoadURL(const char* url);
  void LoadFile(const char* fileName, const char* bundleID);
  void ReloadPageContent();
  void EvaluateJavaScript(const char* scriptStr, IWebView::completionHandlerFunc func);
  void EnableScroll(bool enable);
  void EnableInteraction(bool enable);
  void SetWebViewBounds(float x, float y, float w, float h, float scale);
  void GetWebRoot(WDL_String& path) const { path.Set(mWebRoot.Get()); }
  void GetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath);

private:
  IWebView* mIWebView = nullptr;
  WDL_String mWebRoot;
  bool mOpen = false;
};

END_IPLUG_NAMESPACE

using namespace iplug;

namespace
{
uintptr_t OwnerPtr(IWebView* pWebView)
{
  return reinterpret_cast<uintptr_t>(pWebView);
}

void OnMessageFromWebView(uintptr_t ownerPtr, const std::string& json)
{
  if (auto* pWebView = reinterpret_cast<IWebView*>(ownerPtr))
  {
    pWebView->OnMessageFromWebView(json.c_str());
  }
}

void OnWebContentLoaded(uintptr_t ownerPtr)
{
  if (auto* pWebView = reinterpret_cast<IWebView*>(ownerPtr))
  {
    pWebView->OnWebContentLoaded();
  }
}
}

IWebViewImpl::IWebViewImpl(IWebView* owner)
: mIWebView(owner)
{
  mWebRoot.Set("web");
}

IWebViewImpl::~IWebViewImpl()
{
  CloseWebView();
}

void* IWebViewImpl::OpenWebView(void* pParent, float x, float y, float w, float h, float scale)
{
  (void) pParent;
  mOpen = true;

  EM_ASM({
    if (typeof window.__iPlugWebViewOpen === 'function') {
      window.__iPlugWebViewOpen($0, $1, $2, $3, $4, $5);
    }
  }, OwnerPtr(mIWebView), x, y, w, h, scale);

  mIWebView->OnWebViewReady();
  return reinterpret_cast<void*>(mIWebView);
}

void IWebViewImpl::CloseWebView()
{
  if (!mOpen)
    return;

  EM_ASM({
    if (typeof window.__iPlugWebViewClose === 'function') {
      window.__iPlugWebViewClose($0);
    }
  }, OwnerPtr(mIWebView));

  mOpen = false;
}

void IWebViewImpl::HideWebView(bool hide)
{
  EM_ASM({
    if (typeof window.__iPlugWebViewHide === 'function') {
      window.__iPlugWebViewHide($0, !!$1);
    }
  }, OwnerPtr(mIWebView), hide);
}

void IWebViewImpl::LoadHTML(const char* html)
{
  EM_ASM({
    if (typeof window.__iPlugWebViewLoadHTML === 'function') {
      window.__iPlugWebViewLoadHTML($0, UTF8ToString($1));
    }
  }, OwnerPtr(mIWebView), html ? html : "");
}

void IWebViewImpl::LoadURL(const char* url)
{
  EM_ASM({
    if (typeof window.__iPlugWebViewLoadURL === 'function') {
      window.__iPlugWebViewLoadURL($0, UTF8ToString($1));
    }
  }, OwnerPtr(mIWebView), url ? url : "");
}

void IWebViewImpl::LoadFile(const char* fileName, const char* bundleID)
{
  (void) bundleID;

  WDL_String filePart;
  if (fileName && fileName[0] != '\0')
  {
    WDL_String requestedFile(fileName);
    filePart.Set(requestedFile.get_filepart());
  }

  if (filePart.GetLength() == 0)
  {
    filePart.Set("index.html");
  }

  mWebRoot.Set("web");

  WDL_String webFile;
  webFile.SetFormatted(1024, "web/%s", filePart.Get());

  EM_ASM({
    if (typeof window.__iPlugWebViewLoadFile === 'function') {
      window.__iPlugWebViewLoadFile($0, UTF8ToString($1), UTF8ToString($2));
    }
  }, OwnerPtr(mIWebView), webFile.Get(), mWebRoot.Get());
}

void IWebViewImpl::ReloadPageContent()
{
  EM_ASM({
    if (typeof window.__iPlugWebViewReload === 'function') {
      window.__iPlugWebViewReload($0);
    }
  }, OwnerPtr(mIWebView));
}

void IWebViewImpl::EvaluateJavaScript(const char* scriptStr, IWebView::completionHandlerFunc func)
{
  EM_ASM({
    if (typeof window.__iPlugWebViewEvaluate === 'function') {
      window.__iPlugWebViewEvaluate($0, UTF8ToString($1));
    }
  }, OwnerPtr(mIWebView), scriptStr ? scriptStr : "");

  if (func)
  {
    func("");
  }
}

void IWebViewImpl::EnableScroll(bool enable)
{
  EM_ASM({
    if (typeof window.__iPlugWebViewEnableScroll === 'function') {
      window.__iPlugWebViewEnableScroll($0, !!$1);
    }
  }, OwnerPtr(mIWebView), enable);
}

void IWebViewImpl::EnableInteraction(bool enable)
{
  EM_ASM({
    if (typeof window.__iPlugWebViewEnableInteraction === 'function') {
      window.__iPlugWebViewEnableInteraction($0, !!$1);
    }
  }, OwnerPtr(mIWebView), enable);
}

void IWebViewImpl::SetWebViewBounds(float x, float y, float w, float h, float scale)
{
  EM_ASM({
    if (typeof window.__iPlugWebViewSetBounds === 'function') {
      window.__iPlugWebViewSetBounds($0, $1, $2, $3, $4, $5);
    }
  }, OwnerPtr(mIWebView), x, y, w, h, scale);
}

void IWebViewImpl::GetLocalDownloadPathForFile(const char* fileName, WDL_String& localPath)
{
  localPath.Set(fileName ? fileName : "");
}

EMSCRIPTEN_BINDINGS(IPlugWebViewWeb) {
  emscripten::function("IPlugWebView_OnMessage", &OnMessageFromWebView);
  emscripten::function("IPlugWebView_OnContentLoaded", &OnWebContentLoaded);
}

#include "IPlugWebView.cpp"
