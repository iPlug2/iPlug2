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
#include "ILEDControl.h"
#include "IPopupMenuControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/**
 * \addtogroup Controls
 * @{
 */

#pragma mark - Vector Controls

/** A vector label control that can display text with a shadow. Uses the IVStyle "value" text for the label. */
class IVLabelControl : public ITextControl
                     , public IVectorBase
{
public:
  IVLabelControl(const IRECT& bounds, const char* label, const IVStyle& style = DEFAULT_STYLE.WithDrawFrame(false).WithColor(kSH, COLOR_BLACK).WithShadowOffset(1).WithValueText(DEFAULT_VALUE_TEXT.WithSize(20.f).WithFGColor(COLOR_WHITE)));
  void Draw(IGraphics& g) override;
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
  IVButtonControl(const IRECT& bounds, IActionFunction aF = SplashClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool labelInButton = true, bool valueInButton = true, EVShape shape = EVShape::Rectangle);

  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;
  bool IsHit(float x, float y) const override;
  void OnResize() override;
};

/** A vector switch control. Click to cycle through states. */
class IVSwitchControl : public ISwitchControlBase
                      , public IVectorBase
{
public:
  IVSwitchControl(const IRECT& bounds, int paramIdx = kNoParameter, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool valueInButton = true);
  
  IVSwitchControl(const IRECT& bounds, IActionFunction aF = SplashClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, int numStates = 2, bool valueInButton = true);

  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;
  void OnMouseOut() override { ISwitchControlBase::OnMouseOut(); SetDirty(false); }
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
  
  IVToggleControl(const IRECT& bounds, IActionFunction aF = SplashClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, const char* offText = "OFF", const char* onText = "ON", bool initialState = false);
  
  void DrawValue(IGraphics& g, bool mouseOver) override;
  void DrawWidget(IGraphics& g) override;
protected:
  WDL_String mOffText;
  WDL_String mOnText;
};

/** A switch with a slide animation when clicked */
class IVSlideSwitchControl : public IVSwitchControl
{
public:
  IVSlideSwitchControl(const IRECT& bounds, int paramIdx = kNoParameter, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool valueInButton = false, EDirection direction = EDirection::Horizontal);
  
  IVSlideSwitchControl(const IRECT& bounds, IActionFunction aF = EmptyClickActionFunc, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool valueInButton = false, EDirection direction = EDirection::Horizontal, int numStates = 2, int initialState = 0);
  
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
};

/** A vector "tab" multi switch control. Click tabs to cycle through states. */
class IVTabSwitchControl : public ISwitchControlBase
                         , public IVectorBase
{
public:
  enum class ETabSegment { Start, Mid, End };

 /** Constructs a vector tab switch control, linked to a parameter
 * @param bounds The control's bounds
 * @param paramIdx The parameter index to link this control to
 * @param options An initializer list of CStrings for the button labels to override the parameter display text labels. Supply an empty {} list if you don't want to do that.
 * @param label The IVControl label CString
 * @param style The styling of this vector control \see IVStyle
 * @param shape The buttons shape \see IVShape
 * @param direction The direction of the buttons */
  IVTabSwitchControl(const IRECT& bounds, int paramIdx = kNoParameter, const std::initializer_list<const char*>& options = {}, const char* label = "", const IVStyle & style = DEFAULT_STYLE, EVShape shape = EVShape::Rectangle, EDirection direction = EDirection::Horizontal);

  /** Constructs a vector tab switch control, with an action function (no parameter)
  * @param bounds The control's bounds
  * @param actionFunc An action function to execute when a button is clicked \see IActionFunction
  * @param options An initializer list of CStrings for the button labels. The size of the list decides the number of buttons.
  * @param label The IVControl label CString
  * @param style The styling of this vector control \see IVStyle
  * @param shape The buttons shape \see IVShape
  * @param direction The direction of the buttons */
  IVTabSwitchControl(const IRECT& bounds, IActionFunction aF, const std::initializer_list<const char*>& options, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EVShape shape = EVShape::Rectangle, EDirection direction = EDirection::Horizontal);
  
  virtual ~IVTabSwitchControl() { mTabLabels.Empty(true); }
  void Draw(IGraphics& g) override;
  void OnInit() override;

  virtual void DrawWidget(IGraphics& g) override;
  virtual void DrawButton(IGraphics& g, const IRECT& bounds, bool pressed, bool mouseOver, ETabSegment segment, bool disabled);
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override { mMouseOverButton = -1; ISwitchControlBase::OnMouseOut(); SetDirty(false); }
  void OnResize() override;
  virtual bool IsHit(float x, float y) const override;
  
  /** returns the label string on the selected tab */
  const char* GetSelectedLabelStr() const;
protected:
  
  /** @return the index of the entry at the given point or -1 if no entry was hit */
  virtual int GetButtonForPoint(float x, float y) const;

  int mMouseOverButton = -1;
  WDL_TypedBuf<IRECT> mButtons;
  WDL_PtrList<WDL_String> mTabLabels;
  EDirection mDirection;
};

/** A vector "radio buttons" switch control */
class IVRadioButtonControl : public IVTabSwitchControl
{
public:
  /** Constructs a vector radio button control, linked to a parameter
   * @param bounds The control's bounds
   * @param paramIdx The parameter index to link this control to
   * @param options An initializer list of CStrings for the button labels to override the parameter display text labels. Supply an empty {} list if you don't want to do that.
   * @param style The styling of this vector control \see IVStyle
   * @param shape The buttons shape \see IVShape
   * @param direction The direction of the buttons
   * @param buttonSize The size of the buttons */
  IVRadioButtonControl(const IRECT& bounds, int paramIdx = kNoParameter, const std::initializer_list<const char*>& options = {}, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EVShape shape = EVShape::Ellipse, EDirection direction = EDirection::Vertical, float buttonSize = 10.f);

  /** Constructs a vector radio button control, with an action function (no parameter)
   * @param bounds The control's bounds
   * @param actionFunc An action function to execute when a button is clicked \see IActionFunction
   * @param options An initializer list of CStrings for the button labels. The size of the list decides the number of buttons
   * @param label The label for the vector control, leave empty for no label
   * @param style The styling of this vector control \see IVStyle
   * @param shape The buttons shape \see IVShape
   * @param direction The direction of the buttons
   * @param buttonSize The size of the buttons */
  IVRadioButtonControl(const IRECT& bounds, IActionFunction aF, const std::initializer_list<const char*>& options, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EVShape shape = EVShape::Ellipse, EDirection direction = EDirection::Vertical, float buttonSize = 10.f);
  
  virtual void DrawWidget(IGraphics& g) override;
protected:
  /** @return the index of the clickable entry at the given point or -1 if no entry was hit */
  int GetButtonForPoint(float x, float y) const override;

  float mButtonSize;
  float mButtonAreaWidth;
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
                EDirection direction = EDirection::Vertical, double gearing = DEFAULT_GEARING, float trackSize = 2.f);

  IVKnobControl(const IRECT& bounds, IActionFunction aF,
                const char* label = "",
                const IVStyle& style = DEFAULT_STYLE,
                bool valueIsEditable = false, bool valueInWidget = false,
                float a1 = -135.f, float a2 = 135.f, float aAnchor = -135.f,
                EDirection direction = EDirection::Vertical, double gearing = DEFAULT_GEARING, float trackSize = 2.f);

  virtual ~IVKnobControl() {}

  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;
  virtual void DrawIndicatorTrack(IGraphics& g, float angle, float cx, float cy, float radius);
  virtual void DrawPointer(IGraphics& g, float angle, float cx, float cy, float radius);

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override { mValueMouseOver = false; IKnobControlBase::OnMouseOut(); }

  void OnResize() override;
  bool IsHit(float x, float y) const override;
  void SetDirty(bool push, int valIdx = kNoValIdx) override;
  void OnInit() override;

  void SetInnerPointerFrac(float frac) { mInnerPointerFrac = frac; }
  void SetOuterPointerFrac(float frac) { mOuterPointerFrac = frac; }
  void SetPointerThickness(float thickness) { mPointerThickness = thickness; }

protected:
  virtual IRECT GetKnobDragBounds() override;

  float mTrackToHandleDistance = 4.f;
  float mInnerPointerFrac = 0.1f;
  float mOuterPointerFrac = 1.f;
  float mPointerThickness = 2.5f;
  float mAngle1, mAngle2;
  float mAnchorAngle; // for bipolar arc
  bool mValueMouseOver = false;
};

