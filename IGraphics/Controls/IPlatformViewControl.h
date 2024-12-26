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
 * @copydoc IPlatformViewControl
 */

#include "IControl.h"
#include "IPlugPlatformView.h"

#include <functional>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A control that let's you embed a HWND, UIView or NSView inside an IGraphics UI
 * @ingroup IControls */
class IPlatformViewControl : public IControl, public IPlatformView
{
public:
  using AttachFunc = std::function<void(void* pParentView)>;

  /** Constructs an IPlatformViewControl
   * @param bounds The control's bounds
   * @param opaque Should the  view's background be opaque 
   * @param attachFunc If you want to attach the sub views in-line */
  IPlatformViewControl(const IRECT& bounds, bool opaque = true, AttachFunc attachFunc = nullptr)
  : IControl(bounds)
  , IPlatformView(opaque)
  , mAttachFunc(attachFunc)
  {
  }
  
  ~IPlatformViewControl()
  {
    GetUI()->RemovePlatformView(mPlatformView);
    mPlatformView = nullptr;
  }
  
  void OnAttached() override
  {
    mPlatformView = CreatePlatformView(GetUI()->GetWindow(), mRECT.L, mRECT.T, mRECT.W(), mRECT.H(), GetUI()->GetTotalScale());
    GetUI()->AttachPlatformView(mRECT, mPlatformView);
    AttachSubViews(mPlatformView);
  }
  
  void Draw(IGraphics& g) override { /* NO-OP */ }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override { /* NO-OP */ }

  void OnRescale() override
  {
    UpdateChildViewBounds();
  }

  void OnResize() override
  {
    UpdateChildViewBounds();
  }
  
  /* Override this method in separate .cpp or .mm files if you want to keep
   * objective C out of you main layout function
   */
  virtual void AttachSubViews(void* pPlatformView)
  {
    if (mAttachFunc)
      mAttachFunc(pPlatformView);
  }
  
  void* GetPlatformView()
  {
    return mPlatformView;
  }

  void Hide(bool hide) override
  {
    if (mPlatformView)
      GetUI()->HidePlatformView(mPlatformView, hide);
    
    IControl::Hide(hide);
  }
  
private:
  void UpdateChildViewBounds()
  {
    auto ds = GetUI()->GetDrawScale();
    SetChildViewBounds(mRECT.L * ds, mRECT.T * ds, mRECT.W() * ds, mRECT.H() * ds, ds);
  }
  
  void* mPlatformView = nullptr;
  AttachFunc mAttachFunc;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

