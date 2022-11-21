/*
==============================================================================

This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

See LICENSE.txt for  more info.

==============================================================================
*/

#include "IPlugWebView.h"
#include "IPlugPaths.h"
#include <string>
#include <windows.h>
#include <shlobj.h>
//#include <shlwapi.h>
#include <cassert>

using namespace iplug;
using namespace Microsoft::WRL;

extern float GetScaleForHWND(HWND hWnd);

IWebView::IWebView(bool opaque)
: mOpaque(opaque)
{
}

IWebView::~IWebView()
{
  CloseWebView();
}

typedef HRESULT (*TCCWebView2EnvWithOptions)(
  PCWSTR browserExecutableFolder, PCWSTR userDataFolder, PCWSTR additionalBrowserArguments,
  ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environment_created_handler);

void* IWebView::OpenWebView(void* pParent, float x, float y, float w, float h, float scale, bool enableDevTools)
{
  mParentWnd = (HWND)pParent;

  float ss = GetScaleForHWND(mParentWnd);

  x *= ss;
  y *= ss;
  w *= ss;
  h *= ss;

  WDL_String cachePath;
  WebViewCachePath(cachePath);
  WCHAR cachePathWide[IPLUG_WIN_MAX_WIDE_PATH];
  UTF8ToUTF16(cachePathWide, cachePath.Get(), IPLUG_WIN_MAX_WIDE_PATH);

  auto options = Microsoft::WRL::Make<CoreWebView2EnvironmentOptions>();
  options->put_AllowSingleSignOnUsingOSPrimaryAccount(FALSE);
  options->put_ExclusiveUserDataFolderAccess(FALSE);
  // options->put_Language(m_language.c_str());
  options->put_IsCustomCrashReportingEnabled(FALSE);

  // Microsoft::WRL::ComPtr<ICoreWebView2EnvironmentOptions4> options4;
  // if (options.As(&options4) == S_OK)
  //{
  //   // const WCHAR* allowedOrigins[1] = {L"https://*.example.com"};
  //   auto customSchemeRegistration = Microsoft::WRL::Make<CoreWebView2CustomSchemeRegistration>(L"iplug");
  //   customSchemeRegistration->put_TreatAsSecure(TRUE);
  //   // customSchemeRegistration->SetAllowedOrigins(1, allowedOrigins);
  //   customSchemeRegistration->put_HasAuthorityComponent(TRUE);
  //   ICoreWebView2CustomSchemeRegistration *registrations[1] = {customSchemeRegistration.Get()};
  //   options4->SetCustomSchemeRegistrations(1, static_cast<ICoreWebView2CustomSchemeRegistration **>(registrations));
  // }

  CreateCoreWebView2EnvironmentWithOptions(
    nullptr, cachePathWide, options.Get(),
    Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([&, x, y, w, h](
                                                                           HRESULT result,
                                                                           ICoreWebView2Environment* env) -> HRESULT {
      mWebViewEnvironment = env;
      mWebViewEnvironment->CreateCoreWebView2Controller(
        mParentWnd,
        Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
          [&, x, y, w, h](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
            if (controller != nullptr)
            {
              mWebViewCtrlr = controller;
              mWebViewCtrlr->get_CoreWebView2(&mCoreWebView);
            }

            ICoreWebView2Settings* Settings;
            mCoreWebView->get_Settings(&Settings);
            Settings->put_IsScriptEnabled(TRUE);
            Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
            Settings->put_IsWebMessageEnabled(TRUE);
            // Settings->put_AreDefaultContextMenusEnabled(FALSE);
            Settings->put_AreDevToolsEnabled(enableDevTools);

            // this script adds a function IPlugSendMsg that is used to call the platform webview messaging function in
            // JS
            mCoreWebView->AddScriptToExecuteOnDocumentCreated(
              L"function IPlugSendMsg(m) {window.chrome.webview.postMessage(m)};",
              Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>([this](HRESULT error,
                                                                                                PCWSTR id) -> HRESULT {
                return S_OK;
              }).Get());

            mCoreWebView->add_WebMessageReceived(
              Callback<ICoreWebView2WebMessageReceivedEventHandler>([this](
                                                                      ICoreWebView2* sender,
                                                                      ICoreWebView2WebMessageReceivedEventArgs* args) {
                wil::unique_cotaskmem_string jsonString;
                args->get_WebMessageAsJson(&jsonString);
                std::wstring jsonWString = jsonString.get();
                WDL_String cStr;
                UTF16ToUTF8(cStr, jsonWString.c_str());
                OnMessageFromWebView(cStr.Get());
                return S_OK;
              }).Get(),
              &mWebMessageReceivedToken);

            mCoreWebView->add_NavigationCompleted(
              Callback<ICoreWebView2NavigationCompletedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                  BOOL success;
                  args->get_IsSuccess(&success);
                  if (success)
                  {
                    OnWebContentLoaded();
                  }
                  return S_OK;
                })
                .Get(),
              &mNavigationCompletedToken);

            // mCoreWebView->add_ContainsFullScreenElementChanged(
            //     Callback<ICoreWebView2ContainsFullScreenElementChangedEventHandler>([this](ICoreWebView2 *sender,
            //     IUnknown *args) -> HRESULT
            //     {
            //       sender->get_ContainsFullScreenElement(&mContainsFullscreenElement);
            //       if (mContainsFullscreenElement)
            //       {
            //       }
            //       else
            //       {
            //       }
            //       return S_OK; })
            //       .Get(),
            //  nullptr);

            // mCoreWebView->AddWebResourceRequestedFilter(L"iplug*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
            // mCoreWebView->add_WebResourceRequested(
            //   Callback<ICoreWebView2WebResourceRequestedEventHandler>([this](ICoreWebView2 *sender,
            //   ICoreWebView2WebResourceRequestedEventArgs *args)
            //   {
            //     wil::com_ptr<ICoreWebView2WebResourceRequest> request;
            //     wil::com_ptr<ICoreWebView2WebResourceResponse> response;
            //     args->get_Request(&request);
            //     wil::com_ptr<IStream> content;
            //     request->get_Content(&content);
            //     wil::unique_cotaskmem_string uri;
            //     request->get_Uri(&uri);
            //     if (wcsncmp(uri.get(), L"iplug://", ARRAYSIZE(L"iplug://") - 1) == 0)
            //     {
            //     std::wstring filePath = uri.get();
            //     filePath.erase(0, ARRAYSIZE(L"iplug://") - 1);
            //     wil::com_ptr<IStream> stream;
            //     SHCreateStreamOnFileEx(filePath.c_str(), STGM_READ, FILE_ATTRIBUTE_NORMAL, FALSE, nullptr, &stream);
            //     if (stream)
            //     {
            //       std::wstring headers;
            //       if (filePath.substr(filePath.find_last_of(L".") + 1) == L"html")
            //       {
            //         headers = L"Content-Type: text/html";
            //       }
            //       else if (filePath.substr(filePath.find_last_of(L".") + 1) == L"jpg")
            //       {
            //         headers = L"Content-Type: image/jpeg";
            //       }
            //       else if (filePath.substr(filePath.find_last_of(L".") + 1) == L"png")
            //       {
            //         headers = L"Content-Type: image/png";
            //       }
            //       else if (filePath.substr(filePath.find_last_of(L".") + 1) == L"css")
            //       {
            //         headers = L"Content-Type: text/css";
            //       }
            //       else if (filePath.substr(filePath.find_last_of(L".") + 1) == L"js")
            //       {
            //         headers = L"Content-Type: application/javascript";
            //       }

            //       mWebViewEnvironment->CreateWebResourceResponse(stream.get(), 200, L"OK", headers.c_str(),
            //       &response); args->put_Response(response.get());
            //     }
            //     else
            //     {
            //       mWebViewEnvironment->CreateWebResourceResponse(nullptr, 404, L"Not Found", L"", &response);
            //       args->put_Response(response.get());
            //     }
            //     return S_OK;
            //     }

            //   return S_OK; }).Get(), &mWebResourceRequestedToken);

            if (!mOpaque)
            {
              wil::com_ptr<ICoreWebView2Controller2> controller2 = mWebViewCtrlr.query<ICoreWebView2Controller2>();
              COREWEBVIEW2_COLOR color;
              memset(&color, 0, sizeof(COREWEBVIEW2_COLOR));
              controller2->put_DefaultBackgroundColor(color);
            }

            mWebViewCtrlr->put_Bounds({(LONG)x, (LONG)y, (LONG)(x + w), (LONG)(y + h)});
            OnWebViewReady();
            return S_OK;
          })
          .Get());

      return S_OK;
    }).Get());

  return mParentWnd;
}

void IWebView::CloseWebView()
{
  if (mWebViewCtrlr.get() != nullptr)
  {
    mWebViewCtrlr->Close();
    mWebViewCtrlr = nullptr;
    mCoreWebView = nullptr;
    mWebViewEnvironment = nullptr;
  }
}

void IWebView::HideWebView(bool hide)
{
  if (mWebViewCtrlr.get() != nullptr)
  {
    mWebViewCtrlr->put_IsVisible(!hide);
  }
}

void IWebView::LoadHTML(const char* html)
{
  if (mCoreWebView)
  {
    WCHAR htmlWide[IPLUG_WIN_MAX_WIDE_PATH];
    UTF8ToUTF16(htmlWide, html, IPLUG_WIN_MAX_WIDE_PATH);
    mCoreWebView->NavigateToString(htmlWide);
  }
}

void IWebView::LoadURL(const char* url)
{
  // TODO: error check url?
  if (mCoreWebView)
  {
    WCHAR urlWide[IPLUG_WIN_MAX_WIDE_PATH];
    UTF8ToUTF16(urlWide, url, IPLUG_WIN_MAX_WIDE_PATH);
    mCoreWebView->Navigate(urlWide);
  }
}

void IWebView::LoadFile(const char* fileName, const char* bundleID, bool /*useCustomScheme*/)
{
  if (mCoreWebView)
  {
    wil::com_ptr<ICoreWebView2_3> webView3 = mCoreWebView.try_query<ICoreWebView2_3>();
    if (webView3)
    {
      WDL_String webFolder{fileName};
      webFolder.remove_filepart();
      WCHAR webFolderWide[IPLUG_WIN_MAX_WIDE_PATH];
      UTF8ToUTF16(webFolderWide, webFolder.Get(), IPLUG_WIN_MAX_WIDE_PATH);
      webView3->SetVirtualHostNameToFolderMapping(
        L"iplug.example", webFolderWide, COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);
    }

    WDL_String baseName{fileName};
    WDL_String fullStr;
    fullStr.SetFormatted(MAX_WIN32_PATH_LEN, "https://iplug.example/%s", baseName.get_filepart());
    // fullStr.SetFormatted(MAX_WIN32_PATH_LEN, useCustomScheme ? "iplug://%s" : "file://%s", fileName);
    WCHAR fileUrlWide[IPLUG_WIN_MAX_WIDE_PATH];
    UTF8ToUTF16(fileUrlWide, fullStr.Get(), IPLUG_WIN_MAX_WIDE_PATH);
    mCoreWebView->Navigate(fileUrlWide);
  }
}