/** A vector slider control */
class IVSliderControl : public ISliderControlBase
                      , public IVectorBase
{
public:
  IVSliderControl(const IRECT& bounds, int paramIdx = kNoParameter, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool valueIsEditable = false, EDirection dir = EDirection::Vertical, double gearing = DEFAULT_GEARING, float handleSize = 8.f, float trackSize = 2.f, bool handleInsideTrack = false);
  
  IVSliderControl(const IRECT& bounds, IActionFunction aF, const char* label = "", const IVStyle& style = DEFAULT_STYLE, bool valueIsEditable = false, EDirection dir = EDirection::Vertical, double gearing = DEFAULT_GEARING, float handleSize = 8.f, float trackSize = 2.f, bool handleInsideTrack = false);

  virtual ~IVSliderControl() {}
  void Draw(IGraphics& g) override;
  virtual void DrawWidget(IGraphics& g) override;
  virtual void DrawTrack(IGraphics& g, const IRECT& filledArea);
  virtual void DrawHandle(IGraphics& g, const IRECT& bounds);

  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override { mValueMouseOver = false; ISliderControlBase::OnMouseOut(); }
  bool IsHit(float x, float y) const override;
  void OnResize() override;
  void SetDirty(bool push, int valIdx = kNoValIdx) override;
  void OnInit() override;

protected:
  bool mHandleInsideTrack = false;
  bool mValueMouseOver = false;
};

