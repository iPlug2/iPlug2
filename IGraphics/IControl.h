#pragma once

/**
 * @file
 * @copydoc IControl
 */

#include <cstring>
#include <cstdlib>

#ifdef VST3_API
#undef stricmp
#undef strnicmp
#include "pluginterfaces/vst/ivstcontextmenu.h"
#include "base/source/fobject.h"
#endif

#include "IPlugPlatform.h"

#include "wdlstring.h"
#include "ptrlist.h"

#include "IGraphics.h"
#include "IPlugEditorDelegate.h"

/** The lowest level base class of an IGraphics control. A control is anything on the GUI, it could be a static bitmap, or something that moves or changes.  The control could manipulate bitmaps or do run-time vector drawing, or whatever.
 * Some controls respond to mouse actions, either by moving a bitmap, transforming a bitmap, or cycling through a set of bitmaps.
 * Other controls are readouts only.
 */
class IControl
#ifdef VST3_API
: public Steinberg::Vst::IContextMenuTarget
, public Steinberg::FObject
#endif
{
public:
  /** Constructor
   * @param dlg The class implementing the IEditorDelegate interface that will handle parameter changes.
   * In a plug-in, this would typically be your main plug-in class
   * If you're doing something using IGraphics on its own (e.g. drawing into an extra window), you need to implement the IEditorDelegate interface somewhere
   * to handle dummy "parameter" changes.
   * @param bounds The rectangular area that the control occupies
   * @param paramIdx If this is > -1 (kNoParameter) this control will be associated with a plugin parameter
   * @param actionFunc pass in a lambda function to provide custom functionality when the control "action" happens (usually mouse down). */
  IControl(IEditorDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = nullptr);
  
  IControl(IEditorDelegate& dlg, IRECT bounds, IActionFunction actionFunc);

  virtual ~IControl() {}

  virtual void OnMouseDown(float x, float y, const IMouseMod& mod);
  virtual void OnMouseUp(float x, float y, const IMouseMod& mod) {}
  virtual void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) {}
  virtual void OnMouseDblClick(float x, float y, const IMouseMod& mod);
  virtual void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) {};
  virtual bool OnKeyDown(float x, float y, int key) { return false; }

  // For efficiency, mouseovers/mouseouts are ignored unless you call IGraphics::HandleMouseOver.
  virtual void OnMouseOver(float x, float y, const IMouseMod& mod) { mMouseIsOver = true; SetDirty(false); }
  virtual void OnMouseOut() { mMouseIsOver = false; SetDirty(false);  }

  /** Implement to do something when something was drag n dropped onto this control */
  virtual void OnDrop(const char* str) {};

  /** Implement to do something when graphics is scaled globally (e.g. moves to high DPI screen) */
  virtual void OnRescale() {}

  /** Called when IControl is constructed or resized using SetRect(). NOTE: if you call SetDirty() in this method, you should pass false as the argument to avoid triggering parameter changes */
  virtual void OnResize() {}
  
  virtual void OnMsgFromDelegate(int messageTag, int dataSize, const void* pData) {};
  
  virtual void OnMidi(const IMidiMsg& msg) {};

  /** Called by default when the user right clicks a control. If IGRAPHICS_NO_CONTEXT_MENU is enabled as a preprocessor macro right clicking control will mean IControl::CreateContextMenu() and IControl::OnContextSelection() do not function on right clicking control. VST3 provides contextual menu support which is hard wired to right click controls by default. You can add custom items to the menu by implementing IControl::CreateContextMenu() and handle them in IControl::OnContextSelection(). In non-VST 3 hosts right clicking will still create the menu, but it will not feature entries added by the host. */
  virtual void CreateContextMenu(IPopupMenu& contextMenu) {}
  
  /** @param pSelectedMenu If pSelectedMenu is invalid it means the user didn't select anything*/
  virtual void OnPopupMenuSelection(IPopupMenu* pSelectedMenu);

  virtual void OnTextEntryCompletion(const char* str) {}

  /** Called in response to a menu selection from CreateContextMenu(); /see CreateContextMenu() */
  virtual void OnContextSelection(int itemSelected) {}

  // By default, mouse double click has its own handler.  A control can set mDblAsSingleClick to true to change,
  // which maps double click to single click for this control (and also causes the mouse to be
  // captured by the control on double click).
  bool MouseDblAsSingleClick() { return mDblAsSingleClick; }

  virtual void Draw(IGraphics& g) = 0;

  virtual void DrawPTHighlight(IGraphics& g);
  virtual void SetPTParameterHighlight(bool isHighlighted, int color);

  /** Create an edit box so the user can enter a value for this control, or pop up a pop-up menu, if we are linked to a parameter. */
  void PromptUserInput();
  
  /** Create an edit box so the user can enter a value for this control, or pop up a pop-up menu,
   * if we are linked to a parameter, specifying bounds for the text entry/pop-up menu corner */
  void PromptUserInput(IRECT& bounds);
  
  inline void SetActionFunction(IActionFunction actionFunc) { mActionFunc = actionFunc; }

  /** @param tooltip Text to be displayed */
  inline void SetTooltip(const char* str) { mTooltip.Set(str); }
  
  /** @return Currently set tooltip text */
  inline const char* GetTooltip() const { return mTooltip.Get(); }

  /** @return Parameter index */
  int ParamIdx() const { return mParamIdx; }
  const IParam* GetParam();
  
  /** This method is called from the class implementing the IEditorDelegate interface in order to update a controls mValue and set it to be marked
   dirty for redraw. This either happens on a parameter changing value or if this control is not linked to a parameter (mParamIdx = kNoParameter);
   @param value Normalised incoming value */
  virtual void SetValueFromDelegate(double value);
  
  /** This method is called after a text entry or popup menu prompt triggered by PromptUserInput();
   * @param value the normalised value after user input via text entry or pop-up menu
   * it calls SetDirty(true), which will mean that the new value gets sent back to the delegate */
  virtual void SetValueFromUserInput(double value);
  
  /** @return Value of the control */
  double GetValue() const { return mValue; }

  const IText& GetText() const { return mText; }
  int GetTextEntryLength() const { return mTextEntryLength; }
  void SetTextEntryLength(int len) { mTextEntryLength = len;  }
  void SetText(IText& txt) { mText = txt; }
  const IRECT& GetRECT() const { return mRECT; } // The draw area for this control.
  void SetRECT(const IRECT& bounds) { mRECT = bounds; mMouseIsOver = false; OnResize(); }
  const IRECT& GetTargetRECT() const { return mTargetRECT; } // The mouse target area (default = draw area).
  void SetTargetRECT(const IRECT& bounds) { mTargetRECT = bounds; mMouseIsOver = false; }
  void SetTargetAndDrawRECTs(const IRECT& bounds) { mRECT = mTargetRECT = bounds; mMouseIsOver = false; OnResize(); }
  /** Shows or hides the IControl.
   * @param hide Set to true to hide the control */
  virtual void Hide(bool hide);
  
  /** @return \c true if the control is hidden. */
  bool IsHidden() const { return mHide; }

  /** Sets grayout for the control to be true or false
   * @param gray \c true for grayed out*/
  virtual void GrayOut(bool gray);
  
  /** @return \c true if the control is grayed */
  bool IsGrayed() const { return mGrayed; }

  void SetMOWhenGrayed(bool allow) { mMOWhenGrayed = allow; }
  void SetMEWhenGrayed(bool allow) { mMEWhenGrayed = allow; }
  bool GetMOWhenGrayed() const { return mMOWhenGrayed; }
  bool GetMEWhenGrayed() const { return mMEWhenGrayed; }
  bool GetIgnoreMouse() const { return mIgnoreMouse; }

  // Override if you want the control to be hit only if a visible part of it is hit, or whatever.
  virtual bool IsHit(float x, float y) const { return mTargetRECT.Contains(x, y); }

  void SetValDisplayControl(IControl* pValDisplayControl) { mValDisplayControl = pValDisplayControl; }
  void SetNameDisplayControl(IControl* pNameDisplayControl) { mNameDisplayControl = pNameDisplayControl; }

  virtual void SetDirty(bool triggerAction = true);
  virtual void SetClean();
  
  virtual bool IsDirty()
  {
    if(mAnimationFunc)
      mAnimationFunc(this);
    
    return mDirty;
  } // This is not const, because it may be overridden and used to update something at the fps
  
  void Clamp(double lo, double hi) { mClampLo = lo; mClampHi = hi; }
  void DisablePrompt(bool disable) { mDisablePrompt = disable; }  // Disables the right-click manual value entry.

  // Sometimes a control changes its state as part of its Draw method.
  // Redraw() prevents the control from being cleaned immediately after drawing.
  void Redraw() { mRedraw = true; }

  // This is an idle call from the GUI thread
  // Only active if USE_IDLE_CALLS is defined.
  virtual void OnGUIIdle() {}

  /** A struct that contains a parameter index and normalized value, used when an IControl must reference multiple parameters */
  struct AuxParam
  {
    /** Normalized value */
    double mValue  = 0.;
    /** Parameter index */
    int mParamIdx;

    AuxParam(int idx) : mParamIdx(idx)
    {
      assert(idx > kNoParameter); // no negative params please
    }
  };

  //TODO: this should change
  /** @name Auxiliary parameter
   *  Normally an IControl is linked to a single parameter, with the index mParamIdx. In some cases it makes sense for a single IControl
   *  to be linked to more than one parameter (for example a break point envelope control). In this case you may add Auxilliary parameter*/
  /**@{*/
  /** @return A pointer to the AuxParam instance at auxParamIdx in the mAuxParams list */
  AuxParam* GetAuxParam(int auxParamIdx);
  /** @return Index of the auxillary parameter in the mAuxParams list that holds the paramIdx */
  int GetAuxParamIdx(int paramIdx);
  /** Adds an auxilliary parameter linked to paramIdx */
  void AddAuxParam(int paramIdx);
  /** Used to update the AuxParam object at auxParamIdx in the mAuxParams list to value */
  virtual void SetAuxParamValueFromDelegate(int auxParamIdx, double value);
  /** If the control modifies values linked to the AuxParams, it can call this method to update all the relevant parameters in the delegate */
  void SetAllAuxParamsFromGUI();
  /** Get the number of Aux Params for this control */
  int NAuxParams() const { return mAuxParams.GetSize(); }
  /**@}*/
  
  void SetTag(int tag) { mTag = tag; }
  int GetTag() const { return mTag; }
  
  void SetWantsMidi(bool enable) { mWantsMidi = true; }
  bool WantsMidi() { return mWantsMidi; }

  /** Gets a pointer to the class implementing the IEditorDelegate interface that handles parameter changes from this IGraphics instance.
   * If you need to call other methods on that class, you can use static_cast<PLUG_CLASS_NAME>(GetDelegate();
   * @return The class implementing the IEditorDelegate interface that handles parameter changes from this IGraphics instance.*/
  IEditorDelegate* GetDelegate() { return &mDelegate; }
  
  void SetGraphics(IGraphics* pGraphics) { mGraphics = pGraphics; }
  IGraphics* GetUI() { return mGraphics; }

  bool GetMouseIsOver() { return mMouseIsOver; }
  
  virtual void SnapToMouse(float x, float y, EDirection direction, IRECT& bounds, float scalar = 1.);
  
  virtual void Animate(double progress) {}

  virtual void EndAnimation()
  {
    mAnimationFunc = nullptr;
    SetDirty(false);
  }
  
  void SetAnimation(IAnimationFunction func, int duration)
  {
    mAnimationFunc = func;
    mAnimationStartTime = std::chrono::high_resolution_clock::now();
    mAnimationDuration = Milliseconds(duration);
  }
  
  IAnimationFunction GetAnimationFunction() { return mAnimationFunc; }
  
  double GetAnimationProgress()
  {
    auto elapsed = Milliseconds(Time::now() - mAnimationStartTime);
    return elapsed.count() / mAnimationDuration.count();
  }
  
