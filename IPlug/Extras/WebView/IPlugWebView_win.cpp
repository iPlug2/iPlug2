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
#include "IPlugPaths.h"
#include <string>
#include <windows.h>
#include <wininet.h>
#include <shlobj.h>
#include <cassert>

#include <wrl.h>
#include <wil/com.h>
#include "WebView2.h"
#include "WebView2EnvironmentOptions.h"
#include "wdlstring.h"

extern float GetScaleForHWND(HWND hWnd);

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
  void GetLocalDownloadPathForFile(const char* fileName, WDL_String& downloadPath);

private:
  RECT GetScaledRect(float x, float y, float w, float h, float scale)
  {
    RECT r;
    r.left = static_cast<LONG>(std::ceil(x * scale));
    r.top = static_cast<LONG>(std::ceil(y * scale));
    r.right = static_cast<LONG>(std::ceil((x + w) * scale)) + 1;
    r.bottom = static_cast<LONG>(std::ceil((y + h) * scale)) + 1;
    return r;
  }

  IWebView* mIWebView;
  bool mOpaque;
  HWND mParentWnd = NULL;
  wil::com_ptr<ICoreWebView2Controller> mWebViewCtrlr;
  wil::com_ptr<ICoreWebView2> mCoreWebView;
  wil::com_ptr<ICoreWebView2Environment> mWebViewEnvironment;
  EventRegistrationToken mWebMessageReceivedToken;
  EventRegistrationToken mNavigationStartingToken;
  EventRegistrationToken mNavigationCompletedToken;
  EventRegistrationToken mNewWindowRequestedToken;
  EventRegistrationToken mDownloadStartingToken;
  EventRegistrationToken mBytesReceivedChangedToken;
  EventRegistrationToken mStateChangedToken;
  bool mShowOnLoad = true;
  WDL_String mWebRoot;
  RECT mWebViewBounds;
};

END_IPLUG_NAMESPACE

using namespace iplug;
using namespace Microsoft::WRL;

IWebViewImpl::IWebViewImpl(IWebView* owner)
  : mIWebView(owner)
{
}

IWebViewImpl::~IWebViewImpl()
{
  CloseWebView();
}

