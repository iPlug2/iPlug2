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

  /** Create a new IBubbleControl
   * @param text The IText style to use for the readout
   * @param fillColor The background color of the bubble
   * @param strokeColor The stroke color of the bubble */
  IBubbleControl(const IText& text = DEFAULT_TEXT.WithAlign(EAlign::Center), const IColor& fillColor = COLOR_WHITE, const IColor& strokeColor = COLOR_BLACK, float roundness = 5.f)
  : IControl(IRECT())
  , mFillColor(fillColor)
  , mStrokeColor(strokeColor)
  , mRoundness(roundness)
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
          if(GetUI()->ControlIsCaptured(mCaller))
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
          mTouchId = 0;
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
    IRECT r = mBubbleBounds.GetPadded(-mDropShadowSize);

    DrawDropShadow(g, r);
    DrawBubble(g, r);
    DrawContent(g, r);
  }
  
  /** If controls get moved, you may need to call this to avoid the wrong regions getting dirtied */
  void ResetBounds()
  {
    SetTargetAndDrawRECTs(IRECT());
    mBubbleBounds = mRECT;
    mCaller = nullptr;
  }
  
  /** Set the bounds that the menu can potentially occupy, if not the full graphics context */
  void SetMaxBounds(const IRECT& bounds) { mMaxBounds = bounds; }
  
  /** Set the background color for the bubble */
  void SetFillColor(const IColor& color) { mFillColor = color; }
  
  /** Set the stroke color for the bubble */
  void SetStrokeColor(const IColor& color) { mStrokeColor = color; }