#ifdef VST3_API
  Steinberg::tresult PLUGIN_API executeMenuItem (Steinberg::int32 tag) override { OnContextSelection(tag); return Steinberg::kResultOk; }
#endif
  
#pragma mark - IControl Member variables
protected:
  IEditorDelegate& mDelegate;
  IGraphics* mGraphics = nullptr;
  int mTag = kNoTag;
  IRECT mRECT;
  IRECT mTargetRECT;

  /** Parameter index or -1 (kNoParameter) */
  int mParamIdx = kNoParameter;

  IText mText;

  WDL_TypedBuf<AuxParam> mAuxParams;
  int mTextEntryLength = DEFAULT_TEXT_ENTRY_LEN;
  double mValue = 0.;
  double mDefaultValue = -1.; // it's important this is -1 to start with
  double mClampLo = 0.;
  double mClampHi = 1.;
  bool mDirty = true;
  bool mHide = false;
  bool mGrayed = false;
  bool mRedraw = false;
  bool mDisablePrompt = true;
  bool mClamped = false;
  bool mDblAsSingleClick = false;
  bool mMOWhenGrayed = false;
  bool mMEWhenGrayed = false;
  bool mIgnoreMouse = false;
  bool mWantsMidi = false;
  /** if mGraphics::mHandleMouseOver = true, this will be true when the mouse is over control. If you need finer grained control of mouseovers, you can override OnMouseOver() and OnMouseOut() */
  bool mMouseIsOver = false;
  IControl* mValDisplayControl = nullptr;
  IControl* mNameDisplayControl = nullptr;
  WDL_String mTooltip;

  IColor mPTHighlightColor = COLOR_RED;
  bool mPTisHighlighted = false;