/** A vector range slider control, with two handles */
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
  float mHandleSize;
  bool mMouseIsDown = false;
};

/** A vector XY Pad slider control */
class IVXYPadControl : public IControl
                     , public IVectorBase
{
public:
  IVXYPadControl(const IRECT& bounds, const std::initializer_list<int>& params, const char* label = "", const IVStyle& style = DEFAULT_STYLE, float handleRadius = 10.f);

  void Draw(IGraphics& g) override;
  void DrawWidget(IGraphics& g) override;
  virtual void DrawHandle(IGraphics& g, const IRECT& trackBounds, const IRECT& handleBounds);
  virtual void DrawTrack(IGraphics& g);
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  void OnResize() override;
protected:
  float mHandleRadius;
  bool mMouseDown = false;
  bool mTrackClipsHandle = true;
};

/** A vector plot to display functions and waveforms */
class IVPlotControl : public IControl
                    , public IVectorBase
{
public:
  /** IVPlotControl passes values between 0 and 1 to this object, that are the plot normalized x values */
  using IPlotFunc = std::function<double(double)>;
    
  /** Groups a plot function and color
   * @param color The color of the function
   * @param func A callable object that must contain the function to display */
  struct Plot {
    IColor color;
    IPlotFunc func;
  };
  
  /** Constructs an IVPlotControl
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
   * @param func A reference object containing the function implementation to display */
  void AddPlotFunc(const IColor& color, const IPlotFunc& func);
  
protected:
  ILayerPtr mLayer;
  std::vector<Plot> mPlots;
  float mMin;
  float mMax;
  bool mUseLayer = true;
  int mHorizontalDivisions = 10;
  int mVerticalDivisions = 10; // allways + 2 when drawing
  
  std::vector<float> mPoints;
};

/** A control to draw a rectangle around a named IControl group */
class IVGroupControl : public IControl
                     , public IVectorBase
{
public:
  IVGroupControl(const IRECT& bounds, const char* label = "", float labelOffset = 10.f, const IVStyle& style = DEFAULT_STYLE);
  
  IVGroupControl(const char* label, const char* groupName, float padL = 0.f, float padT = 0.f, float padR = 0.f, float padB = 0.f, const IVStyle& style = DEFAULT_STYLE);
  
  void Draw(IGraphics& g) override;
  void DrawWidget(IGraphics& g) override;
  void OnResize() override;
  void OnInit() override;
  
  void SetBoundsBasedOnGroup(const char* groupName, float padL, float padT, float padR, float padB);
protected:
  WDL_String mGroupName;
  float mPadL = 0.f;
  float mPadT = 0.f;
  float mPadR = 0.f;
  float mPadB = 0.f;
  float mLabelOffset = 10.f;
  float mLabelPadding = 10.f;
};

