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

/** A special control to draw contextual info as a slider etc is moved
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
  IBubbleControl(const IText& text = DEFAULT_TEXT.WithAlign(EAlign::Center), const IColor& fillColor = COLOR_WHITE, const IColor& strokeColor = COLOR_BLACK)
  : IControl(IRECT())
  , mFillColor(fillColor)
  , mStrokeColor(strokeColor)
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
          if(GetUI()->GetCapturedControl() == mCaller)
          {
            mState = kExpanded;
            SetDirty(true); // triggers animation again
          }
          else
          {
            mBlend.mWeight = mOpacity;
            mState = kCollapsing;
            SetDirty(true); // triggers animation again
          }
            
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
  
  void Draw(IGraphics& g) override
  {
    IRECT r = mBubbleRect.GetPadded(-mDropShadowSize);

    DrawDropShadow(g, r);
    DrawBubble(g, r);
    DrawContent(g, r);
  }
  
  void ResetBounds()
  {
    SetTargetAndDrawRECTs(IRECT());
    mBubbleRect = mRECT;
    mCaller = nullptr;
  }
  
protected:
  virtual void DrawContent(IGraphics& g, const IRECT& r)
  {
    g.DrawText(mText, mStr.Get(), r, &mBlend);
  }
  
  virtual void DrawBubble(IGraphics& g, const IRECT& r)
  {
    g.PathMoveTo(r.L, r.T + mRoundness);
    g.PathArc(r.L + mRoundness, r.T + mRoundness, mRoundness, 270.f, 360.f);
    g.PathArc(r.R - mRoundness, r.T + mRoundness, mRoundness, 0.f, 90.f);
    g.PathArc(r.R - mRoundness, r.B - mRoundness, mRoundness, 90.f, 180.f);
    g.PathArc(r.L + mRoundness, r.B - mRoundness, mRoundness, 180.f, 270.f);
    g.PathLineTo(r.L, r.MH() + mCalloutSize/2.f);
    g.PathLineTo(r.L - mCalloutSize, r.MH());
    g.PathLineTo(r.L, r.MH() - mCalloutSize/2.f);
    g.PathClose();

    g.PathFill(mFillColor, true, &mBlend);
    g.PathStroke(mStrokeColor, mStrokeThickness, IStrokeOptions(), &mBlend);
  }
  
  void DrawDropShadow(IGraphics& g, const IRECT& r)
  {
    const float yDrop = 2.0;

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
    NVGpaint shadowPaint = nvgBoxGradient(vg, r.L, r.T + yDrop, r.W(), r.H(), mRoundness * 2.f, 20.f, NanoVGColor(COLOR_BLACK_DROP_SHADOW, &mBlend), NanoVGColor(COLOR_TRANSPARENT));
    nvgBeginPath(vg);
    nvgRect(vg, mBubbleRect.L, mBubbleRect.T, mBubbleRect.W(), mBubbleRect.H());
    nvgFillPaint(vg, shadowPaint);
    nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
    nvgFill(vg);
    nvgBeginPath(vg); // Clear the paths
  #else
    if (!g.CheckLayer(mShadowLayer))
    {
      g.StartLayer(this, mBubbleRect);
      g.FillRoundRect(COLOR_BLACK, mBubbleRect, mRoundness);
      mShadowLayer = g.EndLayer();
      g.ApplyLayerDropShadow(mShadowLayer, IShadow(COLOR_BLACK_DROP_SHADOW, 20.0, 0.0, yDrop, 1.0, true));
    }
    g.DrawLayer(mShadowLayer, &mBlend);
  #endif
  }

  void ShowBubble(IControl* pCaller, float x, float y, const char* str, IRECT minimumContentBounds)
  {
    mStr.Set(str);
    IRECT contentBounds;
    GetUI()->MeasureText(mText, str, contentBounds);
    
    if (!minimumContentBounds.Empty())
    {
      if(minimumContentBounds.W() > contentBounds.W() || minimumContentBounds.H() > contentBounds.H())
      {
        contentBounds = minimumContentBounds;
      }
    }
    
    contentBounds.HPad(mHorizontalPadding);
    contentBounds.Pad(mDropShadowSize);
    const float halfHeight = contentBounds.H() / 2.f;
    mBubbleRect = IRECT(x, y - halfHeight, x + contentBounds.W(), y + halfHeight);
    
    SetRECT(mRECT.Union(mBubbleRect));
    
    if(mState == kCollapsed)
    {
      Hide(false);
      mState = kExpanding;
      SetDirty(true);
    }

    if(pCaller != mCaller)
    {
      mRECT = mBubbleRect;
      GetUI()->SetAllControlsDirty();
    }
    
    mCaller = pCaller;
  }
  
protected:
  friend class IGraphics;

  IRECT mBubbleRect;
  IControl* mCaller = nullptr;
  #ifndef IGRAPHICS_NANOVG
  ILayerPtr mShadowLayer;
  #endif
  WDL_String mStr;
  IBlend mBlend = { EBlend::Default, 0.f }; // blend for sub panels appearing
  float mRoundness = 5.f; // The roundness of the corners of the menu panel backgrounds
  float mDropShadowSize = 10.f; // The size in pixels of the drop shadow
  float mCalloutSize = 10.f;
  float mOpacity = 0.95f; // The opacity of the menu panel backgrounds when fully faded in
  float mStrokeThickness = 2.f;
  float mHorizontalPadding = 5.f;
  IColor mFillColor = COLOR_WHITE;
  IColor mStrokeColor = COLOR_BLACK;
  EPopupState mState = kCollapsed; // The state of the pop-up, mainly used for animation
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
