/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IAboutBoxControl
 */

#include "IControls.h"

#include "IPlugPluginBase.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A "meta control" which you can attach child controls to for "about box" info
 * and optionally animate fade hiding/showing.
 * Should probably be added last so it sits on top of other controls,
 * and set hidden when attached
 * @ingroup IControls */
class IAboutBoxControl : public IPanelControl
{
public:
  IAboutBoxControl(const IRECT& bounds, const IPattern& color,
                   AttachFunc attachFunc = nullptr, ResizeFunc resizeFunc = nullptr,
                   double animationTime = 200)
  : IPanelControl(bounds, color, false, attachFunc, resizeFunc)
  , mAnimationTime(animationTime)
  {
    mIgnoreMouse = false;
  }
  
  bool OnKeyDown(float x, float y, const IKeyPress& key) override
  {
    if (key.VK == kVK_ESCAPE)
    {
      HideAnimated(true);
      return true;
    }
    
    return false;
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    HideAnimated(true);
  }
  
  void HideAnimated(bool hide)
  {
    mWillHide = hide;

    if (hide == false)
    {
      mHide = false;
    }
    else // hide subcontrols immediately
    {
      ForAllChildrenFunc([hide](int childIdx, IControl* pChild) {
        pChild->Hide(hide);
      });
    }

    SetAnimation([&](IControl* pCaller) {
      auto progress = pCaller->GetAnimationProgress();

      if (mWillHide)
        SetBlend(IBlend(EBlend::Default, 1.0-progress));
      else
        SetBlend(IBlend(EBlend::Default, progress));

      if (progress > 1.) {
        pCaller->OnEndAnimation();
        IPanelControl::Hide(mWillHide);
        GetUI()->SetAllControlsDirty();
        return;
      }

    }, mAnimationTime);

    SetDirty(true);
  }
  
private:
  double mAnimationTime = 200;
  bool mWillHide = false;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE


