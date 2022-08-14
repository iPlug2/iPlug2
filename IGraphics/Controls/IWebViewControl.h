/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup Controls
 * @copydoc IWebViewControl
 */

#include "IControl.h"
#include "IPlugWebView.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IWebViewControl;

using onReadyFunc = std::function<void(IWebViewControl* pWebView)>;
using onMessageFunc = std::function<void(IWebViewControl* pWebView, const char* jsonMsg)>;

/** A control that allows the embedding of HTML UI inside an IGraphics context using a platform-native webview and bi-directional communication with that content
 * NOTE: this control attaches a sub view on top of the IGraphics view, so it will render all content on-top
 * The platform native webviews run in a separate process and communicate via IPC.
 * On Windows the ICoreWebView2 is used, which requires Edge Chromium to be installed: https://docs.microsoft.com/en-us/microsoft-edge/webview2
 * On macOS and iOS WkWebView is used
 * @ingroup IControls */
class IWebViewControl : public IControl, public IWebView
{
public:
  /** Constructs am IWebViewControl
   * @param bounds The control's bounds
   * @param opaque Should the web view background be opaque
   * @param readyFunc A function conforming to onReadyFunc, that will be called asyncronously when the webview has been initialized
   * @param msgFunc A function conforming to onMessageFunc, that will be called when messages are posted from the webview
   * @param dllPath (Windows only) an absolute path to the WebView2Loader.dll that is required to use the WebView2 on windows
   * @param tmpPath (Windows only) an absolute path to the folder that should be used */
  IWebViewControl(const IRECT& bounds, bool opaque, onReadyFunc readyFunc, onMessageFunc msgFunc = nullptr, const char* dllPath = "", const char* tmpPath = "")
  : IControl(bounds)
  , IWebView(opaque)
  , mOnReadyFunc(readyFunc)
  , mOnMessageFunc(msgFunc)
  {
#ifdef OS_WIN
    SetWebViewPaths(dllPath, tmpPath);
#endif
  }
  
  ~IWebViewControl()
  {
    GetUI()->RemovePlatformView(mPlatformView);
    mPlatformView = nullptr;
  }
  
  void OnAttached() override
  {
    mPlatformView = OpenWebView(GetUI()->GetWindow(), mRECT.L, mRECT.T, mRECT.W(), mRECT.H(), GetUI()->GetTotalScale());
    GetUI()->AttachPlatformView(mRECT, mPlatformView);
  }
  
  void Draw(IGraphics& g) override
  {
     /* NO-OP */
  }

  void OnWebViewReady() override
  {
    if (mOnReadyFunc)
      mOnReadyFunc(this);
  }
  
  void OnMessageFromWebView(const char* json) override
  {
    if (mOnMessageFunc)
      mOnMessageFunc(this, json);
  }

  void OnRescale() override
  {
    UpdateWebViewBounds();
  }

  void OnResize() override
  {
    UpdateWebViewBounds();
  }
  
private:
  void UpdateWebViewBounds()
  {
    auto ds = GetUI()->GetDrawScale();
    SetWebViewBounds(mRECT.L * ds, mRECT.T * ds, mRECT.W() * ds, mRECT.H() * ds, ds);
  }
  
  void* mPlatformView = nullptr;
  onReadyFunc mOnReadyFunc;
  onMessageFunc mOnMessageFunc;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

