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
 * @brief A collection of IControls for common UI widgets, such as knobs, sliders, switches
 */

#include "IControl.h"
#include "IColorPickerControl.h"
#include "IVKeyboardControl.h"
#include "IVMeterControl.h"
#include "IVScopeControl.h"
#include "IVMultiSliderControl.h"
#include "IRTTextControl.h"

/**
 * \addtogroup Controls
 * @{
 */

#pragma mark - Vector Controls

/** A vector button/momentary switch control. */
class IVButtonControl : public IButtonControlBase
                      , public IVectorBase
{
public:
  IVButtonControl(IRECT bounds, IActionFunction actionFunc = FlashCircleClickActionFunc,
    const char* str = "", const IText& text = DEFAULT_TEXT, const IVColorSpec& colorSpec = DEFAULT_SPEC);

  void Draw(IGraphics& g) override;

protected:
  WDL_String mStr;
};

/** A vector switch control. Click to cycle through states. */
class IVSwitchControl : public ISwitchControlBase
                      , public IVectorBase
{
public:
  IVSwitchControl(IRECT bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = FlashCircleClickActionFunc,
                  const char* label = "", const IVColorSpec& colorSpec = DEFAULT_SPEC, int numStates = 2);

  void Draw(IGraphics& g) override;

  void SetDirty(bool push) override;

protected:
  WDL_String mStr;
};

/** A vector switch control. Click to cycle through states. */
class IVRadioButtonControl : public ISwitchControlBase
                           , public IVectorBase
{
public:
  IVRadioButtonControl(IRECT bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = FlashCircleClickActionFunc,
                       const IVColorSpec& colorSpec = DEFAULT_SPEC, int numStates = 2, EDirection dir = kVertical);

  virtual ~IVRadioButtonControl() { mLabels.Empty(true); }
  virtual void Draw(IGraphics& g) override;
  virtual void OnResize() override;
//  virtual bool IsHit(float x, float y) const override;

protected:
  EDirection mDirection;
  WDL_TypedBuf<IRECT> mButtons;
  WDL_PtrList<WDL_String> mLabels;
};

/** A vector knob control drawn using graphics primitves */
class IVKnobControl : public IKnobControlBase
                    , public IVectorBase
{
public:
  IVKnobControl(IRECT bounds, int paramIdx,
                const char* label = "", bool displayParamValue = false,
                const IVColorSpec& colorSpec = DEFAULT_SPEC, const IText& labelText = IText(DEFAULT_TEXT_SIZE + 5, IText::kVAlignTop), const IText& valueText = IText(DEFAULT_TEXT_SIZE, IText::kVAlignBottom),
                float aMin = -135.f, float aMax = 135.f, float knobFrac = 0.50f,
                EDirection direction = kVertical, double gearing = DEFAULT_GEARING);

  IVKnobControl(IRECT bounds, IActionFunction actionFunction,
                const char* label = "", bool displayParamValue = false,
                const IVColorSpec& colorSpec = DEFAULT_SPEC, const IText& labelText = IText(DEFAULT_TEXT_SIZE + 5, IText::kVAlignTop), const IText& valueText = IText(DEFAULT_TEXT_SIZE, IText::kVAlignBottom),
                float aMin = -135.f, float aMax = 135.f, float knobFrac = 0.50f,
                EDirection direction = kVertical, double gearing = DEFAULT_GEARING);

  virtual ~IVKnobControl() {}

  void Draw(IGraphics& g) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
//  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {  OnMouseDown(x, y, mod); }
  void OnResize() override;
protected:
  bool mDisplayParamValue;
  bool mShowParamLabel = true;
  IRECT mHandleBounds;
  IRECT mLabelBounds;
  IRECT mValueBounds;
  float mAngleMin, mAngleMax;
  float mKnobFrac;
  WDL_String mLabel;
  IText mLabelText;
  IText& mValueText = mText;
};

/** A vector knob/dial control which rotates an SVG image */
class ISVGKnob : public IKnobControlBase
{
public:
  ISVGKnob(IRECT bounds, const ISVG& svg, int paramIdx = kNoParameter)
    : IKnobControlBase(bounds, paramIdx)
    , mSVG(svg)
  {
  }

  void Draw(IGraphics& g) override
  {
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(mRECT);
      g.DrawSVG(mSVG, mRECT);
      mLayer = g.EndLayer();
    }

    g.DrawRotatedLayer(mLayer, mStartAngle + mValue * (mEndAngle - mStartAngle));
  }

  void SetSVG(ISVG& svg)
  {
    mSVG = svg;
    SetDirty(false);
  }

private:
  ILayerPtr mLayer;
  ISVG mSVG;
  float mStartAngle = -135.f;
  float mEndAngle = 135.f;
};

/** A vector slider control */
class IVSliderControl : public ISliderControlBase
                      , public IVectorBase
{
public:
  IVSliderControl(IRECT bounds, int paramIdx = kNoParameter,
                  const IVColorSpec& colorSpec = DEFAULT_SPEC,
                  EDirection dir = kVertical, bool onlyHandle = false, float handleSize = 8.f, float trackSize = 2.f)
  : ISliderControlBase(bounds, paramIdx, dir, onlyHandle, handleSize)
  , IVectorBase(colorSpec)
  , mTrackSize(trackSize)
  {
    AttachIControl(this);
  }

  IVSliderControl(IRECT bounds, IActionFunction aF,
                  const IVColorSpec& colorSpec = DEFAULT_SPEC,
                  EDirection dir = kVertical, bool onlyHandle = false, float handleSize = 8.f, float trackSize = 2.f)
  : ISliderControlBase(bounds, aF, dir, onlyHandle, handleSize)
  , IVectorBase(colorSpec)
  , mTrackSize(trackSize)
  {
    AttachIControl(this);
  }

  virtual ~IVSliderControl() {}

  virtual void Draw(IGraphics& g) override;
  void OnResize() override;

private:
  float mTrackSize;
};