#ifdef VST3_API
  OBJ_METHODS(IControl, FObject)
  DEFINE_INTERFACES
  DEF_INTERFACE (IContextMenuTarget)
  END_DEFINE_INTERFACES (FObject)
  REFCOUNT_METHODS(FObject)
#endif
  
private:
  IActionFunction mActionFunc = nullptr;
  IAnimationFunction mAnimationFunc = nullptr;
  TimePoint mAnimationStartTime;
  Milliseconds mAnimationDuration;
};

#pragma mark - BASIC CONTROLS AND BASE CLASSES

class IBitmapBase
{
public:
  IBitmapBase(IBitmap& bitmap, EBlendType blend = kBlendNone)
  : mBitmap(bitmap)
  , mBlend(blend)
  {
  }
  
  virtual ~IBitmapBase() {}
  
  void GrayOut(bool gray)
  {
    mBlend.mWeight = (gray ? GRAYED_ALPHA : 1.0f);
  }

protected:
  IBitmap mBitmap;
  IBlend mBlend;
};

/** A An interface for IVControls, in order for them to share a common set of colors. If you need more flexibility for theming, you're on your own! */
class IVectorBase
{
public:
  IVectorBase(const IColor* pBGColor = &DEFAULT_BGCOLOR,
              const IColor* pFGColor = &DEFAULT_FGCOLOR,
              const IColor* pPRColor = &DEFAULT_PRCOLOR,
              const IColor* pFRColor = &DEFAULT_FRCOLOR,
              const IColor* pHLColor = &DEFAULT_HLCOLOR,
              const IColor* pSHColor = &DEFAULT_SHCOLOR,
              const IColor* pX1Color = &DEFAULT_X1COLOR,
              const IColor* pX2Color = &DEFAULT_X2COLOR,
              const IColor* pX3Color = &DEFAULT_X3COLOR)
  {
    AddColors(pBGColor, pFGColor, pPRColor, pFRColor, pHLColor, pSHColor, pX1Color, pX2Color, pX3Color);
  }

  IVectorBase(const IVColorSpec& spec)
  {
    AddColors(&spec.mBGColor,
              &spec.mFGColor,
              &spec.mPRColor,
              &spec.mFRColor,
              &spec.mHLColor,
              &spec.mSHColor,
              &spec.mX1Color,
              &spec.mX2Color,
              &spec.mX3Color);
  }
  
  void AttachIControl(IControl* pControl) { mControl = pControl; }
  
  void AddColor(const IColor& color)
  {
    mColors.Add(color);
  }
  
  void AddColors(const IColor* pBGColor = 0,
                 const IColor* pFGColor = 0,
                 const IColor* pPRColor = 0,
                 const IColor* pFRColor = 0,
                 const IColor* pHLColor = 0,
                 const IColor* pSHColor = 0,
                 const IColor* pX1Color = 0,
                 const IColor* pX2Color = 0,
                 const IColor* pX3Color = 0)
  {
    if(pBGColor) AddColor(*pBGColor);
    if(pFGColor) AddColor(*pFGColor);
    if(pPRColor) AddColor(*pPRColor);
    if(pFRColor) AddColor(*pFRColor);
    if(pHLColor) AddColor(*pHLColor);
    if(pSHColor) AddColor(*pSHColor);
    if(pX1Color) AddColor(*pX1Color);
    if(pX2Color) AddColor(*pX2Color);
    if(pX3Color) AddColor(*pX3Color);
  }

  void SetColor(int colorIdx, const IColor& color)
  {
    if(colorIdx < mColors.GetSize())
      mColors.Get()[colorIdx] = color;
    
    mControl->SetDirty(false);
  }
  
