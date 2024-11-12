/*
==============================================================================

This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

See LICENSE.txt for  more info.

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