#pragma mark - Bitmap Controls

/** A bitmap button/momentary switch control. */
class IBButtonControl : public IButtonControlBase
                      , public IBitmapBase
{
public:
  IBButtonControl(float x, float y, const IBitmap& bitmap, IActionFunction actionFunc = DefaultClickActionFunc)
  : IButtonControlBase(IRECT(x, y, bitmap), actionFunc)
  , IBitmapBase(bitmap)
  {}

  IBButtonControl(IRECT bounds, const IBitmap& bitmap, IActionFunction actionFunc = DefaultClickActionFunc)
  : IButtonControlBase(bounds, actionFunc)
  , IBitmapBase(bitmap)
  {}

  void Draw(IGraphics& g) override
  {
    g.DrawBitmap(mBitmap, mRECT, (int) mValue + 1, &mBlend);
  }

  virtual void OnRescale() override
  {
    mBitmap = GetUI()->GetScaledBitmap(mBitmap);
  }

  virtual void GrayOut(bool gray) override
  {
    IBitmapBase::GrayOut(gray);
    IControl::GrayOut(gray);
  }
};

/** A bitmap switch control. Click to cycle through states. */
class IBSwitchControl : public IBitmapControl
{
public:
  IBSwitchControl(float x, float y, const IBitmap& bitmap, int paramIdx = kNoParameter)
  : IBitmapControl(x, y, bitmap, paramIdx) {}

  IBSwitchControl(IRECT bounds, const IBitmap& bitmap, int paramIdx = kNoParameter)
  : IBitmapControl(bounds, bitmap, paramIdx) {}

  virtual ~IBSwitchControl() {}

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {  OnMouseDown(x, y, mod); }

  virtual void GrayOut(bool gray) override
  {
    IBitmapBase::GrayOut(gray);
    IControl::GrayOut(gray);
  }
};

/** A bitmap knob/dial control that draws a frame from a stacked bitmap */
class IBKnobControl : public IKnobControlBase
                    , public IBitmapBase
{
public:
  IBKnobControl(float x, float y, const IBitmap& bitmap, int paramIdx, EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
  : IKnobControlBase(IRECT(x, y, bitmap), paramIdx, direction, gearing)
  , IBitmapBase(bitmap)
  {
  }

  IBKnobControl(IRECT bounds, const IBitmap& bitmap, int paramIdx, EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
  : IKnobControlBase(bounds.GetCentredInside(bitmap), paramIdx, direction, gearing)
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

/** A bitmap knob/dial control that rotates an image */
class IBKnobRotaterControl : public IBKnobControl
{
public:
  IBKnobRotaterControl(float x, float y, const IBitmap& bitmap, int paramIdx)
  : IBKnobControl(IRECT(x, y, bitmap), bitmap, paramIdx)
  {
  }

  IBKnobRotaterControl(IRECT bounds, const IBitmap& bitmap, int paramIdx)
  : IBKnobControl(bounds.GetCentredInside(bitmap), bitmap, paramIdx)
  {
  }

  virtual ~IBKnobRotaterControl() {}

  void Draw(IGraphics& g) override
  {
    double angle = -130.0 + mValue * 260.0;
    g.DrawRotatedBitmap(mBitmap, mRECT.MW(), mRECT.MH(), angle);
  }
};

/** A bitmap slider/fader control */
class IBSliderControl : public ISliderControlBase
                      , public IBitmapBase
{
public:
  IBSliderControl(IRECT bounds, int paramIdx, const IBitmap& bitmap,
                  EDirection dir = kVertical, bool onlyHandle = false);

  IBSliderControl(float x, float y, int len, int paramIdx,
                  const IBitmap& bitmap, EDirection direction = kVertical, bool onlyHandle = false);

  virtual ~IBSliderControl() {}

  virtual void Draw(IGraphics& g) override;
  virtual void OnRescale() override;
  virtual void OnResize() override;

  IRECT GetHandleBounds(double value = -1.0) const;

  virtual void GrayOut(bool gray) override
  {
    IBitmapBase::GrayOut(gray);
    IControl::GrayOut(gray);
  }
};

/** A control to display text using a monospace bitmap font */
class IBTextControl : public ITextControl
                    , public IBitmapBase
{
public:
  IBTextControl(IRECT bounds, const IBitmap& bitmap, const IText& text = DEFAULT_TEXT, const char* str = "", int charWidth = 6, int charHeight = 12, int charOffset = 0, bool multiLine = false, bool vCenter = true, EBlendType blend = kBlendDefault)
  : ITextControl(bounds, str, text)
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

  void OnRescale() override
  {
    mBitmap = GetUI()->GetScaledBitmap(mBitmap);
  }

protected:
  WDL_String mStr;
  int mCharWidth, mCharHeight, mCharOffset;
  bool mMultiLine;
  bool mVCentre;
};

/**@}*/

