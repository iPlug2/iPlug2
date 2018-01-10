#pragma once

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
  IControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx = kNoParameter, IBlend blendType = IBlend::kBlendNone)
  : mPlug(plug)
  , mRECT(rect)
  , mTargetRECT(rect)
  , mParamIdx(paramIdx)
  , mBlend(blendType)
  {
  }

  virtual ~IControl() {}

  virtual void OnMouseDown(int x, int y, const IMouseMod& mod);
  virtual void OnMouseUp(int x, int y, const IMouseMod& mod) {}
  virtual void OnMouseDrag(int x, int y, int dX, int dY, const IMouseMod& mod) {}
  virtual void OnMouseDblClick(int x, int y, const IMouseMod& mod);
  virtual void OnMouseWheel(int x, int y, const IMouseMod& mod, int d) {};
  virtual bool OnKeyDown(int x, int y, int key) { return false; }
  virtual void CreateContextMenu(IPopupMenu& contextMenu) {}

  // For efficiency, mouseovers/mouseouts are ignored unless you call IGraphics::HandleMouseOver.
  virtual void OnMouseOver(int x, int y, const IMouseMod& mod) {}
  virtual void OnMouseOut() {}
  
  // Something was drag n dropped
  virtual void OnDrop(const char* str) {};
  
  // By default, mouse double click has its own handler.  A control can set mDblAsSingleClick to true to change,
  // which maps double click to single click for this control (and also causes the mouse to be
  // captured by the control on double click).
  bool MouseDblAsSingleClick() { return mDblAsSingleClick; }

  virtual void Draw(IGraphics& graphics) = 0;

  virtual void DrawPTHighlight(IGraphics& graphics);
  virtual void SetPTParameterHighlight(bool isHighlighted, int color);
  
  // Ask the IGraphics object to open an edit box so the user can enter a value for this control.
  void PromptUserInput();
  void PromptUserInput(IRECT& rect);
  
  /** @param tooltip Text to be displayed */
  inline void SetTooltip(const char* tooltip) { mTooltip.Set(tooltip); }
  /** \return Currently set tooltip text */
  inline const char* GetTooltip() const { return mTooltip.Get(); }

  int ParamIdx() { return mParamIdx; }
  IParam* GetParam() { return mPlug.GetParam(mParamIdx); }
  virtual void SetValueFromPlug(double value);
  virtual void SetValueFromUserInput(double value);
  double GetValue() { return mValue; }

  IText& GetText() { return mText; }
  int GetTextEntryLength() { return mTextEntryLength; }
  void SetTextEntryLength(int len) { mTextEntryLength = len;  }
  void SetText(IText& txt) { mText = txt; }
  const IRECT& GetRECT() const { return mRECT; } // The draw area for this control.
  const IRECT& GetTargetRECT() const { return mTargetRECT; } // The mouse target area (default = draw area).
  void SetTargetArea(IRECT rect) { mTargetRECT = rect; }
  virtual void TextFromTextEntry( const char* txt ) {}
  virtual void OnContextSelection(int itemSelected) {}

  /** Shows or hides the IControl.
   * @param hide Set to true to hide the control 
   */
  virtual void Hide(bool hide);
  /** @return \c True if the control is hidden. */
  bool IsHidden() const { return mHide; }

  virtual void GrayOut(bool gray);
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
  
  // a struct that contain a parameter index and normalized value
  struct AuxParam 
  {
    double mValue  = 0.;
    int mParamIdx;
    
    AuxParam(int idx) : mParamIdx(idx)
    {
      assert(idx > kNoParameter); // no negative params please
    }
  };
  
  // return a pointer to the AuxParam instance at idx in the mAuxParams array
  AuxParam* GetAuxParam(int idx);
  // return the index of the auxillary parameter that holds the paramIdx
  int AuxParamIdx(int paramIdx);
  // add an auxilliary parameter linked to paramIdx
  void AddAuxParam(int paramIdx);
  virtual void SetAuxParamValueFromPlug(int auxParamIdx, double value); // can override if nessecary
  void SetAllAuxParamsFromGUI();
  int NAuxParams() { return mAuxParams.GetSize(); }
  
  IPlugBaseGraphics& GetPlug() { return mPlug; }
  IGraphics* GetGUI() { return mPlug.GetGUI(); }
  
  virtual void OnRescale() {};
  
#ifdef VST3_API
  Steinberg::tresult PLUGIN_API executeMenuItem (Steinberg::int32 tag) override { OnContextSelection(tag); return Steinberg::kResultOk; }
#endif

protected:
  IPlugBaseGraphics& mPlug;
  IRECT mRECT, mTargetRECT;
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
  IBitmapControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& bitmap, IBlend::EType blendType = IBlend::kBlendNone)
  : IControl(plug, IRECT(x, y, bitmap), paramIdx, blendType), mBitmap(bitmap) {}

  IBitmapControl(IPlugBaseGraphics& plug, int x, int y, IBitmap& bitmap, IBlend::EType blendType = IBlend::kBlendNone)
  : IControl(plug, IRECT(x, y, bitmap), kNoParameter, blendType), mBitmap(bitmap) {}

  virtual ~IBitmapControl() {}

  virtual void Draw(IGraphics& graphics) override;
  virtual void OnRescale() override;
  
protected:
  IBitmap mBitmap;
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