void IWebView::EvaluateJavaScript(const char* scriptStr, completionHandlerFunc func)
{
  if (mCoreWebView)
  {
    WCHAR scriptWide[IPLUG_WIN_MAX_WIDE_PATH];
    UTF8ToUTF16(scriptWide, scriptStr, IPLUG_WIN_MAX_WIDE_PATH);

    mCoreWebView->ExecuteScript(
      scriptWide, Callback<ICoreWebView2ExecuteScriptCompletedHandler>([func](HRESULT errorCode,
                                                                              LPCWSTR resultObjectAsJson) -> HRESULT {
                    if (func && resultObjectAsJson)
                    {
                      WDL_String str;
                      UTF16ToUTF8(str, resultObjectAsJson);
                      func(str.Get());
                    }
                    return S_OK;
                  }).Get());
  }
}

void IWebView::EnableScroll(bool enable)
{
  // TODO?
}

void IWebView::SetEnableInteraction(bool enable)
{
  // TODO?
}

void IWebView::SetWebViewBounds(float x, float y, float w, float h, float scale)
{
  if (mWebViewCtrlr)
  {
    float ss = GetScaleForHWND(mParentWnd);

    x *= ss;
    y *= ss;
    w *= ss;
    h *= ss;

    mWebViewCtrlr->SetBoundsAndZoomFactor({(LONG)x, (LONG)y, (LONG)(x + w), (LONG)(y + h)}, scale);
  }
}
