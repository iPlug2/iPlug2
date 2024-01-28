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

using OnReadyFunc = std::function<void(IWebViewControl* pControl)>;
using OnMessageFunc = std::function<void(IWebViewControl* pControl, const char* jsonMsg)>;

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
   * @param enableDevTools Should the webview developer tools be available via context menu */
  IWebViewControl(const IRECT& bounds, bool opaque, OnReadyFunc readyFunc = nullptr, OnMessageFunc msgFunc = nullptr, bool enableDevTools = false, bool enableScroll = false)
  : IControl(bounds)
  , IWebView(opaque)
  , mOnReadyFunc(readyFunc)
  , mOnMessageFunc(msgFunc)
  , mEnableDevTools(enableDevTools)
  , mEnableScroll(enableScroll)
  {
    // The IControl itself should never receive mouse messages
    // they need to go to the webview
    mIgnoreMouse = true;
  }
  
  ~IWebViewControl()
  {
    GetUI()->RemovePlatformView(mPlatformView);
    mPlatformView = nullptr;
  }
  
  void OnAttached() override
  {
    IGraphics* pGraphics = GetUI();
    mPlatformView = OpenWebView(pGraphics->GetWindow(), mRECT.L, mRECT.T, mRECT.W(), mRECT.H(), pGraphics->GetDrawScale(), mEnableDevTools);
    pGraphics->AttachPlatformView(mRECT, mPlatformView);
    EnableScroll(mEnableScroll);
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
  
  void OnWebContentLoaded() override
  {
    EnableInteraction(mEnableInteraction);
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
  
  void SetIgnoreMouse(bool ignore) override
  {
    // The IControl itself should never receive mouse messages
    // they need to go to the webview
    mEnableInteraction = !ignore;
    EnableInteraction(mEnableInteraction);
  }
  
  void Hide(bool hide) override
  {
    HideWebView(hide);

    if (mPlatformView)
      GetUI()->HidePlatformView(mPlatformView, hide);
    
    IControl::Hide(hide);
  }

private:
  void UpdateWebViewBounds()
  {
    auto ds = GetUI()->GetDrawScale();
    SetWebViewBounds(mRECT.L * ds, mRECT.T * ds, mRECT.W() * ds, mRECT.H() * ds, ds);
  }
  
  void* mPlatformView = nullptr;
  OnReadyFunc mOnReadyFunc;
  OnMessageFunc mOnMessageFunc;
  bool mEnableDevTools = false;
  bool mEnableInteraction = true;
  bool mEnableScroll = false;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

