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
class IVSwitchControl : public ISwitchControlBase
                      , public IVectorBase
{
public:
  IVSwitchControl(IDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = nullptr, const IVColorSpec& colorSpec = DEFAULT_SPEC, int numStates = 2, EDirection dir = kVertical);

  void Draw(IGraphics& g) override;

  void Animate(double progress) override
  {
    mFlashCircleRadius = progress * mRECT.W() / 2.;
    SetDirty(false);
  }

private:
  float mStep;
  float mFlashCircleRadius = 0.f;
  EDirection mDirection;
};

/** A vector knob control drawn using graphics primitves */
class IVKnobControl : public IKnobControlBase
                    , public IVectorBase
{
public:
  IVKnobControl(IDelegate& dlg, IRECT bounds, int paramIdx,
                const IVColorSpec& colorSpec = DEFAULT_SPEC,
                float aMin = -135.f, float aMax = 135.f,
                EDirection direction = kVertical, double gearing = DEFAULT_GEARING);
  
  IVKnobControl(IDelegate& dlg, IRECT bounds, IActionFunction actionFunction,
                const IVColorSpec& colorSpec = DEFAULT_SPEC,
                float aMin = -135.f, float aMax = 135.f,
                EDirection direction = kVertical, double gearing = DEFAULT_GEARING);
  
  virtual ~IVKnobControl() {}

  void Draw(IGraphics& g) override;
  
protected:
  float mAngleMin, mAngleMax;
};

/** A vector knob control which rotates an SVG image */
class IVSVGKnob : public IKnobControlBase
{
public:
  IVSVGKnob(IDelegate& dlg, IRECT bounds, ISVG& svg, int paramIdx = kNoParameter)
    : IKnobControlBase(dlg, bounds, paramIdx)
    , mSVG(svg)
  {
  }

  void Draw(IGraphics& g) override
  {
#ifdef IGRAPHICS_LICE
    g.DrawText(mText, "UNSUPPORTED", mRECT);
#else
    g.DrawRotatedSVG(mSVG, mRECT.MW(), mRECT.MH(), mRECT.W(), mRECT.H(), mStartAngle + mValue * (mEndAngle - mStartAngle));
#endif
  }

  void SetSVG(ISVG& svg)
  {
    mSVG = svg;
    SetDirty(false);
  }

private:
  ISVG mSVG;
  float mStartAngle = -135.f;
  float mEndAngle = 135.f;
};

class IVSliderControl : public ISliderControlBase
                      , public IVectorBase
{
public:
  IVSliderControl(IDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter,
                  const IVColorSpec& colorSpec = DEFAULT_SPEC,
                  EDirection dir = kVertical, bool onlyHandle = false, int handleSize = 10)
  : ISliderControlBase(dlg, bounds, paramIdx, dir, onlyHandle, handleSize)
  , IVectorBase(colorSpec)
  {
    AttachIControl(this);
  }
  
  IVSliderControl(IDelegate& dlg, IRECT bounds, IActionFunction aF,
                  const IVColorSpec& colorSpec = DEFAULT_SPEC,
                  EDirection dir = kVertical, bool onlyHandle = false, int handleSize = 10)
  : ISliderControlBase(dlg, bounds, aF, dir, onlyHandle, handleSize)
  , IVectorBase(colorSpec)
  {
    AttachIControl(this);
  }
  
  virtual ~IVSliderControl() {}
  
  virtual void Draw(IGraphics& g) override;
  void OnResize() override;
};

class IVContactControl : public IVSwitchControl
{
public:
  IVContactControl(IDelegate& dlg, IRECT bounds, int paramIdx)
  : IVSwitchControl(dlg, bounds, paramIdx)
  {};

  virtual ~IVContactControl() {};

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    mValue = 0.0;
    SetDirty();
  }
};

#pragma mark - Bitmap Controls

/** A bitmap switch control. Click to cycle through states. */
class IBSwitchControl : public IBitmapControl
{
public:
  IBSwitchControl(IDelegate& dlg, float x, float y, int paramIdx, IBitmap& bitmap)
  : IBitmapControl(dlg, x, y, paramIdx, bitmap) {}
  virtual ~IBSwitchControl() {}

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {  OnMouseDown(x, y, mod); }
};

/** A bitmap knob/dial control */
class IBKnobControl : public IKnobControlBase
                    , public IBitmapBase
{
public:
  IBKnobControl(IDelegate& plug, float x, float y, IBitmap& bitmap, int paramIdx)
  : IKnobControlBase(plug, IRECT(x, y, bitmap), paramIdx)
  , IBitmapBase(bitmap)
  {
  }
  
  IBKnobControl(IDelegate& plug, IRECT bounds, IBitmap& bitmap, int paramIdx)
  : IKnobControlBase(plug, bounds.GetCentredInside(bitmap), paramIdx)
  , IBitmapBase(bitmap)
  {
  }
  
  virtual ~IBKnobControl() {}
  
  virtual void Draw(IGraphics& g) override
  {
    int i = 1 + int(0.5 + mValue * (double) (mBitmap.N() - 1));
    g.DrawBitmap(mBitmap, mRECT, i, &mBlend);
  }
  
  void OnRescale() override
  {
    mBitmap = GetUI()->GetScaledBitmap(mBitmap);
  }
  
  virtual void GrayOut(bool gray) override
  {
    IBitmapBase::GrayOut(gray);
    IControl::GrayOut(gray);
  }
};

class IBKnobRotaterControl : public IBKnobControl
{
public:
  IBKnobRotaterControl(IDelegate& plug, float x, float y, IBitmap& bitmap, int paramIdx)
  : IBKnobControl(plug, IRECT(x, y, bitmap), bitmap, paramIdx)
  {
  }
  
  IBKnobRotaterControl(IDelegate& plug, IRECT bounds, IBitmap& bitmap, int paramIdx)
  : IBKnobControl(plug, bounds.GetCentredInside(bitmap), bitmap, paramIdx)
  {
  }
  
  virtual ~IBKnobRotaterControl() {}

  void Draw(IGraphics& g) override
  {
    double angle = -130.0 + mValue * 260.0;
    g.DrawRotatedBitmap(mBitmap, mRECT.MW(), mRECT.MH(), angle);
  }
};

class IBSliderControl : public ISliderControlBase
                      , public IBitmapBase
{
public:
  IBSliderControl(IDelegate& dlg, IRECT bounds, int paramIdx, IBitmap& bitmap,
                  EDirection dir = kVertical, bool onlyHandle = false);
  
  IBSliderControl(IDelegate& dlg, float x, float y, int len, int paramIdx,
                  IBitmap& bitmap, EDirection direction = kVertical, bool onlyHandle = false);
  
  virtual ~IBSliderControl() {}

  virtual void Draw(IGraphics& g) override;
  virtual void OnRescale() override;
  virtual void OnResize() override;
  
  IRECT GetHandleRECT(double value = -1.0) const;

  virtual void GrayOut(bool gray) override
  {
    IBitmapBase::GrayOut(gray);
    IControl::GrayOut(gray);
  }
};

/** Display monospace bitmap font text */
// TODO: fix Centre/Right aligned behaviour when string exceeds bounds or should wrap onto new line
class IBTextControl : public ITextControl
                    , public IBitmapBase
{
public:
  IBTextControl(IDelegate& dlg, IRECT bounds, IBitmap& bitmap, const IText& text = DEFAULT_TEXT, const char* str = "", int charWidth = 6, int charHeight = 12, int charOffset = 0, bool multiLine = false, bool vCenter = true, EBlendType blend = kBlendNone)
  : ITextControl(dlg, bounds, text, str)
  , IBitmapBase(bitmap, blend)
  , mCharWidth(charWidth)
  , mCharHeight(charHeight)
  , mCharOffset(charOffset)
  , mMultiLine(multiLine)
  , mVCentre(vCenter)
  {
    mStr.Set(str);
  }
  
  virtual ~IBTextControl() {}

  void Draw(IGraphics& g) override
  {
    g.DrawBitmapedText(mBitmap, mRECT, mText, &mBlend, mStr.Get(), mVCentre, mMultiLine, mCharWidth, mCharHeight, mCharOffset);
  }
  
  virtual void GrayOut(bool gray) override
  {
    IBitmapBase::GrayOut(gray);
    IControl::GrayOut(gray);
  }

protected:
  WDL_String mStr;
  int mCharWidth, mCharHeight, mCharOffset;
  bool mMultiLine;
  bool mVCentre;
};

/**@}*/