void* IWebViewImpl::OpenWebView(void* pParent, float,float,float,float,float)
{
  mParentWnd = (HWND)pParent;

  WDL_String cachePath;
  WebViewCachePath(cachePath);
  int bufSize = UTF8ToUTF16Len(cachePath.Get());
  std::vector<WCHAR> cachePathWide(bufSize);
  UTF8ToUTF16(cachePathWide.data(), cachePath.Get(), IPLUG_WIN_MAX_WIDE_PATH);

  auto options = Make<CoreWebView2EnvironmentOptions>();
  options->put_AllowSingleSignOnUsingOSPrimaryAccount(FALSE);
  options->put_ExclusiveUserDataFolderAccess(FALSE);
  // options->put_Language(m_language.c_str());
  options->put_IsCustomCrashReportingEnabled(FALSE);

  CreateCoreWebView2EnvironmentWithOptions(
    nullptr, cachePathWide.data(), options.Get(),
    Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>([&](
                                                                           HRESULT result,
                                                                           ICoreWebView2Environment* env) -> HRESULT {
      mWebViewEnvironment = env;

      mWebViewEnvironment->CreateCoreWebView2Controller(
        mParentWnd,
          Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>([&](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
            if (controller != nullptr)
            {
              mWebViewCtrlr = controller;
              mWebViewCtrlr->get_CoreWebView2(&mCoreWebView);
            }
            
            if (mCoreWebView == nullptr)
            {
              return S_OK;
            }

            mWebViewCtrlr->put_IsVisible(mShowOnLoad);

            const auto enableDevTools = mIWebView->GetEnableDevTools();

            ICoreWebView2Settings* Settings;
            mCoreWebView->get_Settings(&Settings);
            Settings->put_IsScriptEnabled(TRUE);
            Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
            Settings->put_IsWebMessageEnabled(TRUE);
            Settings->put_AreDefaultContextMenusEnabled(enableDevTools);
            Settings->put_AreDevToolsEnabled(enableDevTools);

            // this script adds a function IPlugSendMsg that is used to call the platform webview messaging function in JS
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
                mIWebView->OnMessageFromWebView(cStr.Get());
                return S_OK;
              }).Get(),
              &mWebMessageReceivedToken);


            mCoreWebView->add_NavigationStarting(
              Callback<ICoreWebView2NavigationStartingEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                  
                  wil::unique_cotaskmem_string uri;
                  args->get_Uri(&uri);
                  std::wstring uriUTF16 = uri.get();
                  WDL_String uriUTF8;
                  UTF16ToUTF8(uriUTF8, uriUTF16.c_str());
                  
                  if (mIWebView->OnCanNavigateToURL(uriUTF8.Get()) == false)
                  {
                    args->put_Cancel(TRUE);
                  }

                  return S_OK;
                }).Get(),
              &mNavigationStartingToken);

            mCoreWebView->add_NavigationCompleted(
              Callback<ICoreWebView2NavigationCompletedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2NavigationCompletedEventArgs* args) -> HRESULT {
                  BOOL success;
                  args->get_IsSuccess(&success);
                  if (success)
                  {
                    mIWebView->OnWebContentLoaded();
                  }
                  return S_OK;
                })
                .Get(),
              &mNavigationCompletedToken);

              mCoreWebView->add_NewWindowRequested(
              Callback<ICoreWebView2NewWindowRequestedEventHandler>(
                  [this](ICoreWebView2* sender, ICoreWebView2NewWindowRequestedEventArgs* args)
                    -> HRESULT 
              {
                wil::com_ptr<ICoreWebView2NewWindowRequestedEventArgs2> args2;

                if (SUCCEEDED(args->QueryInterface(IID_PPV_ARGS(&args2))))
                {
                  DWORD inetStatus = 0;
                  if (InternetGetConnectedState(&inetStatus, 0))
                  {
                    wil::unique_cotaskmem_string uri;

                    args2->get_Uri(&uri);

                    if (ShellExecuteW(mParentWnd, L"open", uri.get(), 0, 0, SW_SHOWNORMAL) > HINSTANCE(32))
                    {
                      args->put_Handled(true);
                    }
                  }
                }
                return S_OK;
              }).Get(),
                &mNewWindowRequestedToken);

              auto webView2_4 = mCoreWebView.try_query<ICoreWebView2_4>();
              if (webView2_4)
              {
                webView2_4->add_DownloadStarting(
                  Callback<ICoreWebView2DownloadStartingEventHandler>(
                  [this](
                    ICoreWebView2* sender,
                    ICoreWebView2DownloadStartingEventArgs* args) -> HRESULT {

                    // Hide the default download dialog.
                    args->put_Handled(TRUE);

                    wil::com_ptr<ICoreWebView2DownloadOperation> download;
                    args->get_DownloadOperation(&download);

                    INT64 totalBytesToReceive = 0;
                    download->get_TotalBytesToReceive(&totalBytesToReceive);

                    // validate MIME type
                    wil::unique_cotaskmem_string mimeType;
                    download->get_MimeType(&mimeType);
                    std::wstring mimeTypeUTF16 = mimeType.get();
                    WDL_String mimeTypeUTF8;
                    UTF16ToUTF8(mimeTypeUTF8, mimeTypeUTF16.c_str());
                    if (!mIWebView->OnCanDownloadMIMEType(mimeTypeUTF8.Get()))
                    {
                      args->put_Cancel(TRUE);
                    }

                    wil::unique_cotaskmem_string contentDisposition;
                    download->get_ContentDisposition(&contentDisposition);

                    // Modify download path
                    wil::unique_cotaskmem_string resultFilePath;
                    args->get_ResultFilePath(&resultFilePath);

                    std::wstring initialPathUTF16 = resultFilePath.get();
                    WDL_String initialPathUTF8, downloadPathUTF8;
                    UTF16ToUTF8(initialPathUTF8, initialPathUTF16.c_str());
                    mIWebView->OnGetLocalDownloadPathForFile(initialPathUTF8.Get(), downloadPathUTF8);

                    int bufSize = UTF8ToUTF16Len(downloadPathUTF8.Get());
                    std::vector<WCHAR> downloadPathWide(bufSize);
                    UTF8ToUTF16(downloadPathWide.data(), downloadPathUTF8.Get(), bufSize);

                    args->put_ResultFilePath(downloadPathWide.data());
                    
                    download->add_BytesReceivedChanged(Callback<ICoreWebView2BytesReceivedChangedEventHandler>([this](ICoreWebView2DownloadOperation* download, IUnknown* args) -> HRESULT {
                                                         INT64 bytesReceived, totalNumBytes;
                                                         download->get_BytesReceived(&bytesReceived);
                                                         download->get_TotalBytesToReceive(&totalNumBytes);
                                                         mIWebView->OnReceivedData(bytesReceived, totalNumBytes);
                          return S_OK;
                        }).Get(),
                        &mBytesReceivedChangedToken);

                        download->add_StateChanged(Callback<ICoreWebView2StateChangedEventHandler>([this](ICoreWebView2DownloadOperation* download, IUnknown* args) -> HRESULT {
                                                               COREWEBVIEW2_DOWNLOAD_STATE downloadState;
                                                               download->get_State(&downloadState);

                                                               auto onDownloadEnded = [&](ICoreWebView2DownloadOperation* download, bool success) {
                                                                 download->remove_BytesReceivedChanged(mBytesReceivedChangedToken);
                                                                 download->remove_StateChanged(mStateChangedToken);
                                                                 wil::unique_cotaskmem_string resultFilePath;
                                                                 download->get_ResultFilePath(&resultFilePath);
                                                                 std::wstring downloadPathUTF16 = resultFilePath.get();
                                                                 WDL_String downloadPathUTF8;
                                                                 UTF16ToUTF8(downloadPathUTF8, downloadPathUTF16.c_str());

                                                                 if (success) {
                                                                  mIWebView->OnDownloadedFile(downloadPathUTF8.Get());
                                                                 }
                                                                 else {
                                                                  mIWebView->OnFailedToDownloadFile(downloadPathUTF8.Get());
                                                                 }
                                                               };

                                                               switch (downloadState)
                                                               {
                                                               case COREWEBVIEW2_DOWNLOAD_STATE_IN_PROGRESS:
                                                               // TODO
                                                                 break;
                                                               case COREWEBVIEW2_DOWNLOAD_STATE_INTERRUPTED:
                                                                 onDownloadEnded(download, false);
                                                                 break;
                                                               case COREWEBVIEW2_DOWNLOAD_STATE_COMPLETED:
                                                                 onDownloadEnded(download, true);
                                                                 break;
                                                               }
                                                               return S_OK;
                                                             }).Get(),
                                                             &mStateChangedToken);

                    return S_OK;
                  })
                  .Get(),
              &mDownloadStartingToken);
            }

            if (!mOpaque)
            {
              wil::com_ptr<ICoreWebView2Controller2> controller2 = mWebViewCtrlr.query<ICoreWebView2Controller2>();
              COREWEBVIEW2_COLOR color;
              memset(&color, 0, sizeof(COREWEBVIEW2_COLOR));
              controller2->put_DefaultBackgroundColor(color);
            }

            mWebViewCtrlr->put_Bounds(mWebViewBounds);
            mIWebView->OnWebViewReady();
            return S_OK;
          })
          .Get());

      return S_OK;
    }).Get());

  return mParentWnd;
}

