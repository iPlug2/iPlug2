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

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A "meta control" which you can attach child controls to display an about box, or preferences panel
 * When you attach the control to the IGraphics context, it should be added last so it sits on top of other controls,
 * and set hidden. You can then call Show() to reveal it. When the control is dismissed, there is no animation.
 *  * Basic usage example:
 * \code{.cpp}
 *  pGraphics->AttachControl(new IAboutBoxControl(b,
 *  COLOR_BLACK,
 *  // AttachFunc
 *  [](IContainerBase* pParent, const IRECT& r) {
 *    pParent->AddChildControl(new ITextControl(IRECT(), "MyPlugin", {DEFAULT_TEXT_SIZE * 5, COLOR_WHITE}));
 *    WDL_String versionStr {"Version "};
 *    versionStr.Append(PLUG_VERSION_STR);
 *    pParent->AddChildControl(new IVLabelControl(IRECT(), versionStr.Get()));
 *    pParent->AddChildControl(new IVLabelControl(IRECT(), "By Manufacturer"));
 *    pParent->AddChildControl(new IURLControl(IRECT(),
 *                                              "https://www.acmeinc.com",
 *                                              "https://www.acmeinc.com", {DEFAULT_TEXT_SIZE, COLOR_WHITE}));
 *  },
 *  // ResizeFunc
 *  [](IContainerBase* pParent, const IRECT& r) {
 *    const IRECT mainArea = r.GetPadded(-20);
 *    const auto content = mainArea.GetPadded(-10);
 *    const auto logo = content.GetFromTop(300).GetCentredInside(300, 100);
 *    const auto links = content.FracRectVertical(0.75).GetCentredInside(300, 100);
 *    pParent->GetChild(0)->SetTargetAndDrawRECTs(logo);
 *    pParent->GetChild(1)->SetTargetAndDrawRECTs(links.SubRectVertical(4, 0));
 *    pParent->GetChild(2)->SetTargetAndDrawRECTs(links.SubRectVertical(4, 2));
 *    pParent->GetChild(3)->SetTargetAndDrawRECTs(links.SubRectVertical(4, 3));
 *  }, 200), kCtrlTagAboutBox)->Hide(true);
 * \endcode
 * In a button control somewhere on the UI, call 
 * pGraphics->GetControlWithTag(kCtrlTagAboutBox)->As<IAboutBoxControl>()->Show();
 * in order to get the about box to appear.
 * @ingroup IControls */
class IAboutBoxControl : public IPanelControl
{
public:
  /** Constructor
   * @param bounds The control's bounds
   * @param color The background color or pattern
   * @param attachFunc A function used to attach the child controls
   * @param resizeFunc A function to call when the control is resized, used to position the child controls
   * @param animationTime The default duration of the fade-in in milliseconds */
  IAboutBoxControl(const IRECT& bounds, const IPattern& color,
                   AttachFunc attachFunc = nullptr, ResizeFunc resizeFunc = nullptr,
                   int animationTime = 200)
  : IPanelControl(bounds, color, false, attachFunc, resizeFunc)
  , mAnimationTime(animationTime)
  {
    mIgnoreMouse = false;
  }
  
  bool OnKeyDown(float x, float y, const IKeyPress& key) override
  {
    if (key.VK == kVK_ESCAPE)
    {
      Dismiss();
      return true;
    }
    
    return false;
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    Dismiss();
  }

  void Dismiss()
  {
    Hide(true);
  }
  
  void Show()
  {
    IPanelControl::Hide(false);

    SetAnimation([&](IControl* pCaller) {
      auto progress = static_cast<float>(pCaller->GetAnimationProgress());

      auto blend = IBlend(EBlend::Default, progress);

      SetBlend(blend);
      
      ForAllChildrenFunc([blend](int, IControl* pControl) {
        pControl->SetBlend(blend);
      });
      
      if (progress > 1.0f)
      {
        pCaller->OnEndAnimation();
        GetUI()->SetAllControlsDirty();
        return;
      }

    }, mAnimationTime);

    SetDirty(true);
  }
  
private:
  int mAnimationTime = 200;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE


