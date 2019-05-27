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
  IVButtonControl(IRECT bounds, IActionFunction actionFunc = SplashClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool labelInButton = true, bool valueInButton = true, IVShape shape = kVShapeRectangle, float angle = 0.f);

  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;
  bool IsHit(float x, float y) const override;
  void OnResize() override;
  
  void SetAngle(float angle) { mAngle = angle; SetDirty(false); }
  void SetShape(IVShape shape) { mShape = shape; SetDirty(false); }
  
protected:
  IVShape mShape = kVShapeRectangle;
  float mAngle = 0.; // only used for triangle
};

/** A vector switch control. Click to cycle through states. */
class IVSwitchControl : public ISwitchControlBase
                      , public IVectorBase
{
public:
  IVSwitchControl(IRECT bounds, int paramIdx = kNoParameter, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool valueInButton = true);
  
  IVSwitchControl(IRECT bounds, IActionFunction actionFunc = SplashClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, int numStates = 2, bool valueInButton = true);
  
  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;
  bool IsHit(float x, float y) const override;
  void SetDirty(bool push, int valIdx = kNoValIdx) override;
  void OnResize() override;
};

class IVToggleControl : public IVSwitchControl
{
public:
  IVToggleControl(IRECT bounds, int paramIdx = kNoParameter, const char* offText = "OFF", const char* onText = "ON", const char* label = "", const IVStyle& style = DEFAULT_STYLE);
  
  IVToggleControl(IRECT bounds, IActionFunction actionFunc = SplashClickActionFunc, const char* offText = "OFF", const char* onText = "ON", const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool initialState = false);
  
  void DrawValue(IGraphics& g, bool mouseOver) override;
protected:
  WDL_String mOffText;
  WDL_String mOnText;
};

/** A vector switch control. Click to cycle through states. */
class IVRadioButtonControl : public ISwitchControlBase
                           , public IVectorBase
{
public:
  IVRadioButtonControl(IRECT bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = SplashClickActionFunc, int numStates = 2, const char* label = "", const IVStyle& style = DEFAULT_STYLE, IVShape shape = kVShapeCircle, float buttonSize = 10.f);

  IVRadioButtonControl(IRECT bounds, IActionFunction actionFunc, const std::initializer_list<const char*>& options, const char* label = "", const IVStyle& style = DEFAULT_STYLE, IVShape shape = kVShapeCircle, float buttonSize = 10.f);
  
  virtual ~IVRadioButtonControl() { mLabels.Empty(true); }
  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;
  virtual void DrawButton(IGraphics& g, const IRECT& bounds, bool pressed, bool mouseOver);
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override { mMouseOverButton = -1; }
  void OnResize() override;
  virtual bool IsHit(float x, float y) const override;

  void SetShape(IVShape shape) { mShape = shape; SetDirty(false); }
  int GetSelectedIdx() const { return int(0.5 + GetValue() * (double) (mNumStates - 1)); }
protected:
  int mMouseOverButton = -1;
  IVShape mShape;
  float mButtonSize;
  WDL_TypedBuf<IRECT> mButtons;
  WDL_PtrList<WDL_String> mLabels;
};