void IWebViewImpl::CloseWebView()
{
  if (mWebViewCtrlr.get() != nullptr)
  {
    mWebViewCtrlr->Close();
    mWebViewCtrlr = nullptr;
    mCoreWebView = nullptr;
    mWebViewEnvironment = nullptr;
  }
}

void IWebViewImpl::HideWebView(bool hide)
{
  if (mWebViewCtrlr.get() != nullptr)
  {
    mWebViewCtrlr->put_IsVisible(!hide);
  }
  else
  {
    // the controller is set asynchonously, so we store the state 
    // to apply it when the controller is created
    mShowOnLoad = !hide;
  }
}

void IWebViewImpl::LoadHTML(const char* html)
{
  if (mCoreWebView)
  {
    int bufSize = UTF8ToUTF16Len(html);
    std::vector<WCHAR> htmlWide(bufSize);
    UTF8ToUTF16(htmlWide.data(), html, bufSize);
    mCoreWebView->NavigateToString(htmlWide.data());
  }
}

void IWebViewImpl::LoadURL(const char* url)
{
  if (mCoreWebView)
  {
    int bufSize = UTF8ToUTF16Len(url);
    std::vector<WCHAR> urlWide(bufSize);
    UTF8ToUTF16(urlWide.data(), url, bufSize);
    mCoreWebView->Navigate(urlWide.data());
  }
}

