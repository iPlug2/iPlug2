#pragma once

#include "IPlugBaseGraphics.h"
#include "IGraphics.h"

// A control is anything on the GUI, it could be a static bitmap, or
// something that moves or changes.  The control could manipulate
// bitmaps or do run-time vector drawing, or whatever.
//
// Some controls respond to mouse actions, either by moving a bitmap,
// transforming a bitmap, or cycling through a set of bitmaps.
// Other controls are readouts only.

class IControl
{
public:
  // If paramIdx is > -1, this control will be associated with a plugin parameter.
  IControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx = -1, IChannelBlend blendMethod = IChannelBlend::kBlendNone)
    : mPlug(plug), mRECT(rect), mTargetRECT(rect), mParamIdx(paramIdx), mValue(0.0), mDefaultValue(-1.0),
      mBlend(blendMethod), mDirty(true), mHide(false), mGrayed(false), mDisablePrompt(true), mDblAsSingleClick(false),
      mClampLo(0.0), mClampHi(1.0), mMOWhenGreyed(false), mTextEntryLength(DEFAULT_TEXT_ENTRY_LEN), 
      mValDisplayControl(0), mNameDisplayControl(0), mTooltip("") {}

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

  // By default, mouse double click has its own handler.  A control can set mDblAsSingleClick to true to change,
  // which maps double click to single click for this control (and also causes the mouse to be
  // captured by the control on double click).
  bool MouseDblAsSingleClick() { return mDblAsSingleClick; }

  virtual void Draw(IGraphics& graphics) = 0;

  // Ask the IGraphics object to open an edit box so the user can enter a value for this control.
  void PromptUserInput();
  void PromptUserInput(IRECT& textRect);
  
  inline void SetTooltip(const char* tooltip) { mTooltip.Set(tooltip); }
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
  virtual void TextFromTextEntry( const char* txt ) { return; } // does nothing by default

  virtual void Hide(bool hide);
  bool IsHidden() const { return mHide; }

  virtual void GrayOut(bool gray);
  bool IsGrayed() { return mGrayed; }

  bool GetMOWhenGrayed() { return mMOWhenGreyed; }

  // Override if you want the control to be hit only if a visible part of it is hit, or whatever.
  virtual bool IsHit(int x, int y) { return mTargetRECT.Contains(x, y); }

  void SetBlendMethod(IChannelBlend::EBlendMethod blendMethod) { mBlend = IChannelBlend(blendMethod); }
  
  void SetValDisplayControl(IControl* pValDisplayControl) { mValDisplayControl = pValDisplayControl; }
  void SetNameDisplayControl(IControl* pNameDisplayControl) { mNameDisplayControl = pNameDisplayControl; }

  virtual void SetDirty(bool pushParamToPlug = true);
  virtual void SetClean();
  virtual bool IsDirty() { return mDirty; }
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
    double mValue;
    int mParamIdx;
    
    AuxParam(int idx) : mParamIdx(idx)
    {
      assert(idx > -1); // no negative params please
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

protected:
  int mTextEntryLength;
  IText mText;
  IPlugBaseGraphics& mPlug;
  IRECT mRECT, mTargetRECT;
  int mParamIdx;
  
  WDL_TypedBuf<AuxParam> mAuxParams;
  double mValue, mDefaultValue, mClampLo, mClampHi;
  bool mDirty, mHide, mGrayed, mRedraw, mDisablePrompt, mClamped, mDblAsSingleClick, mMOWhenGreyed;
  IChannelBlend mBlend;
  IControl* mValDisplayControl;
  IControl* mNameDisplayControl;
  WDL_String mTooltip;
};

// Fills a rectangle with a colour
class IPanelControl : public IControl
{
public:
  IPanelControl(IPlugBaseGraphics& plug, IRECT rect, const IColor& color)
    : IControl(plug, rect), mColor(color) {}

  void Draw(IGraphics& graphics) override;

protected:
  IColor mColor;
};

// Draws a bitmap, or one frame of a stacked bitmap depending on the current value.
class IBitmapControl : public IControl
{
public:
  IBitmapControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& bitmap, IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone)
  : IControl(plug, IRECT(x, y, bitmap), paramIdx, blendMethod), mBitmap(bitmap) {}

  IBitmapControl(IPlugBaseGraphics& plug, int x, int y, IBitmap& bitmap, IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone)
  : IControl(plug, IRECT(x, y, bitmap), -1, blendMethod), mBitmap(bitmap) {}

  virtual ~IBitmapControl() {}

  virtual void Draw(IGraphics& graphics) override;
  virtual void OnRescale() override;
  
protected:
  IBitmap mBitmap;
};

// Output text to the screen.
class ITextControl : public IControl
{
public:
  ITextControl(IPlugBaseGraphics& plug, IRECT rect, IText& text, const char* str = "")
  : IControl(plug, rect)
  {
    mText = text;
    mStr.Set(str);
  }
  ~ITextControl() {}
  
  virtual void SetTextFromPlug(const char* pStr);
  virtual void ClearTextFromPlug() { SetTextFromPlug(""); }
  
  void Draw(IGraphics& graphics) override;
  
protected:
  WDL_String mStr;
};
