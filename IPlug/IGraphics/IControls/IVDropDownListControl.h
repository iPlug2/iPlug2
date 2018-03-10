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
  IVDropDownListControl(IDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter, const IVColorSpec& colorSpec = DEFAULT_SPEC, int nStates = 0, const char* labels = 0, ...)
  : IControl(dlg, bounds, kNoParameter)
  , IVectorBase(colorSpec)
  , mCollapsedBounds(bounds)
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
  }
  
  ~IVDropDownListControl()
  {
    mButtonLabels.Empty(true);
  }
  
  void Draw(IGraphics& g) override
  {
    IRECT ir = GetCollapsedBounds();
    const float cornerRadius = mRoundness * (ir.GetLengthOfShortestSide() / 2.);
    
    IColor shadowColor = IColor(60, 0, 0, 0);
    
    if (!mExpanded)
    {
      if (mDrawShadows && !mEmboss)
        g.FillRoundRect(shadowColor, ir.GetShifted(mShadowOffset, mShadowOffset), cornerRadius);
      
      g.FillRoundRect(GetColor(kFG), ir, cornerRadius);
      
      if (mDrawFrame)
        g.DrawRoundRect(GetColor(kFR), ir, cornerRadius);
      
      Collapse(); // Collapse here to clean the expanded area
    }
    else
    {
      int sx = -1;
      int sy = 0;
      const float rw = ir.W();
      const float rh = ir.H();
      // now just shift the rects and draw them
      for (int v = 0; v < NButtons(); ++v)
      {
        if (v % mListHeight == 0.0)
        {
          ++sx;
          sy = 0;
        }
        
        IRECT vR = ir.GetShifted(sx * rw, sy * rh);
        
        if (v != mState)
          g.FillRoundRect(GetColor(kFG), vR, cornerRadius);
        else
        {
          if (mDrawShadows)
            g.FillRoundRect(shadowColor, vR.GetShifted(mShadowOffset, mShadowOffset), cornerRadius);
          
          g.FillRoundRect(GetColor(kPR), vR, cornerRadius);
        }
        
        if (mDrawFrame)
          g.DrawRoundRect(GetColor(kFR), vR, cornerRadius);
        
        ++sy;
      }
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
    ns = BOUNDED(ns, 0, NButtons() - 1);
    
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
      IRECT er = GetExpandedBounds();
      if (mExpanded && er.Contains(x, y))
      {
        float rx = x - er.L;
        float ry = y - er.T;
        
        IRECT cr = GetCollapsedBounds();
        
        int ix = (int)(rx / cr.W());
        int iy = (int)(ry / cr.H());
        
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
    IRECT ir = mCollapsedBounds;
    
    if (mDrawShadows && !mEmboss)
      ir.GetShifted(-mShadowOffset, -mShadowOffset);
    
    if (mExpanded)
      ir = ir.GetShifted(mRECT.L - ir.L, mRECT.T - ir.T);
    // if mRECT didn't fit and was shifted.
    // will be different for some other expand directions
    
    return ir;
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
    IRECT cr = GetCollapsedBounds();
    const float l = cr.L;
    const float t = cr.T;
    // if num states > max list height, we need more columns
    float w = (float) NButtons() / mListHeight;
    if (w < 1.0) w = 1.0;
    else w += 0.5;
    w = std::round(w);
    w *= cr.W();
    
    float h = 0.f;
    
    if (NButtons() > mListHeight)
      h = (float) mListHeight * cr.H();
    else
      h = NButtons() * cr.H();
    
    // TODO: mDirection
    IRECT _mRECT = mRECT;
    IRECT _mTargetRECT = mTargetRECT;
    
    _mRECT = IRECT(l, t, l + w, t + h);
    
    if (mDrawShadows && !mEmboss)
      _mRECT.GetShifted(mShadowOffset, mShadowOffset);
    
    // we don't want expansion to collapse right around the borders, that'd be very UI unfriendly
    _mTargetRECT = _mRECT.GetPadded(20.0); // todo perhaps padding should depend on display dpi
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
  WDL_PtrList<WDL_String> mButtonLabels;
  
  bool mExpanded = false;
  float mLastX = -1.0;
  float mLastY = -1.0;
  int mState = -1;
  
  int mListHeight = 3; // how long the list can get before adding a new column/row
};

