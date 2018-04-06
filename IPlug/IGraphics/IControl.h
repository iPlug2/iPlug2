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

#include "wdlstring.h"
#include "dirscan.h"
#include "ptrlist.h"

#include "IGraphics.h"
#include "IPlugDelegate.h"

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
   * @param dlg The class implementing the IDelegate interface that will handle parameter changes.
   * In a plug-in, this would typically be your main plug-in class, which inherits from IPlugBaseGraphics, which implements the interface.
   * If you're doing something using IGraphics without IPlugBaseGraphics (e.g. drawing into an extra window), you need to implement the delegate interface somewhere
   * to handle parameter changes.
   * @param bounds The rectangular area that the control occupies
   * @param paramIdx If this is > -1 (kNoParameter) this control will be associated with a dlgin parameter
   * @param actionFunc pass in a lambda function to provide custom functionality when the control "action" happens (usually mouse down). */
  IControl(IDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter, IActionFunction actionFunc = nullptr);
  
  IControl(IDelegate& dlg, IRECT bounds, IActionFunction actionFunc);

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

  /** Called by default when the user right clicks a control. If IGRAPHICS_NO_CONTEXT_MENU is enabled as a preprocessor macro right clicking control will mean IControl::CreateContextMenu() and IControl::OnContextSelection() do not function on right clicking control. VST3 provides contextual menu support which is hard wired to right click controls by default. You can add custom items to the menu by implementing IControl::CreateContextMenu() and handle them in IControl::OnContextSelection(). In non-VST 3 hosts right clicking will still create the menu, but it will not feature entries added by the host. */
  virtual void CreateContextMenu(IPopupMenu& contextMenu) {}

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
  
  /** This method is called from the class implementing the IDelegate interface in order to update a controls mValue and set it to be marked
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

  // This is an idle call from the GUI thread, as opposed to
  // IPlugProcessor::OnIdle which is called from the audio processing thread.
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

  /** Gets a reference to the class implementing the IDelegate interface that handles parameter changes from this IGraphics instance.
   * If you need to call other methods on that class, you can use dynamic_cast<ImplementorClass>(GetDelegate();
   * @return The class implementing the IDelegate interface that handles parameter changes from this IGraphics instance.*/
  IDelegate& GetDelegate() { return mDelegate; }
  
  void SetGraphics(IGraphics* pGraphics) { mGraphics = pGraphics; }
  IGraphics* GetUI() { return mGraphics; }

  void GetJSON(WDL_String& json, int idx) const;

  bool GetMouseIsOver() { return mMouseIsOver; }
  
  void SnapToMouse(float x, float y, EDirection direction, IRECT& bounds, float scalar = 1.);
  
  virtual void Animate(double progress) {}

  void EndAnimation()
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
  IDelegate& mDelegate;
  IGraphics* mGraphics = nullptr;
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
              const IColor* pHLColor = 0,
              const IColor* pSHColor = 0,
              const IColor* pX1Color = 0,
              const IColor* pX2Color = 0,
              const IColor* pX3Color = 0)
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
  
protected:
  IControl* mControl = nullptr;
  WDL_TypedBuf<IColor> mColors;
  float mRoundness = 0.f;
  float mShadowOffset = 3.f;
  float mFrameThickness = 2.f;
  bool mDrawFrame = true;
  bool mDrawShadows = true;
  bool mEmboss = false;
};

/** A basic control to fill a rectangle with a color */
class IPanelControl : public IControl
{
public:
  IPanelControl(IDelegate& dlg, IRECT bounds, const IColor& color)
  : IControl(dlg, bounds)
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
  /** Creates a bitmap control with a given parameter
   * @param paramIdx Parameter index (-1 or kNoParameter, if this should not be linked to a parameter)
   * @param bitmap Image to be drawn */
  IBitmapControl(IDelegate& dlg, float x, float y, int paramIdx, IBitmap& bitmap, EBlendType blend = kBlendNone)
  : IControl(dlg, IRECT(x, y, bitmap), paramIdx)
  , IBitmapBase(bitmap, blend)
  {}

  /** Creates a bitmap control without a parameter */
  IBitmapControl(IDelegate& dlg, float x, float y, IBitmap& bitmap, EBlendType blend = kBlendNone)
  : IControl(dlg, IRECT(x, y, bitmap), kNoParameter)
  , IBitmapBase(bitmap, blend)
  {
  }

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

/** A basic control to draw an SVG image to the screen. */
class ISVGControl : public IControl
{
public:
  ISVGControl(IDelegate& dlg, ISVG& svg, IRECT bounds, int paramIdx)
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

/** A basic control to output text to the screen. */
class ITextControl : public IControl
{
public:
  ITextControl(IDelegate& dlg, IRECT bounds, const IText& text, const char* str = "")
  : IControl(dlg, bounds)
  , mStr(str)
  {
    IControl::mText = text;
  }

  ~ITextControl() {}

  virtual void SetTextFromDelegate(const char* str);
  virtual void ClearTextFromDelegate() { SetTextFromDelegate(""); }

  void Draw(IGraphics& g) override;

protected:
  WDL_String mStr;
};

class ICaptionControl : public ITextControl
{
public:
  ICaptionControl(IDelegate& dlg, IRECT bounds, int paramIdx, const IText& text, bool showParamLabel = true);
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
  IKnobControlBase(IDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter,
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
  ISliderControlBase(IDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter,
                     EDirection dir = kVertical, bool onlyHandle = false, int handleSize = 0)
  : IControl(dlg, bounds, paramIdx)
  , mDirection(dir)
  , mOnlyHandle(onlyHandle)
  {
    handleSize == 0 ? mHandleSize = bounds.W() : mHandleSize = handleSize;
  }
  
  ISliderControlBase(IDelegate& dlg, IRECT bounds, IActionFunction aF = nullptr,
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

/** Parent for switch controls (including buttons a.k.a. momentary switches)*/
class ISwitchControlBase : public IControl
{
public:
  ISwitchControlBase(IDelegate& dlg, IRECT bounds, int paramIdx = kNoParameter, IActionFunction aF = nullptr,
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
  IDirBrowseControlBase(IDelegate& dlg, IRECT bounds, const char* extension /* e.g. ".txt"*/)
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

void DefaultAnimationFunc(IControl* pCaller);
void DefaultClickActionFunc(IControl* pCaller);
