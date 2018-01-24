#pragma once

/**
 * @file
 * @brief A collection of IControls for common UI widgets, such as knobs, sliders, switches
 */

#include "IControl.h"

/**
 * \defgroup Controls IGraphics::IControls
 * @{
 */

#pragma mark - Vector Controls

/** A vector switch control. Click to cycle through states. */
class IVSwitchControl : public IControl
{
public:
  IVSwitchControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, int numStates = 0, const IColor& fgColor = COLOR_BLACK, const IColor& bgColor = COLOR_WHITE, EDirection dir = kVertical, IBlend::EType blendMethod = IBlend::kBlendNone)
  : IControl(plug, rect, paramIdx, blendMethod)
  , mFGColor(fgColor)
  , mBGColor(bgColor)
  , mState(0)
  , mDirection(dir)
  {
    if(paramIdx > -1)
    {
      mNumStates = (int) mPlug.GetParam(paramIdx)->GetRange() + 1;
    }
    else
    {
      assert(numStates > 2);
      mNumStates = numStates;
    }
  }
  
  ~IVSwitchControl() {}
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mState = (mState + 1) % mNumStates;
    mValue = mState / double (mNumStates-1);
    SetDirty();
  }
  
  void Draw(IGraphics& graphics)  override
  {
    graphics.FillRect(mBGColor, mRECT);
    
    IRECT handle;
    
    if(mDirection == kHorizontal)
      handle = mRECT.SubRectHorizontal(mNumStates, mState);
    if(mDirection == kVertical)
      handle = mRECT.SubRectVertical(mNumStates, mState);
    
    graphics.FillRect(mFGColor, handle);
  }
  
private:
  int mState;
  int mNumStates;
  IColor mFGColor;
  IColor mBGColor;
  EDirection mDirection;
};


/** Parent for knobs, to handle mouse action and ballistics. */
class IKnobControlBase : public IControl
{
public:
  IKnobControlBase(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
  : IControl(plug, rect, paramIdx), mDirection(direction), mGearing(gearing) {}
  
  virtual ~IKnobControlBase() {}
  
  void SetGearing(double gearing) { mGearing = gearing; }
  virtual void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  virtual void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
  
protected:
  EDirection mDirection;
  double mGearing;
};

/** A knob that is just a line. */
class IVKnobControl : public IKnobControlBase
{
public:
  IVKnobControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, const IColor& color,
                float rMin = 0.f, float rMax = 1.f, float aMin = -135.f, float aMax = 135.f, EDirection direction = kVertical, double gearing = DEFAULT_GEARING);
  ~IVKnobControl() {}
  
  void Draw(IGraphics& graphics) override;
  
protected:
  IColor mColor;
  float mAngleMin, mAngleMax, mInnerRadius, mOuterRadius;
};

/** Display monospace bitmap font text */
// TODO: fix Centre/Right aligned behaviour when string exceeds bounds or should wrap onto new line
class IBitmapTextControl : public IControl
{
public:
  IBitmapTextControl(IPlugBaseGraphics& plug, IRECT rect, IBitmap& bitmap, const char* str = "", IText* text = 0, int charWidth = 6, int charHeight = 12, int charOffset = 0, bool multiLine = false, bool vCenter = true)
  : IControl(plug, rect)
  , mTextBitmap(bitmap)
  , mCharWidth(charWidth)
  , mCharHeight(charHeight)
  , mCharOffset(charOffset)
  , mMultiLine(multiLine)
  , mVCentre(vCenter)
  {
    mStr.Set(str);
  }
  
  ~IBitmapTextControl() {}
  
  void SetTextFromPlug(const char* str)
  {
    if (strcmp(mStr.Get(), str))
    {
      SetDirty(false);
      mStr.Set(str);
    }
  }
  
  void ClearTextFromPlug()
  {
    SetTextFromPlug("");
  }
  
  void Draw(IGraphics& graphics) override
  {
    if (CSTR_NOT_EMPTY(mStr.Get()))
    {
      graphics.DrawBitmapedText(mTextBitmap, mRECT, mText, &mBlend, mStr.Get(), mVCentre, mMultiLine, mCharWidth, mCharHeight, mCharOffset);
    }
  }
  
protected:
  WDL_String mStr;
  int mCharWidth, mCharHeight, mCharOffset;
  IBitmap mTextBitmap;
  bool mMultiLine;
  bool mVCentre;
};

class IXYPad : public IControl
{
public:
  IXYPad(IPlugBaseGraphics& pPlug, IRECT rect, int handleRadius, int paramIdxX, int paramIdxY, const IColor& hcOn = COLOR_WHITE, const IColor& hcOff = COLOR_BLACK)
  : IControl(pPlug, rect)
  , mHandleRadius(handleRadius)
  , mHandleColorOn(hcOn)
  , mHandleColorOff(hcOff)
  , mCurrentHandleColor(hcOff)
  {
    AddAuxParam(paramIdxX);
    AddAuxParam(paramIdxY);
  }
  
  void Draw(IGraphics& graphics) override
  {
    const float xpos = GetAuxParam(0)->mValue * mRECT.W();
    const float ypos = GetAuxParam(1)->mValue * mRECT.H();
    
    graphics.DrawLine(mCurrentHandleColor, xpos+mRECT.L, mRECT.T, xpos+mRECT.L, mRECT.B, 0);
    graphics.DrawLine(mCurrentHandleColor, mRECT.L, ypos+mRECT.T, mRECT.R, ypos+mRECT.T, 0);
    graphics.FillCircle(mCurrentHandleColor, xpos+mRECT.L, ypos+mRECT.T, mHandleRadius, 0);
}

  void OnMouseDown(float x, float y, const IMouseMod& pMod) override
  {
    mCurrentHandleColor = mHandleColorOn;
    return SnapToMouse(x, y);
  }
  
  void OnMouseUp(float x, float y, const IMouseMod& pMod) override
  {
    mCurrentHandleColor = mHandleColorOff;
  }
  
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& pMod) override
  {
    return SnapToMouse(x, y);
  }
  
  void SnapToMouse(float x, float y)
  {
    GetAuxParam(0)->mValue = BOUNDED((double)x / (double)mRECT.W(), 0, 1);
    GetAuxParam(1)->mValue = BOUNDED((double)y / (double)mRECT.H(), 0, 1);
    
    SetDirty();
  }
  
  void SetDirty(bool pushParamToPlug = true) override
  {
    mDirty = true;
    
    if (pushParamToPlug)
    {
      SetAllAuxParamsFromGUI();
    }
  }
private:
  int mHandleRadius;
  IColor mHandleColorOff;
  IColor mHandleColorOn;
  IColor mCurrentHandleColor;
};


/**@}*/

