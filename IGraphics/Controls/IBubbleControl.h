/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup SpecialControls
 * @copydoc IBubbleControl
 */

#include "IControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/**
 * If used in the main IControl stack, you probably want it to be the very last control that is added, so that it gets drawn on top.
 * @ingroup SpecialControls */
class IBubbleControl : public IControl
{
public:
  /** An enumerated list, that is used to determine the state of the menu, mainly for animations*/
  enum EPopupState
  {
    kCollapsed = 0,
    kExpanding,
    kExpanded,
    kCollapsing,
  };

  enum EArrowDir
  {
    kNorth,
    kEast,
    kSouth,
    kWest
  };

  /** Create a new IBubbleControl */
  IBubbleControl(const IText& text)
  : IControl(IRECT())
  {
    mText = text;
    mHide = true;
    mIgnoreMouse = true;
    
    auto animationFunc = [&](IControl* pCaller) {
      auto progress = pCaller->GetAnimationProgress();
            
      if(progress > 1.) {
        
        if(mState == kExpanding)
        {
          mBlend.mWeight = mOpacity;
          mState = kExpanded;
        }
        else if(mState == kExpanded)
        {
          mBlend.mWeight = mOpacity;
          mState = kCollapsing;
          SetDirty(true); // triggers animation again
          return; // don't remove animation
        }
        else if(mState == kCollapsing)
        {
          mBlend.mWeight = 0.;
          Hide(true);
          mState = kCollapsed;
          SetDirty(false);
        }
        
        pCaller->OnEndAnimation();
        return;
      }
      
      switch (mState) {
        case kExpanding: mBlend.mWeight = (float) progress * mOpacity; break;
        case kExpanded: mBlend.mWeight = mOpacity; break;
        case kCollapsing: mBlend.mWeight = (float) (1.-progress) * mOpacity; break;
        default:
          break;
      }
    };
    
    SetActionFunction([animationFunc, this](IControl* pCaller) {
      SetAnimation(animationFunc, 200);
    });
    
    SetAnimationEndActionFunction([animationFunc, this](IControl* pCaller) {
      if(mState == kExpanded) {
        SetAnimation(animationFunc, 1000);
      }
    });
  }
  
  virtual ~IBubbleControl()
  {
  }

  //IControl
  void Draw(IGraphics& g) override
  {
    DrawDropShadow(g);

    IRECT r = mRECT.GetPadded(-mDropShadowSize);
    
    g.FillRoundRect(COLOR_WHITE, r, 5.f, &mBlend);
    g.DrawRoundRect(COLOR_DARK_GRAY, r, 5.f, &mBlend, 2.f);
    g.DrawText(mText, mStr.Get(), r, &mBlend);
  }
  
  void DrawDropShadow(IGraphics& g)
  {
    const float yDrop = 2.0;
    IRECT inner = mRECT.GetPadded(-mDropShadowSize);

  #ifdef IGRAPHICS_NANOVG
    auto NanoVGColor = [](const IColor& color, const IBlend* pBlend = nullptr) {
      NVGcolor c;
      c.r = (float) color.R / 255.0f;
      c.g = (float) color.G / 255.0f;
      c.b = (float) color.B / 255.0f;
      c.a = (BlendWeight(pBlend) * color.A) / 255.0f;
      return c;
    };

    NVGcontext* vg = (NVGcontext*) g.GetDrawContext();
    NVGpaint shadowPaint = nvgBoxGradient(vg, inner.L, inner.T + yDrop, inner.W(), inner.H(), mRoundness * 2.f, 20.f, NanoVGColor(COLOR_BLACK_DROP_SHADOW, &mBlend), NanoVGColor(COLOR_TRANSPARENT));
    nvgBeginPath(vg);
    nvgRect(vg, mRECT.L, mRECT.T, mRECT.W(), mRECT.H());
    nvgFillPaint(vg, shadowPaint);
    nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
    nvgFill(vg);
    nvgBeginPath(vg); // Clear the paths
  #else
    if (!g.CheckLayer(mShadowLayer))
    {
      g.StartLayer(this, mRECT);
      g.FillRoundRect(COLOR_BLACK, inner, mRoundness);
      mShadowLayer = g.EndLayer();
      g.ApplyLayerDropShadow(mShadowLayer, IShadow(COLOR_BLACK_DROP_SHADOW, 20.0, 0.0, yDrop, 1.0, true));
    }
    g.DrawLayer(mShadowLayer, &mBlend);
  #endif
  }

  void ShowBubble(IControl* pCaller, float x, float y, const char* str)
  {
    mStr.Set(str);
    IRECT strBounds;
    GetUI()->MeasureText(mText, "100.00%", strBounds);
    strBounds.Pad(mDropShadowSize + 5.f);
    float halfHeight = strBounds.H() / 2.f;
    mRECT = IRECT(x, y - halfHeight, x + strBounds.W(), y + halfHeight);

    if(mState == kCollapsed)
    {
      Hide(false);
      mState = kExpanding;
      SetDirty(true);
    }
  }
  
protected:
  #ifndef IGRAPHICS_NANOVG
  ILayerPtr mShadowLayer;
  #endif
  WDL_String mStr;
  IBlend mBlend = { EBlend::Default, 0.f }; // blend for sub panels appearing
  float mRoundness = 5.f; // The roundness of the corners of the menu panel backgrounds
  float mDropShadowSize = 10.f; // The size in pixels of the drop shadow
  float mOpacity = 0.95f; // The opacity of the menu panel backgrounds when fully faded in
  EPopupState mState = kCollapsed; // The state of the pop-up, mainly used for animation
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