/** A panel control which can be styled with emboss etc. */
class IVPanelControl : public IControl
                     , public IVectorBase
{
public:
  IVPanelControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE.WithColor(kFG, COLOR_TRANSLUCENT).WithEmboss(true))
  : IControl(bounds)
  , IVectorBase(style)
  {
    mIgnoreMouse = true;
    AttachIControl(this, label);
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    DrawValue(g, mMouseIsOver);
  }
  
  void DrawWidget(IGraphics& g) override
  {
    DrawPressableRectangle(g, mWidgetBounds, false, false, false);
  }
  
  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
  }
};

/** A control to show a colour swatch of up to 9 colous. */
class IVColorSwatchControl : public IControl
                           , public IVectorBase
{
public:
  enum class ECellLayout { kGrid, kHorizontal, kVertical };
  
  using ColorChosenFunc = std::function<void(int, IColor)>;

  IVColorSwatchControl(const IRECT& bounds, const char* label = "", ColorChosenFunc func = nullptr, const IVStyle& spec = DEFAULT_STYLE, ECellLayout layout = ECellLayout::kGrid,
    const std::initializer_list<EVColor>& colorIDs = { kBG, kFG, kPR, kFR, kHL, kSH, kX1, kX2, kX3 },
    const std::initializer_list<const char*>& labelsForIDs = { kVColorStrs[kBG],kVColorStrs[kFG],kVColorStrs[kPR],kVColorStrs[kFR],kVColorStrs[kHL],kVColorStrs[kSH],kVColorStrs[kX1],kVColorStrs[kX2],kVColorStrs[kX3] });
  
  virtual ~IVColorSwatchControl() { mLabels.Empty(true); }
  
  void Draw(IGraphics& g) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnResize() override;

  void DrawWidget(IGraphics& g) override;

private:
  ColorChosenFunc mColorChosenFunc = nullptr;
  int mCellOver = -1;
  ECellLayout mLayout = ECellLayout::kVertical;
  WDL_TypedBuf<IRECT> mCellRects;
  WDL_PtrList<WDL_String> mLabels;
  std::vector<EVColor> mColorIdForCells;
};

#pragma mark - SVG Vector Controls

/** A vector knob/dial control which rotates an SVG image */
class ISVGKnobControl : public IKnobControlBase
{
public:
  ISVGKnobControl(const IRECT& bounds, const ISVG& svg, int paramIdx = kNoParameter);
  
  void Draw(IGraphics& g) override;
  void SetSVG(ISVG& svg);
  
private:
  ISVG mSVG;
  float mStartAngle = -135.f;
  float mEndAngle = 135.f;
};

/** A vector button/momentary switch control which shows two SVG states */
class ISVGButtonControl : public IButtonControlBase
{
public:
  /** Constructs a vector button control, with an action function
   * @param bounds The control's bounds
   * @param actionFunc An action function to execute when a button is clicked \see IActionFunction */
  ISVGButtonControl(const IRECT& bounds, IActionFunction aF, const ISVG& offImage, const ISVG& onImage);

  void Draw(IGraphics& g) override;
  //void OnResize() override;

protected:
  ISVG mOffSVG;
  ISVG mOnSVG;
};

/** A vector switch control which shows one of multiple SVG states. Click to cycle through states. */
class ISVGSwitchControl : public ISwitchControlBase
{
public:
  /** Constructs a SVG switch control
  * @param bounds The control's bounds
  * @param svgs A list of ISVGs for the control states
  * @param paramIdx The parameter index to link this control to */
  ISVGSwitchControl(const IRECT& bounds, const std::initializer_list<ISVG>& svgs, int paramIdx = kNoParameter, IActionFunction aF = nullptr);

  void Draw(IGraphics& g) override;

protected:
  std::vector<ISVG> mSVGs;
};

/** A Slider control with and SVG for track and handle */
class ISVGSliderControl : public ISliderControlBase
{
public:
  /** Constructs an ISVGSliderControl
  * @param bounds The control's bounds
  * @param handleSvg An ISVG for the handle part that moves
  * @param handleSvg An ISVG for the track background
  * @param paramIdx The parameter index to link this control to 
  * @param dir The direction of the slider movement 
  * @param gearing \todo */
  ISVGSliderControl(const IRECT& bounds, const ISVG& handleSvg, const ISVG& trackSVG, int paramIdx = kNoParameter, EDirection dir = EDirection::Vertical, double gearing = DEFAULT_GEARING);