protected:
  virtual void DrawContent(IGraphics& g, const IRECT& r)
  {
    g.DrawText(mText, mStr.Get(), r, &mBlend);
  }
  
  virtual void DrawBubble(IGraphics& g, const IRECT& r)
  {
    float halfCalloutSize = mCalloutSize/2.f;
    
    g.PathMoveTo(r.L, r.T + mRoundness);
    g.PathArc(r.L + mRoundness, r.T + mRoundness, mRoundness, 270.f, 360.f);

    if(mArrowDir == kWest)
    {
      g.PathArc(r.R - mRoundness, r.T + mRoundness, mRoundness, 0.f, 90.f);
      g.PathArc(r.R - mRoundness, r.B - mRoundness, mRoundness, 90.f, 180.f);
      g.PathArc(r.L + mRoundness, r.B - mRoundness, mRoundness, 180.f, 270.f);
      g.PathLineTo(r.L, r.MH() + halfCalloutSize);
      g.PathLineTo(r.L - mCalloutSize, r.MH());
      g.PathLineTo(r.L, r.MH() - halfCalloutSize);
    }
    else if(mArrowDir == kEast)
    {
      g.PathArc(r.R - mRoundness, r.T + mRoundness, mRoundness, 0.f, 90.f);
      g.PathLineTo(r.R, r.MH() - halfCalloutSize);
      g.PathLineTo(r.R + mCalloutSize, r.MH());
      g.PathLineTo(r.R, r.MH() + halfCalloutSize);
      g.PathArc(r.R - mRoundness, r.B - mRoundness, mRoundness, 90.f, 180.f);
      g.PathArc(r.L + mRoundness, r.B - mRoundness, mRoundness, 180.f, 270.f);
    }
    else if(mArrowDir == kNorth)
    {
      g.PathLineTo(r.MW() - halfCalloutSize, r.T);
      g.PathLineTo(r.MW(), r.T - mCalloutSize);
      g.PathLineTo(r.MW() + halfCalloutSize, r.T);
      g.PathArc(r.R - mRoundness, r.T + mRoundness, mRoundness, 0.f, 90.f);
      g.PathArc(r.R - mRoundness, r.B - mRoundness, mRoundness, 90.f, 180.f);
      g.PathArc(r.L + mRoundness, r.B - mRoundness, mRoundness, 180.f, 270.f);
    }
    else if(mArrowDir == kSouth)
    {
      g.PathArc(r.R - mRoundness, r.T + mRoundness, mRoundness, 0.f, 90.f);
      g.PathArc(r.R - mRoundness, r.B - mRoundness, mRoundness, 90.f, 180.f);
      g.PathLineTo(r.MW() + halfCalloutSize, r.B);
      g.PathLineTo(r.MW(), r.B + mCalloutSize);
      g.PathLineTo(r.MW() - halfCalloutSize, r.B);
      g.PathArc(r.L + mRoundness, r.B - mRoundness, mRoundness, 180.f, 270.f);
    }
    g.PathClose();

    g.PathFill(mFillColor, true, &mBlend);
    g.PathStroke(mStrokeColor, mStrokeThickness, IStrokeOptions(), &mBlend);
  }
  
  void DrawDropShadow(IGraphics& g, const IRECT& r)
  {
    g.DrawFastDropShadow(r, mBubbleBounds, 2.0, mRoundness, 10.f, &mBlend);
  }
  
  virtual void MeasureText(const char* str, IRECT& contentBounds)
  {
    GetUI()->MeasureText(mText, str, contentBounds);
  }

  void ShowBubble(IControl* pCaller, float x, float y, const char* str, EDirection dir, IRECT minimumContentBounds, ITouchID touchID = 0)
  {
    if(mMaxBounds.W() == 0)
      mMaxBounds = GetUI()->GetBounds();
    
    mDirection = dir;
    mTouchId = touchID;
    
    mStr.Set(str);
    IRECT contentBounds;
    MeasureText(str, contentBounds);
    
    if (!minimumContentBounds.Empty())
    {
      if(minimumContentBounds.W() > contentBounds.W())
      {
        contentBounds.R = contentBounds.L + minimumContentBounds.W();
      }
      
      if(minimumContentBounds.H() > contentBounds.H())
      {
        contentBounds.B = contentBounds.T + minimumContentBounds.H();
      }
    }
    
    contentBounds.HPad(mHorizontalPadding);
    contentBounds.Pad(mDropShadowSize);
    const float halfHeight = contentBounds.H() / 2.f;
    const float halfWidth = contentBounds.W() / 2.f;

    mBubbleBounds = IRECT(x, y - halfHeight, x + contentBounds.W(), y + halfHeight);
    IRECT controlBounds = pCaller->GetRECT();

    if(mDirection == EDirection::Horizontal)
    {
      const float maxR = mMaxBounds.R - mHorizontalPadding - mDropShadowSize;
      
      // check if it's gone off the right hand side
      if(mBubbleBounds.R > maxR)
      {
        const float shiftLeft = mBubbleBounds.R-controlBounds.L;
        mBubbleBounds.Translate(-shiftLeft, 0.f);
        mArrowDir = EArrowDir::kEast;
      } 
      else
        mArrowDir = EArrowDir::kWest;
    }
    else
    {
      mBubbleBounds.Translate(-halfWidth, -halfHeight);

      // check if it's gone off the top
      if(mBubbleBounds.T < mMaxBounds.T)
      {
        mArrowDir = EArrowDir::kNorth;
        const float shiftDown = mBubbleBounds.H() + controlBounds.H();
        mBubbleBounds.Translate(0.f, shiftDown);
      }
      else
        mArrowDir = EArrowDir::kSouth;
    }
      
    SetRECT(mRECT.Union(mBubbleBounds));

    if(mState == kCollapsed)
    {
      Hide(false);
      mState = kExpanding;
      SetDirty(true);
    }

    if(pCaller != mCaller)
    {
      mRECT = mBubbleBounds;
      GetUI()->SetAllControlsDirty();
    }
    
    mCaller = pCaller;
  }
  
  bool GetActive() const
  {
    return mState > EPopupState::kCollapsed;
  }
  
  ITouchID GetTouchID() const
  {
    return mTouchId;
  }
  
protected:
  friend class IGraphics;
  
  ITouchID mTouchId = 0;
  EDirection mDirection = EDirection::Horizontal;
  IRECT mMaxBounds; // if view is only showing a part of the graphics context, we need to know because menus can't go there
  IRECT mBubbleBounds;
  IControl* mCaller = nullptr;
  EArrowDir mArrowDir = EArrowDir::kWest;
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