/** A vector knob control drawn using graphics primitves */
class IVKnobControl : public IKnobControlBase
                    , public IVectorBase
{
public:
  IVKnobControl(IRECT bounds, int paramIdx,
                const char* label = "",
                const IVStyle& style = DEFAULT_STYLE,
                bool valueIsEditable = false,
                float aMin = -135.f, float aMax = 135.f,
                EDirection direction = kVertical, double gearing = DEFAULT_GEARING);

  IVKnobControl(IRECT bounds, IActionFunction actionFunction,
                const char* label = "",
                const IVStyle& style = DEFAULT_STYLE,
                bool valueIsEditable = false,
                float aMin = -135.f, float aMax = 135.f,
                EDirection direction = kVertical, double gearing = DEFAULT_GEARING);

  virtual ~IVKnobControl() {}

  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override { mValueMouseOver = false; IKnobControlBase::OnMouseOut(); }

//  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {  OnMouseDown(x, y, mod); }
  void OnResize() override;
  bool IsHit(float x, float y) const override;
  void SetDirty(bool push, int valIdx = kNoValIdx) override;
  void OnInit() override;

protected:
  float mAngleMin, mAngleMax;
  bool mValueMouseOver = false;
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

    g.DrawRotatedLayer(mLayer, mStartAngle + GetValue() * (mEndAngle - mStartAngle));
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
                  const char* label = "",
                  const IVStyle& style = DEFAULT_STYLE,
                  bool valueIsEditable = false,
                  EDirection dir = kVertical, bool onlyHandle = false, float handleSize = 8.f, float trackSize = 2.f);
  
  IVSliderControl(IRECT bounds, IActionFunction aF,
                  const char* label = "",
                  const IVStyle& style = DEFAULT_STYLE,
                  bool valueIsEditable = false,
                  EDirection dir = kVertical, bool onlyHandle = false, float handleSize = 8.f, float trackSize = 2.f);

  virtual ~IVSliderControl() {}
  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override { mValueMouseOver = false; ISliderControlBase::OnMouseOut(); }
  bool IsHit(float x, float y) const override;
  void OnResize() override;
  void SetDirty(bool push, int valIdx = kNoValIdx) override;
  void OnInit() override;
  
protected:
  float mTrackSize;
  bool mValueMouseOver = false;
};

class IVRangeSliderControl : public IVSliderControl
{
public:
  IVRangeSliderControl(IRECT bounds, int paramIdxLo = kNoParameter, int paramIdxHi = kNoParameter,
                       const char* label = "",
                       const IVStyle& style = DEFAULT_STYLE,
//                       bool valueIsEditable = false,
                       EDirection dir = kVertical, bool onlyHandle = false, float handleSize = 8.f, float trackSize = 2.f);

  void DrawWidget(IGraphics& g) override;
  void DrawValue(IGraphics& g, bool mouseover) override {};
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  void SnapToMouse(float x, float y, EDirection direction, IRECT& bounds, int valIdx = -1, float scalar = 1.) override;

protected:
  double mMouseDownVal = 0.f;
};


class IVXYPadControl : public IControl
                     , public IVectorBase
{
public:
  IVXYPadControl(IRECT bounds, const std::initializer_list<int>& params,
                 const char* label = "",
                 const IVStyle& style = DEFAULT_STYLE,
                 float handleRadius = 10.f);

  void Draw(IGraphics& g) override;
  void DrawWidget(IGraphics& g) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  void OnResize() override;
protected:
  float mHandleRadius;
  bool mMouseDown = false;
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
  : IButtonControlBase(bounds.GetCentredInside(bitmap), actionFunc)
  , IBitmapBase(bitmap)
  {}

  void Draw(IGraphics& g) override
  {
    int i = 1;
    if (mBitmap.N() > 1)
    {
      i = 1 + int(0.5 + GetValue() * (double) (mBitmap.N() - 1));
      i = Clip(i, 1, mBitmap.N());
    }
    
    g.DrawBitmap(mBitmap, mRECT, i, &mBlend);
  }

  void OnRescale() override
  {
    mBitmap = GetUI()->GetScaledBitmap(mBitmap);
  }

  void GrayOut(bool gray) override
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
  : IBitmapControl(bounds.GetCentredInside(bitmap), bitmap, paramIdx) {}

  virtual ~IBSwitchControl() {}

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {  OnMouseDown(x, y, mod); }

  void GrayOut(bool gray) override
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

  void Draw(IGraphics& g) override
  {
    int i = 1 + int(0.5 + GetValue() * (double) (mBitmap.N() - 1));
    g.DrawBitmap(mBitmap, mRECT, i, &mBlend);
  }

  void OnRescale() override
  {
    mBitmap = GetUI()->GetScaledBitmap(mBitmap);
  }

  void GrayOut(bool gray) override
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
    double angle = -130.0 + GetValue() * 260.0;
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

  void Draw(IGraphics& g) override;
  void OnRescale() override;
  void OnResize() override;

  IRECT GetHandleBounds(double value = -1.0) const;

  void GrayOut(bool gray) override
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

  void GrayOut(bool gray) override
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

