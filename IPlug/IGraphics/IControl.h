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
   * @param rect The rectangular area that the control occupies
   * @param paramIdx If this is > -1 (kNoParameter) this control will be associated with a dlgin parameter
   * @param actionFunc pass in a lambda function to provide custom functionality when the control "action" happens (usually mouse down). */
  IControl(IDelegate& dlg, IRECT rect, int paramIdx = kNoParameter, IActionFunction actionFunc = nullptr);
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

  /** Implement to do something when graphics is scaled globally (e.g. moves to hidpi screen) */
  virtual void OnRescale() {}

  /** Called when IControl is constructed or resized using SetRect() */
  virtual void OnResize() {}

  /** Called by default when the user right clicks a control. If IGRAPHICS_NO_CONTEXT_MENU is enabled as a preprocessor macro right clicking control will mean IControl::CreateContextMenu() and IControl::OnContextSelection() do not function on right clicking control. VST3 provides contextual menu support which is hard wired to right click controls by default. You can add custom items to the menu by implementing IControl::CreateContextMenu() and handle them in IControl::OnContextSelection(). In non-VST 3 hosts right clicking will still create the menu, but it will not feature entries added by the host. */
  virtual void CreateContextMenu(IPopupMenu& contextMenu) {}

  virtual void OnTextEntryCompletion(const char* txt) {}

  /** Called in response to a menu selection from CreateContextMenu(); /see CreateContextMenu() */
  virtual void OnContextSelection(int itemSelected) {}

  // By default, mouse double click has its own handler.  A control can set mDblAsSingleClick to true to change,
  // which maps double click to single click for this control (and also causes the mouse to be
  // captured by the control on double click).
  bool MouseDblAsSingleClick() { return mDblAsSingleClick; }

  virtual void Draw(IGraphics& graphics) = 0;

  virtual void DrawPTHighlight(IGraphics& graphics);
  virtual void SetPTParameterHighlight(bool isHighlighted, int color);

  /** Create an edit box so the user can enter a value for this control, or pop up a pop-up menu, if we are linked to a parameter. */
  void PromptUserInput();
  
  /** Create an edit box so the user can enter a value for this control, or pop up a pop-up menu,
   * if we are linked to a parameter, specifying bounds for the text entry/pop-up menu corner */
  void PromptUserInput(IRECT& rect);
  
  inline void SetActionFunction(IActionFunction actionFunc) { mActionFunc = actionFunc; }

  /** @param tooltip Text to be displayed */
  inline void SetTooltip(const char* tooltip) { mTooltip.Set(tooltip); }
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
  void SetRECT(const IRECT& rect) { mRECT = rect; OnResize(); }
  const IRECT& GetTargetRECT() const { return mTargetRECT; } // The mouse target area (default = draw area).
  void SetTargetRECT(const IRECT& rect) { mTargetRECT = rect; }


  /** Shows or hides the IControl.
   * @param hide Set to true to hide the control */
  virtual void Hide(bool hide);
  
  /** @return \c True if the control is hidden. */
  bool IsHidden() const { return mHide; }

  /** Sets grayout for the control to be true or false
   * @param gray \c True for grayed out*/
  virtual void GrayOut(bool gray);
  
  /** @return \c True if the control is grayed */
  bool IsGrayed() const { return mGrayed; }

  void SetMOWhenGrayed(bool allow) { mMOWhenGrayed = allow; }
  void SetMEWhenGrayed(bool allow) { mMEWhenGrayed = allow; }
  bool GetMOWhenGrayed() const { return mMOWhenGrayed; }
  bool GetMEWhenGrayed() const { return mMEWhenGrayed; }

  // Override if you want the control to be hit only if a visible part of it is hit, or whatever.
  virtual bool IsHit(float x, float y) const { return mTargetRECT.Contains(x, y); }

  void SetBlend(IBlend blend) { mBlend = blend; }

  void SetValDisplayControl(IControl* pValDisplayControl) { mValDisplayControl = pValDisplayControl; }
  void SetNameDisplayControl(IControl* pNameDisplayControl) { mNameDisplayControl = pNameDisplayControl; }

  virtual void SetDirty(bool pushParamToDelegate = true);
  virtual void SetClean();
  virtual bool IsDirty() { return mDirty; } // This is not const, because it may be overridden and used to update something at the fps
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
  
#ifdef VST3_API
  Steinberg::tresult PLUGIN_API executeMenuItem (Steinberg::int32 tag) override { OnContextSelection(tag); return Steinberg::kResultOk; }
#endif

#pragma mark - IControl Member variables
protected:
  IDelegate& mDelegate;
  IGraphics* mGraphics = nullptr;
  IRECT mRECT;
  IRECT mTargetRECT;

  IActionFunction mActionFunc = nullptr;

  /** Parameter index or -1 (kNoParameter) */
  int mParamIdx = kNoParameter;

  IBlend mBlend;
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
};

#pragma mark - BASIC CONTROLS AND BASE CLASSES

/** A An interface for IVControls, in order for them to share a common set of colors. If you need more flexibility for theming, you're on your own! */
class IVectorBase
{
public:
  IVectorBase(const IColor* pBGColor = &DEFAULT_BGCOLOR,
              const IColor* pFGColor = &DEFAULT_FGCOLOR,
              const IColor* pFRColor = 0,
              const IColor* pHLColor = 0,
              const IColor* pX1Color = 0,
              const IColor* pX2Color = 0,
              const IColor* pX3Color = 0)
  {
    SetColors(pBGColor, pFGColor, pFRColor, pHLColor, pX1Color, pX2Color, pX3Color);
  }

