#pragma once

/**
 * @file
 * @copydoc IControl
 */

#ifdef VST3_API
#include "pluginterfaces/vst/ivstcontextmenu.h"
#include "base/source/fobject.h"
#endif

#include "IPlugBaseGraphics.h"
#include "IGraphics.h"

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
  /**
   Constructor

   @param plug The IPlugBaseGraphics that the control belongs to
   @param rect The rectangular area that the control occupies
   @param paramIdx If this is > -1 (kNoParameter) this control will be associated with a plugin parameter
   @param blendType Blend operation
   */
  IControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx = kNoParameter, IBlend blendType = IBlend::kBlendNone);
  virtual ~IControl() {}

  virtual void OnMouseDown(int x, int y, const IMouseMod& mod);
  virtual void OnMouseUp(int x, int y, const IMouseMod& mod) {}
  virtual void OnMouseDrag(int x, int y, int dX, int dY, const IMouseMod& mod) {}
  virtual void OnMouseDblClick(int x, int y, const IMouseMod& mod);
  virtual void OnMouseWheel(int x, int y, const IMouseMod& mod, int d) {};
  virtual bool OnKeyDown(int x, int y, int key) { return false; }
  
  // For efficiency, mouseovers/mouseouts are ignored unless you call IGraphics::HandleMouseOver.
  virtual void OnMouseOver(int x, int y, const IMouseMod& mod) {}
  virtual void OnMouseOut() {}
  
  /** Implement to do something when something was drag n dropped onto this control */
  virtual void OnDrop(const char* str) {};
  
  /** Implement to do something when graphics is scaled globally (e.g. moves to hidpi screen) */
  virtual void OnRescale() {}
  
  /** Called when IControl is constructed or resized using SetRect() */
  virtual void OnResize() {}
  
  /** Called by default when the user right clicks a control. If IGRAPHICS_NO_CONTEXT_MENU is enabled as a preprocessor macro right clicking control will mean IControl::CreateContextMenu() and IControl::OnContextSelection() do not function on right clicking control. VST3 provides contextual menu support which is hard wired to right click controls by default. You can add custom items to the menu by implementing IControl::CreateContextMenu() and handle them in IControl::OnContextSelection(). In non-VST 3 hosts right clicking will still create the menu, but it will not feature entries added by the host. */
  virtual void CreateContextMenu(IPopupMenu& contextMenu) {}
  
  virtual void TextFromTextEntry( const char* txt ) {}
  
  /** Called in response to a menu selection from CreateContextMenu(); /see CreateContextMenu() */
  virtual void OnContextSelection(int itemSelected) {}
  
  // By default, mouse double click has its own handler.  A control can set mDblAsSingleClick to true to change,
  // which maps double click to single click for this control (and also causes the mouse to be
  // captured by the control on double click).
  bool MouseDblAsSingleClick() { return mDblAsSingleClick; }

  virtual void Draw(IGraphics& graphics) = 0;
  
  virtual void DrawPTHighlight(IGraphics& graphics);
  virtual void SetPTParameterHighlight(bool isHighlighted, int color);
  
  // Create an edit box so the user can enter a value for this control.
  void PromptUserInput();
  void PromptUserInput(IRECT& rect);
  
  /** @param tooltip Text to be displayed */
  inline void SetTooltip(const char* tooltip) { mTooltip.Set(tooltip); }
  /** @return Currently set tooltip text */
  inline const char* GetTooltip() const { return mTooltip.Get(); }

  /** @return Parameter index */
  int ParamIdx() { return mParamIdx; }
  IParam* GetParam() { return mPlug.GetParam(mParamIdx); }
  virtual void SetValueFromPlug(double value);
  virtual void SetValueFromUserInput(double value);
  /** @return Value of the control */
  double GetValue() { return mValue; }

  IText& GetText() { return mText; }
  int GetTextEntryLength() { return mTextEntryLength; }
  void SetTextEntryLength(int len) { mTextEntryLength = len;  }
  void SetText(IText& txt) { mText = txt; }
  const IRECT& GetRECT() const { return mRECT; } // The draw area for this control.
  void SetRECT(IRECT& rect) { mRECT = rect; OnResize(); }
  const IRECT& GetTargetRECT() const { return mTargetRECT; } // The mouse target area (default = draw area).
  void SetTargetRECT(IRECT& rect) { mTargetRECT = rect; }


  /** Shows or hides the IControl.
   * @param hide Set to true to hide the control 
   */
  virtual void Hide(bool hide);
  /** @return \c True if the control is hidden. */
  bool IsHidden() const { return mHide; }

  /** Sets grayout for the control to be true or false
   * @param gray \c True for grayed out
   */
  virtual void GrayOut(bool gray);
  /** @return \c True if the control is grayed */
  bool IsGrayed() const { return mGrayed; }

  bool GetMOWhenGrayed() { return mMOWhenGreyed; }

  // Override if you want the control to be hit only if a visible part of it is hit, or whatever.
  virtual bool IsHit(int x, int y) const { return mTargetRECT.Contains(x, y); }

  void SetBlendType(IBlend::EType blendType) { mBlend = IBlend(blendType); }
  
  void SetValDisplayControl(IControl* pValDisplayControl) { mValDisplayControl = pValDisplayControl; }
  void SetNameDisplayControl(IControl* pNameDisplayControl) { mNameDisplayControl = pNameDisplayControl; }

  virtual void SetDirty(bool pushParamToPlug = true);
  virtual void SetClean();
  virtual bool IsDirty() { return mDirty; } // This is not const, because it may be used to update something at the fps
  void Clamp(double lo, double hi) { mClampLo = lo; mClampHi = hi; }
  void DisablePrompt(bool disable) { mDisablePrompt = disable; }  // Disables the right-click manual value entry.

  // Sometimes a control changes its state as part of its Draw method.
  // Redraw() prevents the control from being cleaned immediately after drawing.
  void Redraw() { mRedraw = true; }

  // This is an idle call from the GUI thread, as opposed to
  // IPlugBase::OnIdle which is called from the audio processing thread.
  // Only active if USE_IDLE_CALLS is defined.
  virtual void OnGUIIdle() {}
  
  
  /** A struct that contains a parameter index and normalized value */
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

  /** @name Auxiliary parameter
   *  TODO: Insert a meaningful description here
   */
  /**@{*/
  /** @return A pointer to the AuxParam instance at idx in the mAuxParams array */
  AuxParam* GetAuxParam(int idx);
  /** @return Index of the auxillary parameter that holds the paramIdx */
  int AuxParamIdx(int paramIdx);
  /** Adds an auxilliary parameter linked to paramIdx */
  void AddAuxParam(int paramIdx);
  /** Can override if necessary */
  virtual void SetAuxParamValueFromPlug(int auxParamIdx, double value);
  void SetAllAuxParamsFromGUI();
  int NAuxParams() { return mAuxParams.GetSize(); }
  /**@}*/
  
  IPlugBaseGraphics& GetPlug() { return mPlug; }
  IGraphics* GetGUI() { return mPlug.GetGUI(); }