  void SetColors(const IColor& BGColor,
                 const IColor& FGColor,
                 const IColor& PRColor,
                 const IColor& FRColor,
                 const IColor& HLColor,
                 const IColor& SHColor,
                 const IColor& X1Color,
                 const IColor& X2Color,
                 const IColor& X3Color)
  {
    mColors.Get()[kBG] = BGColor;
    mColors.Get()[kFG] = FGColor;
    mColors.Get()[kPR] = PRColor;
    mColors.Get()[kFR] = FRColor;
    mColors.Get()[kHL] = HLColor;
    mColors.Get()[kSH] = SHColor;
    mColors.Get()[kX1] = X1Color;
    mColors.Get()[kX2] = X2Color;
    mColors.Get()[kX3] = X3Color;
    
    mControl->SetDirty(false);
  }

  void SetColors(const IVColorSpec& spec)
  {
    SetColors(spec.mBGColor,
              spec.mFGColor,
              spec.mPRColor,
              spec.mFRColor,
              spec.mHLColor,
              spec.mSHColor,
              spec.mX1Color,
              spec.mX2Color,
              spec.mX3Color);
  }

  IColor& GetColor(int colorIdx)
  {
    if(colorIdx < mColors.GetSize())
      return mColors.Get()[colorIdx];
    else
      return mColors.Get()[0];
  }
  
  void SetRoundness(float roundness) { mRoundness = Clip(roundness, 0.f, 1.f); mControl->SetDirty(false); }
  void SetDrawFrame(bool draw) { mDrawFrame = draw; mControl->SetDirty(false); }
  void SetDrawShadows(bool draw) { mDrawShadows = draw; mControl->SetDirty(false); }
  void SetEmboss(bool emboss) { mEmboss = emboss; mControl->SetDirty(false); }
  void SetShadowOffset(float offset) { mShadowOffset = offset; mControl->SetDirty(false); }
  void SetFrameThickness(float thickness) { mFrameThickness = thickness; mControl->SetDirty(false); }

  void Style(bool drawFrame, bool drawShadows, bool emboss, float roundness, float frameThickness, float shadowOffset, const IVColorSpec& spec)
  {
    mDrawFrame = drawFrame;
    mDrawShadows = drawShadows;
    mEmboss = emboss;
    mRoundness = roundness;
    mFrameThickness = frameThickness;
    mShadowOffset = shadowOffset;
    SetColors(spec);
  }
  
  IRECT GetAdjustedHandleBounds(IRECT handleBounds)
  {
    if(mDrawFrame)
      handleBounds.Pad(- 0.5 * mFrameThickness);
    
    if (mDrawShadows && !mEmboss)
      handleBounds.Shift(0, 0, -mShadowOffset, -mShadowOffset);
    
    return handleBounds;
  }
  
  void DefaultClickAnimation(IGraphics& g)
  {
    float mouseDownX, mouseDownY;
    g.GetMouseDownPoint(mouseDownX, mouseDownY);
    g.FillCircle(GetColor(kHL), mouseDownX, mouseDownY, mFlashCircleRadius);
  }
  
protected:
  IControl* mControl = nullptr;
  WDL_TypedBuf<IColor> mColors;
  float mRoundness = 0.f;
  float mShadowOffset = 3.f;
  float mFrameThickness = 2.f;
  bool mDrawFrame = true;
  bool mDrawShadows = true;
  bool mEmboss = false;
  float mFlashCircleRadius = 0.f;
  float mMaxFlashCircleRadius = 50.f;
};

/** A basic control to fill a rectangle with a color */
class IPanelControl : public IControl
{
public:
  IPanelControl(IEditorDelegate& dlg, IRECT bounds, const IColor& color, int paramIdx = kNoParameter)
  : IControl(dlg, bounds, paramIdx)
  , mColor(color)
  {
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(mColor, mRECT);
  }
  
private:
  IColor mColor;
};

/** A basic control to draw a bitmap, or one frame of a stacked bitmap depending on the current value. */
class IBitmapControl : public IControl
                     , public IBitmapBase
{
public:
  /** Creates a bitmap control
   * @param paramIdx Parameter index (-1 or kNoParameter, if this should not be linked to a parameter)
   * @param bitmap Image to be drawn */
  IBitmapControl(IEditorDelegate& dlg, float x, float y, IBitmap& bitmap, int paramIdx = kNoParameter, EBlendType blend = kBlendNone)
  : IControl(dlg, IRECT(x, y, bitmap), paramIdx)
  , IBitmapBase(bitmap, blend)
  {}
  
  virtual ~IBitmapControl() {}

  virtual void Draw(IGraphics& g) override;

  /** Implement to do something when graphics is scaled globally (e.g. moves to high DPI screen),
   *  if you override this make sure you call the parent method in order to rescale mBitmap */
  virtual void OnRescale() override;
  
  virtual void GrayOut(bool gray) override
  {
    IBitmapBase::GrayOut(gray);
    IControl::GrayOut(gray);
  }
};
#ifndef OS_WEB
/** A basic control to draw an SVG image to the screen. */
class ISVGControl : public IControl
{
public:
  ISVGControl(IEditorDelegate& dlg, IRECT bounds, int paramIdx, ISVG& svg)
    : IControl(dlg, bounds, paramIdx)
    , mSVG(svg)
  {}