  IVectorBase(const IVColorSpec& spec)
  {
    SetColors(&spec.mBGColor,
              &spec.mFGColor,
              &spec.mFRColor,
              &spec.mHLColor,
              &spec.mX1Color,
              &spec.mX2Color,
              &spec.mX3Color);
  }

  void AddColor(const IColor& color)
  {
    mColors.Add(color);
  }

  void SetColor(int colorIdx, const IColor& color)
  {
    if(colorIdx < mColors.GetSize())
      mColors.Get()[colorIdx] = color;
  }

  void SetColors(IVColorSpec& spec)
  {
    SetColors(&spec.mBGColor,
              &spec.mFGColor,
              &spec.mFRColor,
              &spec.mHLColor,
              &spec.mX1Color,
              &spec.mX2Color,
              &spec.mX3Color);
  }

  void SetColors(const IColor* pBGColor = 0,
                 const IColor* pFGColor = 0,
                 const IColor* pFRColor = 0,
                 const IColor* pHLColor = 0,
                 const IColor* pX1Color = 0,
                 const IColor* pX2Color = 0,
                 const IColor* pX3Color = 0)
  {
    if(pBGColor) AddColor(*pBGColor);
    if(pFGColor) AddColor(*pFGColor);
    if(pFRColor) AddColor(*pFRColor);
    if(pHLColor) AddColor(*pHLColor);
    if(pX1Color) AddColor(*pX1Color);
    if(pX2Color) AddColor(*pX2Color);
    if(pX3Color) AddColor(*pX3Color);
  }

  IColor& GetColor(int colorIdx)
  {
    if(colorIdx < mColors.GetSize())
      return mColors.Get()[colorIdx];
    else
      return mColors.Get()[0];
  }
protected:
  WDL_TypedBuf<IColor> mColors;
};

/** A basic control to fill a rectangle with a color */
class IPanelControl : public IControl, public IVectorBase
{
public:
  IPanelControl(IDelegate& dlg, IRECT rect, const IColor& color)
  : IControl(dlg, rect)
  , IVectorBase(&color)
  {}

  void Draw(IGraphics& graphics) override;
};

/** A basic control to draw a bitmap, or one frame of a stacked bitmap depending on the current value. */
class IBitmapControl : public IControl
{
public:
  /** Creates a bitmap control with a given parameter
   * @param paramIdx Parameter index (-1 or KNoParameter, if this should not be linked to a parameter)
   * @param bitmap Image to be drawn
  */
  IBitmapControl(IDelegate& dlg, float x, float y, int paramIdx, IBitmap& bitmap, EBlendType blend = kBlendNone)
  : IControl(dlg, IRECT(x, y, bitmap), paramIdx)
  , mBitmap(bitmap)
  {
    mBlend = blend;
  }

  /** Creates a bitmap control without a parameter */
  IBitmapControl(IDelegate& dlg, float x, float y, IBitmap& bitmap, EBlendType blend = kBlendNone)
  : IControl(dlg, IRECT(x, y, bitmap), kNoParameter)
  , mBitmap(bitmap)
  {
    mBlend = blend;
  }

  virtual ~IBitmapControl() {}

  virtual void Draw(IGraphics& graphics) override;

  /** Implement to do something when graphics is scaled globally (e.g. moves to hidpi screen),
   *  if you override this make sure you call the parent method in order to rescale mBitmap */
  virtual void OnRescale() override;

protected:
  IBitmap mBitmap;
};

/** A basic control to draw an SVG image to the screen. */
class ISVGControl : public IControl
{
public:
  ISVGControl(IDelegate& dlg, ISVG& svg, IRECT rect, int paramIdx)
    : IControl(dlg, rect, paramIdx)
    , mSVG(svg)
  {}

  virtual ~ISVGControl() {}

  virtual void Draw(IGraphics& graphics) override;

private:
  //TODO: cache the SVG to intermediate bitmap?
  ISVG mSVG;
};

/** A basic control to output text to the screen. */
class ITextControl : public IControl
{
public:
  ITextControl(IDelegate& dlg, IRECT rect, const IText& text, const char* str = "")
  : IControl(dlg, rect)
  , mStr(str)
  {
    IControl::mText = text;
  }

  ~ITextControl() {}

  virtual void SetTextFromDelegate(const char* str);
  virtual void ClearTextFromDelegate() { SetTextFromDelegate(""); }

  void Draw(IGraphics& graphics) override;

protected:
  WDL_String mStr;
};

class ICaptionControl : public ITextControl
{
public:
  ICaptionControl(IDelegate& dlg, IRECT rect, int paramIdx, const IText& text, bool showParamLabel = true);
  ~ICaptionControl() {}
  
  virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void Draw(IGraphics& graphics) override;

protected:
  bool mShowParamLabel;
};

#pragma mark - Base Controls

/** Parent for knobs, to handle mouse action and ballistics. */
class IKnobControlBase : public IControl
{
public:
  IKnobControlBase(IDelegate& dlg, IRECT rect, int param = kNoParameter,
    EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
    : IControl(dlg, rect, param)
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

/** Parent for switch controls (including buttons a.k.a. momentary switches)*/
class ISwitchControlBase : public IControl
{
public:
  ISwitchControlBase(IDelegate& dlg, IRECT rect, int param = kNoParameter, IActionFunction aF = nullptr,
    uint32_t numStates = 2);

  virtual ~ISwitchControlBase() {}

  virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override;

protected:
  uint32_t mNumStates;
};

/** An abstract IControl base class that you can inherit from in order to make a control that pops up a menu to browse files */
class IDirBrowseControlBase : public IControl
{
public:
  IDirBrowseControlBase(IDelegate& dlg, IRECT rect, const char* extension /* e.g. ".txt"*/)
  : IControl(dlg, rect)
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
