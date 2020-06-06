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

/** @ingroup IControls */
class IWebViewControl : public IControl, public IWebView
{
public:
  IWebViewControl(const IRECT& bounds, bool opaque, onReadyFunc readyFunc, onMessageFunc msgFunc, const char* dllPath = "", const char* tmpPath = "")
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
    if(mOnReadyFunc)
      mOnReadyFunc(this);
  }
  
  void OnMessageFromWebView(const char* json) override
  {
    if(mOnMessageFunc)
      mOnMessageFunc(this, json);
  }

  void OnRescale() override
  {
    SetWebViewBounds(mRECT.L, mRECT.T, mRECT.W(), mRECT.H(), GetUI()->GetTotalScale());
  }

  void OnResize() override
  {
    SetWebViewBounds(mRECT.L, mRECT.T, mRECT.W(), mRECT.H(), GetUI()->GetTotalScale());
  }
  
private:
  void* mPlatformView = nullptr;
  onReadyFunc mOnReadyFunc;
  onMessageFunc mOnMessageFunc;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