  virtual ~ISVGControl() {}

  virtual void Draw(IGraphics& g) override
  {
    g.DrawSVG(mSVG, mRECT);
  }
  
private:
  //TODO: cache the SVG to intermediate bitmap?
  ISVG mSVG;
};
#endif

/** A basic control to output text to the screen. */
class ITextControl : public IControl
{
public:
  ITextControl(IEditorDelegate& dlg, IRECT bounds, const char* str = "", int paramIdx = kNoParameter, const IText& text = DEFAULT_TEXT, const IColor& BGColor = DEFAULT_BGCOLOR)
  : IControl(dlg, bounds, paramIdx)
  , mStr(str)
  , mBGColor(BGColor)
  {
    IControl::mText = text;
  }

  ~ITextControl() {}

  virtual void SetTextFromDelegate(const char* str);
  virtual void ClearTextFromDelegate() { SetTextFromDelegate(""); }

  void Draw(IGraphics& g) override;

protected:
  WDL_String mStr;
  IColor mBGColor;
};

class ICaptionControl : public ITextControl
{
public:
  ICaptionControl(IEditorDelegate& dlg, IRECT bounds, int paramIdx, const IText& text = DEFAULT_TEXT, bool showParamLabel = true);
  ~ICaptionControl() {}
  
  virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void Draw(IGraphics& g) override;

protected:
  bool mShowParamLabel;
};

#pragma mark - Base Controls

/** Parent for knobs, to handle mouse action and ballistics. */
class IKnobControlBase : public IControl
{
public:
  IKnobControlBase(IEditorDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter,
    EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
    : IControl(dlg, bounds, paramIdx)
    , mDirection(direction)
    , mGearing(gearing)
  {}

  virtual ~IKnobControlBase() {}

  void SetGearing(double gearing) { mGearing = gearing; }
  virtual void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  virtual void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;

protected:
  EDirection mDirection;
  double mGearing;
};

class ISliderControlBase : public IControl
{
public:
  ISliderControlBase(IEditorDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter,
                     EDirection dir = kVertical, bool onlyHandle = false, int handleSize = 0)
  : IControl(dlg, bounds, paramIdx)
  , mDirection(dir)
  , mOnlyHandle(onlyHandle)
  {
    handleSize == 0 ? mHandleSize = bounds.W() : mHandleSize = handleSize;
  }
  
  ISliderControlBase(IEditorDelegate& dlg, IRECT bounds, IActionFunction aF = nullptr,
                     EDirection dir = kVertical, bool onlyHandle = false, int handleSize = 0)
  : IControl(dlg, bounds, aF)
  , mDirection(dir)
  , mOnlyHandle(onlyHandle)
  {
    handleSize == 0 ? mHandleSize = bounds.W() : mHandleSize = handleSize;
  }
  
  virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override { SnapToMouse(x, y, mDirection, mTrack); }
  virtual void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override { SnapToMouse(x, y, mDirection, mTrack); }
  
protected:
  EDirection mDirection;
  IRECT mTrack;
  bool mOnlyHandle;
  int mHandleSize;
};

class IVTrackControlBase : public IControl
                         , public IVectorBase
{
public:
  IVTrackControlBase(IEditorDelegate& dlg, IRECT bounds, int maxNTracks = 1, float minTrackValue = 0.f, float maxTrackValue = 1.f, const char* trackNames = 0, ...)
  : IControl(dlg, bounds)
  , mMaxNTracks(maxNTracks)
  , mMinTrackValue(minTrackValue)
  , mMaxTrackValue(maxTrackValue)
  {
    for (auto i=0; i<maxNTracks; i++) {
      mTrackData.Add(0.f);
      mTrackBounds.Add(IRECT());
    }
    
    AttachIControl(this);
  }
  
  void MakeRects()
  {
    for (auto ch = 0; ch < MaxNTracks(); ch++)
    {
      mTrackBounds.Get()[ch] = mRECT.GetPadded(-mOuterPadding).SubRect(EDirection(!mDirection), MaxNTracks(), ch);
    }
  }
  
  void Draw(IGraphics& g) override
  {
    g.FillRect(GetColor(kBG), mRECT);
    
    const float cornerRadius = mRoundness * (mRECT.W() / 2.);
    
    for (auto ch = 0; ch < MaxNTracks(); ch++)
    {
      DrawTrack(g, mTrackBounds.Get()[ch], ch);
    }
    
    if(mDrawFrame)
      g.DrawRoundRect(GetColor(kFR), mRECT, cornerRadius, nullptr, mFrameThickness);
  }
  
  int NTracks() { return mNTracks; }
  int MaxNTracks() { return mMaxNTracks; }
  void SetTrackData(int trackIdx, float val) { mTrackData.Get()[trackIdx] = Clip(val, mMinTrackValue, mMaxTrackValue); }
  float* GetTrackData(int trackIdx) { return &mTrackData.Get()[trackIdx];  }
  void SetAllTrackData(float val) { memset(mTrackData.Get(), Clip(val, mMinTrackValue, mMaxTrackValue), mTrackData.GetSize() * sizeof(float) ); }
private:
  virtual void DrawTrack(IGraphics& g, IRECT& r, int chIdx)
  {
    DrawTrackBG(g, r, chIdx);
    DrawTrackHandle(g, r, chIdx);
    
    if(mDrawTrackFrame)
      g.DrawRect(GetColor(kFR), r, nullptr, mFrameThickness);
  }
  
  virtual void DrawTrackBG(IGraphics& g, IRECT& r, int chIdx)
  {
    g.FillRect(GetColor(kSH), r);
  }
  
  virtual void DrawTrackHandle(IGraphics& g, IRECT& r, int chIdx)
  {
    IRECT fillRect = r.FracRect(mDirection, *GetTrackData(chIdx));
    
    g.FillRect(GetColor(kFG), fillRect); // TODO: shadows!
    
    IRECT peakRect;
    
    if(mDirection == kVertical)
      peakRect = IRECT(fillRect.L, fillRect.T, fillRect.R, fillRect.T + mPeakSize);
    else
      peakRect = IRECT(fillRect.R - mPeakSize, fillRect.T, fillRect.R, fillRect.B);
    
    DrawPeak(g, peakRect, chIdx);
  }
  
  virtual void DrawPeak(IGraphics& g, IRECT& r, int chIdx)
  {
    g.FillRect(GetColor(kHL), r);
  }
  
  void OnResize() override
  {
    MakeRects();
  }
  
protected:
  
  EDirection mDirection = EDirection::kVertical;
  int mMaxNTracks;
  WDL_TypedBuf<float> mTrackData; // real values of sliders/meters
  WDL_TypedBuf<IRECT> mTrackBounds;

  int mNTracks = 1;
  
  float mMinTrackValue;
  float mMaxTrackValue;
  float mOuterPadding = 10.;
  float mTrackPadding = 2;
  float mPeakSize = 5.;
  bool mDrawTrackFrame = true;
};

/** Parent for switch controls (including buttons a.k.a. momentary switches)*/
class ISwitchControlBase : public IControl
{
public:
  ISwitchControlBase(IEditorDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter, IActionFunction aF = nullptr,
    int numStates = 2);

  virtual ~ISwitchControlBase() {}

  virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override;
protected:
  int mNumStates;
};

/** An abstract IControl base class that you can inherit from in order to make a control that pops up a menu to browse files */
class IDirBrowseControlBase : public IControl
{
public:
  IDirBrowseControlBase(IEditorDelegate& dlg, IRECT bounds, const char* extension /* e.g. ".txt"*/)
  : IControl(dlg, bounds)
  {
    mExtension.Set(extension);
  }

  ~IDirBrowseControlBase();

  int NItems();

  void AddPath(const char* path, const char* label);

  void SetUpMenu();

