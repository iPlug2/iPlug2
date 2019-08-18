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
#include "IVDisplayControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/**
 * \addtogroup Controls
 * @{
 */

#pragma mark - Vector Controls
class IVLabelControl : public ITextControl
                     , public IVectorBase
{
public:
  IVLabelControl(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE)
  : ITextControl(bounds, label)
  {
    mText = style.labelText;
    AttachIControl(this, label);
  }
  
  void Draw(IGraphics& g) override
  {
    g.FillRect(GetColor(kBG), mRECT);
    
    if (mStr.GetLength())
    {
      if(mStyle.drawShadows)
        g.DrawText(mText.WithFGColor(GetColor(kSH)), mStr.Get(), mRECT.GetTranslated(mStyle.shadowOffset, mStyle.shadowOffset));
      
      g.DrawText(mText, mStr.Get(), mRECT);
    }
  }
};

/** A vector button/momentary switch control. */
class IVButtonControl : public IButtonControlBase
                      , public IVectorBase
{
public:
  /** Constructs a vector button control, with an action function
   * @param bounds The control's bounds
   * @param actionFunc An action function to execute when a button is clicked \see IActionFunction
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param labelInButton if the label inside or outside the button
   * @param valueInButton if the value inside or outside the button
   * @param shape The shape of the button */
  IVButtonControl(const IRECT& bounds, IActionFunction actionFunc = SplashClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool labelInButton = true, bool valueInButton = true, EVShape shape = EVShape::Rectangle);

  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;
  bool IsHit(float x, float y) const override;
  void OnResize() override;

  void SetShape(EVShape shape) { mShape = shape; SetDirty(false); }

protected:
  EVShape mShape;
};

/** A vector switch control. Click to cycle through states. */
class IVSwitchControl : public ISwitchControlBase
                      , public IVectorBase
{
public:
  IVSwitchControl(const IRECT& bounds, int paramIdx = kNoParameter, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool valueInButton = true);
  
  IVSwitchControl(const IRECT& bounds, IActionFunction actionFunc = SplashClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, int numStates = 2, bool valueInButton = true);
  
  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;
  bool IsHit(float x, float y) const override;
  void SetDirty(bool push, int valIdx = kNoValIdx) override;
  void OnResize() override;
  void OnInit() override;
};

/** A vector toggle control. Click to cycle through two states. */
class IVToggleControl : public IVSwitchControl
{
public:
  IVToggleControl(const IRECT& bounds, int paramIdx = kNoParameter, const char* label = "", const IVStyle& style = DEFAULT_STYLE, const char* offText = "OFF", const char* onText = "ON");
  
  IVToggleControl(const IRECT& bounds, IActionFunction actionFunc = SplashClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, const char* offText = "OFF", const char* onText = "ON", bool initialState = false);
  
  void DrawValue(IGraphics& g, bool mouseOver) override;
  void DrawWidget(IGraphics& g) override;
protected:
  WDL_String mOffText;
  WDL_String mOnText;
};

/** /todo. */
class IVSlideSwitchControl : public IVSwitchControl
{
public:
  IVSlideSwitchControl(const IRECT& bounds, int paramIdx = kNoParameter, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool valueInButton = false, EDirection direction = EDirection::Horizontal);
  
  IVSlideSwitchControl(const IRECT& bounds, IActionFunction actionFunc = EmptyClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool valueInButton = false, EDirection direction = EDirection::Horizontal, int numStates = 2, int initialState = 0);
  
  void Draw(IGraphics& g) override;
  void DrawWidget(IGraphics& g) override;
  virtual void DrawTrack(IGraphics& g, const IRECT& filledArea);

  void OnResize() override;
  void OnEndAnimation() override;
  void SetDirty(bool push, int valIdx = kNoValIdx) override;
protected:
  void UpdateRects();

  IRECT mStartRect, mEndRect;
  IRECT mHandleBounds;
  EDirection mDirection;
  IActionFunction mSecondaryActionFunc = EmptyClickActionFunc;
  EVShape mShape = EVShape::Rectangle;
};

/** A vector "tab" multi switch control. Click tabs to cycle through states. */
class IVTabSwitchControl : public ISwitchControlBase
                         , public IVectorBase
{
public:
  enum class ETabSegment { Start, Mid, End };

  IVTabSwitchControl(const IRECT& bounds, int paramIdx = kNoParameter, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EVShape shape = EVShape::Rectangle, EDirection direction = EDirection::Horizontal);
  
  IVTabSwitchControl(const IRECT& bounds, IActionFunction actionFunc, const std::initializer_list<const char*>& options, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EVShape shape = EVShape::Rectangle, EDirection direction = EDirection::Horizontal);
  
  virtual ~IVTabSwitchControl() { mTabLabels.Empty(true); }
  void Draw(IGraphics& g) override;
  void OnInit() override;

  virtual void DrawWidget(IGraphics& g) override;
  virtual void DrawButton(IGraphics& g, const IRECT& bounds, bool pressed, bool mouseOver, ETabSegment segment);
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override { mMouseOverButton = -1; }
  void OnResize() override;
  virtual bool IsHit(float x, float y) const override;
  void SetShape(EVShape shape) { mShape = shape; SetDirty(false); }
protected:
  int mMouseOverButton = -1;
  WDL_TypedBuf<IRECT> mButtons;
  WDL_PtrList<WDL_String> mTabLabels;
  EDirection mDirection;
  EVShape mShape;
};

class IVRadioButtonControl : public IVTabSwitchControl
{
public:
  /** Constructs a vector radio button control, linked to a parameter
   * @param bounds The control's bounds
   * @param paramIdx The parameter index to link this control to
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param shape The buttons shape \see IVShape
   * @param direction The direction of the buttons
   * @param buttonSize The size of the buttons */
  IVRadioButtonControl(const IRECT& bounds, int paramIdx = kNoParameter, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EVShape shape = EVShape::Ellipse, EDirection direction = EDirection::Vertical, float buttonSize = 20.f);

  /** Constructs a vector radio button control, with an action function (no parameter)
   * @param bounds The control's bounds
   * @param actionFunc An action function to execute when a button is clicked \see IActionFunction
   * @param options An initializer list of CStrings for the button labels. The size of the list decides the number of buttons
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param shape The buttons shape \see IVShape
   * @param direction The direction of the buttons
   * @param buttonSize The size of the buttons */
  IVRadioButtonControl(const IRECT& bounds, IActionFunction actionFunc, const std::initializer_list<const char*>& options, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EVShape shape = EVShape::Ellipse, EDirection direction = EDirection::Vertical, float buttonSize = 20.f);
  
  virtual void DrawWidget(IGraphics& g) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override { mMouseOverButton = -1; }
  void OnResize() override;
  virtual bool IsHit(float x, float y) const override;
protected:
  float mButtonSize;
  bool mOnlyButtonsRespondToMouse = false;
};

/** A vector knob control drawn using graphics primitves */
class IVKnobControl : public IKnobControlBase
                    , public IVectorBase
{
public:
  IVKnobControl(const IRECT& bounds, int paramIdx,
                const char* label = "",
                const IVStyle& style = DEFAULT_STYLE,
                bool valueIsEditable = false, bool valueInWidget = false,
                float a1 = -135.f, float a2 = 135.f, float aAnchor = -135.f,
                EDirection direction = EDirection::Vertical, double gearing = DEFAULT_GEARING);

  IVKnobControl(const IRECT& bounds, IActionFunction actionFunction,
                const char* label = "",
                const IVStyle& style = DEFAULT_STYLE,
                bool valueIsEditable = false, bool valueInWidget = false,
                float a1 = -135.f, float a2 = 135.f, float aAnchor = -135.f,
                EDirection direction = EDirection::Vertical, double gearing = DEFAULT_GEARING);

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
  float mAngle1, mAngle2;
  float mAnchorAngle; // for bipolar arc
  bool mValueMouseOver = false;
};

/** A vector slider control */
class IVSliderControl : public ISliderControlBase
                      , public IVectorBase
{
public:
  IVSliderControl(const IRECT& bounds, int paramIdx = kNoParameter, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool valueIsEditable = false, EDirection dir = EDirection::Vertical, bool onlyHandle = false, float handleSize = 8.f, float trackSize = 2.f);
  
  IVSliderControl(const IRECT& bounds, IActionFunction actionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool valueIsEditable = false, EDirection dir = EDirection::Vertical, bool onlyHandle = false, float handleSize = 8.f, float trackSize = 2.f);

  virtual ~IVSliderControl() {}
  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;
  virtual void DrawTrack(IGraphics& g, const IRECT& filledArea);
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override { mValueMouseOver = false; ISliderControlBase::OnMouseOut(); }
  bool IsHit(float x, float y) const override;
  void OnResize() override;
  void SetDirty(bool push, int valIdx = kNoValIdx) override;
  void OnInit() override;
  void SetShape(EVShape shape) { mShape = shape; SetDirty(false); }

protected:
  float mTrackSize;
  bool mValueMouseOver = false;
  EVShape mShape = EVShape::Ellipse;
};

class IVRangeSliderControl : public IVTrackControlBase
{
public:
  IVRangeSliderControl(const IRECT& bounds, const std::initializer_list<int>& params, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Vertical, bool onlyHandle = false, float handleSize = 8.f, float trackSize = 2.f);

  void Draw(IGraphics& g) override;
  void DrawTrack(IGraphics& g, const IRECT& r, int chIdx) override;
  void DrawWidget(IGraphics& g) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override { mMouseOverHandle = -1; IVTrackControlBase::OnMouseOut(); }
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override { mMouseIsDown = false; }
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;

protected:
  void MakeTrackRects(const IRECT& bounds) override;
  IRECT GetHandleBounds(int trackIdx);
  
  int mMouseOverHandle = -1;
  float mTrackSize;
  float mHandleSize;
  bool mMouseIsDown = false;
  EVShape mShape = EVShape::Ellipse;
};

class IVXYPadControl : public IControl
                     , public IVectorBase
{
public:
  IVXYPadControl(const IRECT& bounds, const std::initializer_list<int>& params, const char* label = "", const IVStyle& style = DEFAULT_STYLE, float handleRadius = 10.f);

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

/** a vector plot to display functions and waveforms **/
class IVPlotControl : public IControl
                    , public IVectorBase
{
public:
    /** IVPlotControl passes values between 0 and 1 to this object, that are the plot normalized x values
     */
  using IPlotFunc = std::function<double(double)>;
    
    /** This struct specifies a plot function
     * @param color The color of the function
     * @param func A callable object that must contain the function to display
     */
  struct Plot {
    IColor color;
    IPlotFunc func;
  };
  
    /** Constructs a vector plot
     * @param bounds The control's bounds
     * @param funcs A function list reference containing the functions to display
     * @param numPoints The number of points used to draw the functions
     * @param label The label for the vector control, leave empty for no label
     * @param style The styling of this vector control \see IVStyle
     * @param shape The buttons shape \see IVShape
     * @param min The minimum y axis plot value
     * @param max The maximum y axis plot value
     * @param useLayer A flag to draw the control layer */
  IVPlotControl(const IRECT& bounds, const std::initializer_list<Plot>& funcs, int numPoints, const char* label = "", const IVStyle& style = DEFAULT_STYLE, float min = -1., float max = 1., bool useLayer = false);
  void Draw(IGraphics& g) override;
  void OnResize() override;
    /** add a new function to the plot
     * @param color The function color
     * @param func A reference object containing the function implementation to display*/
  void AddPlotFunc(const IColor& color, const IPlotFunc& func);
protected:
  ILayerPtr mLayer;
  std::vector<Plot> mPlots;
  float mMin;
  float mMax;
  bool mUseLayer = true;
  std::vector<float> mPoints;
};

#pragma mark - SVG Vector Controls

/** A vector knob/dial control which rotates an SVG image */
class ISVGKnob : public IKnobControlBase
{
public:
  ISVGKnob(const IRECT& bounds, const ISVG& svg, int paramIdx = kNoParameter)
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

#pragma mark - Bitmap Controls

/** A bitmap button/momentary switch control. */
class IBButtonControl : public IButtonControlBase
                      , public IBitmapBase
{
public:
  IBButtonControl(float x, float y, const IBitmap& bitmap, IActionFunction actionFunc = DefaultClickActionFunc)
  : IButtonControlBase(IRECT(x, y, bitmap), actionFunc)
  , IBitmapBase(bitmap)
  {
    AttachIControl(this);
  }

  IBButtonControl(const IRECT& bounds, const IBitmap& bitmap, IActionFunction actionFunc = DefaultClickActionFunc)
  : IButtonControlBase(bounds.GetCentredInside(bitmap), actionFunc)
  , IBitmapBase(bitmap)
  {
    AttachIControl(this);
  }

  void Draw(IGraphics& g) override
  {
    DrawBitmap(g);
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
class IBSwitchControl : public ISwitchControlBase
                      , public IBitmapBase
{
public:
  IBSwitchControl(float x, float y, const IBitmap& bitmap, int paramIdx = kNoParameter)
  : ISwitchControlBase(IRECT(x, y, bitmap), paramIdx)
  , IBitmapBase(bitmap)
  {
    AttachIControl(this);
    mDblAsSingleClick = true;
  }

  IBSwitchControl(const IRECT& bounds, const IBitmap& bitmap, int paramIdx = kNoParameter)
  : ISwitchControlBase(bounds.GetCentredInside(bitmap), paramIdx)
  , IBitmapBase(bitmap)
  {
    AttachIControl(this);
    mDblAsSingleClick = true;
  }
  
  virtual ~IBSwitchControl() {}
  void Draw(IGraphics& g) override { DrawBitmap(g); }
  void GrayOut(bool gray) override { IBitmapBase::GrayOut(gray); IControl::GrayOut(gray); }
  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
};

/** A bitmap knob/dial control that draws a frame from a stacked bitmap */
class IBKnobControl : public IKnobControlBase
                    , public IBitmapBase
{
public:
  IBKnobControl(float x, float y, const IBitmap& bitmap, int paramIdx, EDirection direction = EDirection::Vertical, double gearing = DEFAULT_GEARING)
  : IKnobControlBase(IRECT(x, y, bitmap), paramIdx, direction, gearing)
  , IBitmapBase(bitmap)
  {
    AttachIControl(this);
  }

  IBKnobControl(const IRECT& bounds, const IBitmap& bitmap, int paramIdx, EDirection direction = EDirection::Vertical, double gearing = DEFAULT_GEARING)
  : IKnobControlBase(bounds.GetCentredInside(bitmap), paramIdx, direction, gearing)
  , IBitmapBase(bitmap)
  {
    AttachIControl(this);
  }

  virtual ~IBKnobControl() {}
  void Draw(IGraphics& g) override { DrawBitmap(g); }
  void GrayOut(bool gray) override { IBitmapBase::GrayOut(gray); IControl::GrayOut(gray); }
  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }
};

/** A bitmap knob/dial control that rotates an image */
class IBKnobRotaterControl : public IBKnobControl
{
public:
  IBKnobRotaterControl(float x, float y, const IBitmap& bitmap, int paramIdx)
  : IBKnobControl(IRECT(x, y, bitmap), bitmap, paramIdx) {}

  IBKnobRotaterControl(const IRECT& bounds, const IBitmap& bitmap, int paramIdx)
  : IBKnobControl(bounds.GetCentredInside(bitmap), bitmap, paramIdx) {}

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
  IBSliderControl(const IRECT& bounds, int paramIdx, const IBitmap& bitmap, EDirection dir = EDirection::Vertical, bool onlyHandle = false);

  IBSliderControl(float x, float y, int len, int paramIdx, const IBitmap& bitmap, EDirection direction = EDirection::Vertical, bool onlyHandle = false);

  virtual ~IBSliderControl() {}

  void Draw(IGraphics& g) override;
  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }
  void OnResize() override { SetDirty(false); }
  void GrayOut(bool gray) override  { IBitmapBase::GrayOut(gray); IControl::GrayOut(gray); }
  
  IRECT GetHandleBounds(double value = -1.0) const;
};

/** A control to display text using a monospace bitmap font */
class IBTextControl : public ITextControl
                    , public IBitmapBase
{
public:
  IBTextControl(const IRECT& bounds, const IBitmap& bitmap, const IText& text = DEFAULT_TEXT, const char* str = "", int charWidth = 6, int charHeight = 12, int charOffset = 0, bool multiLine = false, bool vCenter = true, EBlend blend = EBlend::Default)
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

  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }
  void GrayOut(bool gray) override  { IBitmapBase::GrayOut(gray); IControl::GrayOut(gray); }

protected:
  WDL_String mStr;
  int mCharWidth, mCharHeight, mCharOffset;
  bool mMultiLine;
  bool mVCentre;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

/**@}*/