#ifdef VST3_API
  Steinberg::tresult PLUGIN_API executeMenuItem (Steinberg::int32 tag) override { OnContextSelection(tag); return Steinberg::kResultOk; }
#endif

  void GetJSON(WDL_String& json, int idx) const;
  
protected:
  IPlugBaseGraphics& mPlug;
  IRECT mRECT;
  IRECT mTargetRECT;
  
  /** Parameter index or -1 (kNoParameter) */
  int mParamIdx;
  
  IBlend mBlend;
  IText mText;
  
  WDL_TypedBuf<AuxParam> mAuxParams;
  int mTextEntryLength = DEFAULT_TEXT_ENTRY_LEN;
  double mValue = 0.;
  double mDefaultValue = -1.;
  double mClampLo = 0.;
  double mClampHi = 1.;
  bool mDirty = true;
  bool mHide = false;
  bool mGrayed = false;
  bool mRedraw = false;
  bool mDisablePrompt = true;
  bool mClamped = false;
  bool mDblAsSingleClick = false;
  bool mMOWhenGreyed = false;
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

/** A basic control to fill a rectangle with a color */
class IPanelControl : public IControl
{
public:
  IPanelControl(IPlugBaseGraphics& plug, IRECT rect, const IColor& color)
  : IControl(plug, rect)
  , mColor(color)
  {}

  void Draw(IGraphics& graphics) override;

protected:
  IColor mColor;
};

/** A basic control to draw a bitmap, or one frame of a stacked bitmap depending on the current value. */
class IBitmapControl : public IControl
{
public:
  /** Creates a bitmap control with a given parameter
   * @param paramIdx Parameter index (-1 or KNoParameter, if this should not be linked to a parameter)
   * @param bitmap Image to be drawn
  */
  IBitmapControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& bitmap, IBlend::EType blendType = IBlend::kBlendNone)
  : IControl(plug, IRECT(x, y, bitmap), paramIdx, blendType), mBitmap(bitmap) {}

  /** Creates a bitmap control without a parameter */
  IBitmapControl(IPlugBaseGraphics& plug, int x, int y, IBitmap& bitmap, IBlend::EType blendType = IBlend::kBlendNone)
  : IControl(plug, IRECT(x, y, bitmap), kNoParameter, blendType), mBitmap(bitmap) {}

  virtual ~IBitmapControl() {}

  virtual void Draw(IGraphics& graphics) override;
  
  /** Implement to do something when graphics is scaled globally (e.g. moves to hidpi screen), if you override this make sure you call the parent method in order to rescale mBitmap */
  virtual void OnRescale() override;
  
protected:
  IBitmap mBitmap;
};

/** A basic control to draw an SVG image to the screen. */
class ISVGControl : public IControl
{
public:
  ISVGControl(IPlugBaseGraphics& plug, ISVG& svg, IRECT rect, int paramIdx)
    : IControl(plug, rect, paramIdx)
    , mSVG(svg)
  {
  };

  ~ISVGControl()
  {
  };

  void Draw(IGraphics& graphics);

private:
  //TODO: should draw the SVG to intermediate bitmap
  ISVG mSVG;
};

/** A basic control to output text to the screen. */
class ITextControl : public IControl
{
public:
  ITextControl(IPlugBaseGraphics& plug, IRECT rect, IText& text, const char* str = "")
  : IControl(plug, rect)
  , mStr(str)
  {
    mText = text;
  }
  
  ~ITextControl() {}
  
  virtual void SetTextFromPlug(const char* str);
  virtual void ClearTextFromPlug() { SetTextFromPlug(""); }
  
  void Draw(IGraphics& graphics) override;
  
protected:
  WDL_String mStr;
};