  void Draw(IGraphics& g) override;
  void OnResize() override;

protected:
  IRECT GetHandleBounds(double value = -1.0) const;

  IRECT mTrackSVGBounds;
  IRECT mHandleBoundsAtMax;
  ISVG mHandleSVG;
  ISVG mTrackSVG;
};

#pragma mark - Bitmap Controls

/** A bitmap button/momentary switch control. */
class IBButtonControl : public IButtonControlBase
                      , public IBitmapBase
{
public:
  IBButtonControl(float x, float y, const IBitmap& bitmap, IActionFunction aF = DefaultClickActionFunc);

  IBButtonControl(const IRECT& bounds, const IBitmap& bitmap, IActionFunction aF = DefaultClickActionFunc);

  void Draw(IGraphics& g) override  { DrawBitmap(g); }
  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }
};

/** A bitmap switch control. Click to cycle through states. */
class IBSwitchControl : public ISwitchControlBase
                      , public IBitmapBase
{
public:
  /** Constructs a bitmap switch control
  * @param x The x position of the top left point in the control's bounds (width will be determined by bitmap's dimensions)
  * @param y The y position of the top left point in the control's bounds (height will be determined by bitmap's dimensions)
  * @param bitmap The bitmap resource for the control
  * @param paramIdx The parameter index to link this control to */
  IBSwitchControl(float x, float y, const IBitmap& bitmap, int paramIdx = kNoParameter);

  /** Constructs a bitmap switch control
  * @param bounds The control's bounds
  * @param bitmap The bitmap resource for the control
  * @param paramIdx The parameter index to link this control to */
  IBSwitchControl(const IRECT& bounds, const IBitmap& bitmap, int paramIdx = kNoParameter);
  
  virtual ~IBSwitchControl() {}
  void Draw(IGraphics& g) override { DrawBitmap(g); }
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
  , IBitmapBase(bitmap)  { AttachIControl(this); }

  IBKnobControl(const IRECT& bounds, const IBitmap& bitmap, int paramIdx, EDirection direction = EDirection::Vertical, double gearing = DEFAULT_GEARING)
  : IKnobControlBase(bounds.GetCentredInside(bitmap), paramIdx, direction, gearing)
  , IBitmapBase(bitmap)  { AttachIControl(this); }

  virtual ~IBKnobControl() {}
  void Draw(IGraphics& g) override { DrawBitmap(g); }
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
  void Draw(IGraphics& g) override;
};

/** A bitmap slider/fader control */
class IBSliderControl : public ISliderControlBase
                      , public IBitmapBase
{
public:
  IBSliderControl(float x, float y, float trackLength, const IBitmap& handleBitmap, const IBitmap& trackBitmap = IBitmap(), int paramIdx = kNoParameter, EDirection dir = EDirection::Vertical, double gearing = DEFAULT_GEARING);

  IBSliderControl(const IRECT& bounds, const IBitmap& handleBitmap, const IBitmap& trackBitmap = IBitmap(), int paramIdx = kNoParameter, EDirection dir = EDirection::Vertical, double gearing = DEFAULT_GEARING);

  virtual ~IBSliderControl() {}

  void Draw(IGraphics& g) override;
  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }
  void OnResize() override;
  
  IRECT GetHandleBounds(double value = -1.0) const;

protected:
  IBitmap mTrackBitmap;
};

/** A control to display text using a monospace bitmap font */
class IBTextControl : public ITextControl
                    , public IBitmapBase
{
public:
  IBTextControl(const IRECT& bounds, const IBitmap& bitmap, const IText& text = DEFAULT_TEXT, const char* str = "", int charWidth = 6, int charHeight = 12, int charOffset = 0, bool multiLine = false, bool vCenter = true, EBlend blend = EBlend::Default);
  virtual ~IBTextControl() {}

  void Draw(IGraphics& g) override;
  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }

protected:
  int mCharWidth, mCharHeight, mCharOffset;
  bool mMultiLine;
  bool mVCentre;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#include "IVPresetManagerControl.h"
#include "IVNumberBoxControl.h"

/**@}*/