void IWebViewImpl::LoadFile(const char* fileName, const char* bundleID)
{
  if (mCoreWebView)
  {
    wil::com_ptr<ICoreWebView2_3> webView3 = mCoreWebView.try_query<ICoreWebView2_3>();
    if (webView3)
    {
      WDL_String webFolder{fileName};
      webFolder.remove_filepart();
      int bufSize = UTF8ToUTF16Len(webFolder.Get());
      std::vector<WCHAR> webFolderWide(bufSize);
      UTF8ToUTF16(webFolderWide.data(), webFolder.Get(), bufSize);

      webView3->SetVirtualHostNameToFolderMapping(
        L"iplug.example", webFolderWide.data(), COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY_CORS);
    }

    WDL_String baseName{fileName};
    WDL_String root{fileName};
    root.remove_filepart();
    mWebRoot.Set(root.Get());

    WDL_String fullStr;
    fullStr.SetFormatted(2048, "https://iplug.example/%s", baseName.get_filepart());
    // fullStr.SetFormatted(2048, useCustomUrlScheme ? "iplug://%s" : "file://%s", fileName);
    int bufSize = UTF8ToUTF16Len(fullStr.Get());
    std::vector<WCHAR> fileUrlWide(bufSize);
    UTF8ToUTF16(fileUrlWide.data(), fullStr.Get(), bufSize);
    mCoreWebView->Navigate(fileUrlWide.data());
  }
}


void IWebViewImpl::ReloadPageContent()
{
  if (mCoreWebView)
  {
    mCoreWebView->Reload();
  }
}

void IWebViewImpl::EvaluateJavaScript(const char* scriptStr, IWebView::completionHandlerFunc func)
{
  if (mCoreWebView)
  {
    int bufSize = UTF8ToUTF16Len(scriptStr);
    std::vector<WCHAR> scriptWide(bufSize);
    UTF8ToUTF16(scriptWide.data(), scriptStr, bufSize);

    mCoreWebView->ExecuteScript(
      scriptWide.data(), Callback<ICoreWebView2ExecuteScriptCompletedHandler>([func](HRESULT errorCode,
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

void IWebViewImpl::EnableScroll(bool enable)
{
  /* NO-OP */
}

void IWebViewImpl::EnableInteraction(bool enable)
{
  /* NO-OP */
}

void IWebViewImpl::SetWebViewBounds(float x, float y, float w, float h, float scale)
{
  mWebViewBounds = GetScaledRect(x, y, w, h, GetScaleForHWND(mParentWnd));

  if (mWebViewCtrlr)
  {
    mWebViewCtrlr->SetBoundsAndZoomFactor(mWebViewBounds, scale);
  }
}

void IWebViewImpl::GetLocalDownloadPathForFile(const char* fileName, WDL_String& downloadPath)
{
  DesktopPath(downloadPath);
  downloadPath.Append(fileName);
}

#include "IPlugWebView.cpp"