  void GetSelecteItemPath(WDL_String& path);

private:
  void ScanDirectory(const char* path, IPopupMenu& menuToAddTo);

protected:
  int mSelectedIndex = -1;
  IPopupMenu* mSelectedMenu = nullptr;
  IPopupMenu mMainMenu;
  WDL_PtrList<WDL_String> mPaths;
  WDL_PtrList<WDL_String> mPathLabels;
  WDL_PtrList<WDL_String> mFiles;
  WDL_String mExtension;
};

/** A base control for a pop-up menu/drop-down list that stays within the bounds of the IGraphics context
 * On the web and on iOS, we don't have traditional contextual menus. This class provides a way of making a menu
 * that works across all platforms
 */
class IPopupMenuControlBase : public IControl
{
public:
  IPopupMenuControlBase(IEditorDelegate& dlg, EDirection direction = kVertical)
  : IControl(dlg, IRECT(), kNoParameter)
  , mDirection(direction)
  {
    SetActionFunction(DefaultClickActionFunc);
    
    mText = IText(COLOR_BLACK, 12, nullptr, IText::kStyleNormal, IText::kAlignNear);
    mDirty = false;
  }
  
  virtual ~IPopupMenuControlBase() {}
  
  //IControl
  virtual bool IsDirty() override
  {
    return mExpanded | IControl::IsDirty();
  }
  
  void Draw(IGraphics& g) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  void OnMouseOver(float x, float y, const IMouseMod& mod) override;
  void OnMouseOut() override;
  
  void Animate(double progress) override
  {
    mBlend.mWeight = progress * mOpacity;
    SetDirty(false);
  }

  void EndAnimation() override
  {
    mBlend.mWeight = mOpacity;
    SetAnimation(nullptr, 0);
    SetDirty(false);
  }

  //IPopupMenuControlBase
  virtual void DrawBackground(IGraphics& g, const IRECT& bounds);
  virtual void DrawShadow(IGraphics& g, const IRECT& bounds);
  virtual void DrawCell(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem);
  virtual void DrawHighlightCell(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem);
  virtual void DrawCellText(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem);
  virtual void DrawSeparator(IGraphics& g, const IRECT& bounds);
  
  /** Call this to create a pop-up menu. This method
   @param menu Reference to a menu from which to populate this user interface control. NOTE: this object should not be a temporary, otherwise when the menu returns asynchronously, it may not exist.
   @param pCaller The IControl that called this method, and will receive the call back after menu selection
   @return the menu */
  IPopupMenu* CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller);
  
  bool GetExpanded() const { return mExpanded; }
  
private:
  
  /** This method is called to expand the modal pop-up menu. It calculates the dimensions and wrapping, to keep the cells within the graphics context. It handles the dirtying of the graphics context, and modification of graphics behaviours such as tooltips and mouse cursor */
  void Expand();
  
    /** This method is called to collapse the modal pop-up menu and make it invisible. It handles the dirtying of the graphics context, and modification of graphics behaviours such as tooltips and mouse cursor */
  virtual void Collapse();
  
  float CellWidth() const { return mCollapsedBounds.W(); }
  float CellHeight() const { return mCollapsedBounds.H(); }
  
  /** Checks if any of the expanded cells contain a x, y coordinate, and if so returns an IRECT pointer to the cell bounds
   * @param x X position to test
   * @param y Y position to test
   * @return Pointer to the cell bounds IRECT, or nullptr if nothing got hit */
  IRECT* HitTestCells(float x, float y) const
  {
    for(auto i = 0; i < mCellBounds.GetSize(); i++)
    {
      IRECT* r = mCellBounds.Get(i);
      if(r->Contains(x, y))
      {
        return r;
      }
    }
    return nullptr;
  }
  
private:
  bool mExpanded = false;
  EDirection mDirection;
  IRECT mCollapsedBounds; // The rectangle that the control occupies when it's collapsed. The dimensions are the dimensions of 1 cell.
  WDL_PtrList<IRECT> mCellBounds; // The size of this array will always correspond to the number of items in the top level of the menu
  IRECT* mMouseCellBounds = nullptr;
  IControl* mCaller = nullptr;
  IPopupMenu* mMenu = nullptr; // This control does not own the menu
  float mCellGap = 2.f; // the gap between cells
  int mMaxColumnItems = 0; // how long the list can get before adding a new column - 0 equals no limit
  int mMaxRowItems = 0; // how long the list can get before adding a new row - 0 equals no limit
  float mSeparatorSize = 2.; // The size in pixels of a separator. This could be width or height
  bool mScrollIfTooBig = false;
  const float TEXT_PAD = 5.; // 5px on either side of text
  float mPadding = 5.; // How much white space between the background and the cells
  IBlend mBlend = { kBlendNone, 0.f };
  float mRoundness = 5.f;
  float mDropShadowSize = 10.f;
  float mOpacity = 0.95f;
};
