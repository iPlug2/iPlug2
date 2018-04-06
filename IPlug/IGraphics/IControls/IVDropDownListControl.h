#pragma once
#include "IControl.h"

/** A vector drop down list.
 Put this control on top of a draw stack
 so that the expanded list is fully visible
 and doesn't close when mouse is over another control */

class IVDropDownListControl : public IControl,
public IVectorBase
{
public:
  IVDropDownListControl(IDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter,
                        const IVColorSpec& colorSpec = DEFAULT_SPEC, EDirection direction = kVertical, int nStates = 0, const char* labels = 0, ...)
  : IControl(dlg, bounds, paramIdx)
  , IVectorBase(colorSpec)
  , mCollapsedBounds(bounds)
  , mDirection(direction)
  {
    AttachIControl(this);

    if (nStates)
    {
      va_list args;
      va_start(args, labels);
      SetButtonLabels(nStates, labels, args);
      va_end(args);
    }
    else if(paramIdx > kNoParameter)
    {
      for (int i = 0; i < GetParam()->NDisplayTexts(); ++i)
        mButtonLabels.Add(new WDL_String(GetParam()->GetDisplayTextAtIdx(i)));
    }
    
    SetActionFunction(DefaultClickActionFunc);
  }

  ~IVDropDownListControl()
  {
    mButtonLabels.Empty(true);
  }
  
  void Animate(double progress) override
  {
    mFlashCircleRadius = progress * mRECT.W() / 2.;
    SetDirty(false);
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(GetColor(kBG), mRECT);

    IRECT collapsedBounds = GetCollapsedBounds();
    
    const float cornerRadius = mRoundness * (collapsedBounds.GetLengthOfShortestSide() / 2.);

    if (!mExpanded)
    {
      if (mDrawShadows && !mEmboss)
        g.FillRoundRect(GetColor(kSH), collapsedBounds.GetShifted(mShadowOffset, mShadowOffset), cornerRadius);

      g.FillRoundRect(GetColor(kFG), collapsedBounds, cornerRadius);

      if(mMouseIsOver)
        g.FillRoundRect(GetColor(kHL), collapsedBounds, cornerRadius);

      if (mDrawFrame)
        g.DrawRoundRect(GetColor(kFR), collapsedBounds, cornerRadius, 0, mFrameThickness);

      Collapse(); // Collapse here to clean the expanded area
    }
    else
    {
      int sx = -1;
      int sy = 0;
      
      GetAdjustedHandleBounds(collapsedBounds);
      
      const float rw = collapsedBounds.W() + mShadowOffset; //TODO: what if no shadow
      const float rh = collapsedBounds.H() + mShadowOffset;
      
      float xshift = mRECT.L - collapsedBounds.L; // TODO: something wrong here, when flush with right of UI
      float yshift = mRECT.T - collapsedBounds.T;
      // now just shift the rects and draw them
      for (int v = 0; v < NButtons(); ++v)
      {
        if (v % mListHeight == 0.0)
        {
          ++sx;
          sy = 0;
        }

        IRECT vR = mCollapsedBounds.GetShifted(sx * rw, sy * rh);
        vR = GetAdjustedHandleBounds(vR);
        vR.Shift(xshift, yshift);

        if (v != mState)
          g.FillRoundRect(GetColor(kFG), vR, cornerRadius);
        else
        {
          if (mDrawShadows)
            g.FillRoundRect(GetColor(kSH), vR.GetShifted(mShadowOffset, mShadowOffset), cornerRadius);

          g.FillRoundRect(GetColor(kPR), vR, cornerRadius);
        }

        if (mDrawFrame)
          g.DrawRoundRect(GetColor(kFR), vR, cornerRadius, 0, mFrameThickness);

        ++sy;
      }
    }
    
    if(GetAnimationFunction())
    {
      float mouseDownX, mouseDownY;
      g.GetMouseDownPoint(mouseDownX, mouseDownY);
      g.FillCircle(GetColor(kHL), mouseDownX, mouseDownY, mFlashCircleRadius);
    }
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (!mExpanded)
      Expand();
    else
    {
      mExpanded = false;
      mValue = NormalizedValueFromState();
      SetDirty();
    }
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    SetDirty(false);
  }

  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
  {
    mValue = mDefaultValue;
    int ns = StateFromNormalizedValue();

    if (mState != ns)
    {
      mState = ns;
      mValue = NormalizedValueFromState();
      SetDirty();
    }

    mExpanded = false;
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    OnMouseOver(x, y, mod);
  }

  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
  {
    int ns = mState;
    ns += (int) d;
    ns = Clip(ns, 0, NButtons() - 1);

    if (ns != mState)
    {
      mState = ns;
      mValue = NormalizedValueFromState();

      SetDirty();
    }
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    if (mLastX != x || mLastY != y)
    {
      mLastX = x;
      mLastY = y;
      IRECT expandedBounds = GetExpandedBounds();
      
      if (mExpanded && expandedBounds.Contains(x, y))
      {
        float rx = x - expandedBounds.L;
        float ry = y - expandedBounds.T;

        IRECT collapsedBounds = GetCollapsedBounds();

        int ix = (int)(rx / collapsedBounds.W());
        int iy = (int)(ry / collapsedBounds.H());

        int i = ix * mListHeight + iy;

        if (i >= NButtons())
          i = NButtons() - 1;

        if (i != mState)
        {
          mState = i;
          SetDirty(false);
        }
      }
    }
  }

  void OnMouseOut() override
  {
    mState = StateFromNormalizedValue();
    mExpanded = false;
    mLastX = mLastY = -1.0;
    SetDirty(false);
  }

  void OnResize() override
  {
    mCollapsedBounds = mRECT;
    mExpanded = false;
    mLastX = mLastY = -1.0;
    SetDirty(false);
  }

  void SetMaxListHeight(int nItems)
  {
    mListHeight = nItems;
  }

  void SetNames(int numStates, const char* names...)
  {
    mButtonLabels.Empty(true);

    va_list args;
    va_start(args, names);
    SetButtonLabels(numStates, names, args);
    va_end(args);

    SetDirty(false);
  }

#pragma mark -

private:
  void SetButtonLabels(int numStates, const char* labels, va_list args)
  {
    if (numStates < 1)
      return;

    mButtonLabels.Add(new WDL_String(labels));

    for (auto i = 1; i < numStates; ++i)
      mButtonLabels.Add(new WDL_String(va_arg(args, const char*)));
  }

  int NButtons()
  {
    return mButtonLabels.GetSize();
  }

  double NormalizedValueFromState()
  {
    if (NButtons() < 2)
      return 0.0;
    else
      return (double) mState / (NButtons() - 1);
  }

  int StateFromNormalizedValue()
  {
    return (int) (mValue * (NButtons() - 1));
  }

  IRECT GetCollapsedBounds()
  {
    return GetAdjustedHandleBounds(mCollapsedBounds);
  }

  IRECT GetExpandedBounds()
  {
    IRECT r = mRECT;

    if (mDrawShadows && !mEmboss)
      r.GetShifted(-mShadowOffset, -mShadowOffset);

    return r;
  }

  void Expand()
  {
    // expand from top left of init Rect
    IRECT collapsedBounds = GetCollapsedBounds();
    const float l = collapsedBounds.L;
    const float t = collapsedBounds.T;
    // if num states > max list height, we need more columns
    float w = (float) NButtons() / mListHeight;
    if (w < 1.0) w = 1.0;
    else w += 0.5;
    w = std::round(w);
    w *= (collapsedBounds.W() + mShadowOffset);

    float h = 0.f;

    if (NButtons() > mListHeight)
      h = (float) mListHeight * (collapsedBounds.H() + mShadowOffset);
    else
      h = NButtons() * (collapsedBounds.H() + mShadowOffset);

    // TODO: mDirection
    IRECT _mRECT = mRECT;
    IRECT _mTargetRECT = mTargetRECT;

    _mRECT = IRECT(l, t, l + w, t + h);

//    if (mDrawShadows && !mEmboss)
//      _mRECT.GetShifted(mShadowOffset, mShadowOffset);

    // we don't want expansion to collapse right around the borders, that'd be very UI unfriendly
//    _mTargetRECT = _mRECT.GetPadded(20.0); // todo perhaps padding should depend on display dpi
    // expansion may get over the bounds. if so, shift it

    IRECT uir = GetUI()->GetBounds();
    const float ex = _mRECT.R - uir.R;

    if (ex > 0.f)
    {
      _mRECT.Shift(-ex);
      _mTargetRECT.Shift(-ex);
    }

    const float ey = _mRECT.B - uir.B;

    if (ey > 0.f)
    {
      _mRECT.Shift(0.f, -ey);
      _mTargetRECT.Shift(0.f, -ey);
    }

    mRECT = _mRECT;
    mTargetRECT = _mTargetRECT;

    mExpanded = true;

    SetDirty(false);
  }

  void Collapse()
  {
    mTargetRECT = mRECT = mCollapsedBounds;
  }

  //  void UpdateRectsOnInitChange()
  //  {
  //   if (!mExpanded)
  //     Collapse();
  //   else
  //     Expand();
  //  }

private:
  IRECT mCollapsedBounds;
  EDirection mDirection;
  WDL_PtrList<WDL_String> mButtonLabels;
  float mGap = 5.f;
  bool mExpanded = false;
  float mLastX = -1.0;
  float mLastY = -1.0;
  int mState = -1;
  float mFlashCircleRadius = 0.f;
  int mListHeight = 3; // how long the list can get before adding a new column/row
};

