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

// A switch.  Click to cycle through the bitmap states.
class ISwitchControl : public IBitmapControl
{
public:
  ISwitchControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& bitmap, IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone)
  : IBitmapControl(plug, x, y, paramIdx, bitmap, blendMethod) {}
  ~ISwitchControl() {}

  void OnMouseDblClick(int x, int y, const IMouseMod& mod) override;
  void OnMouseDown(int x, int y, const IMouseMod& mod) override;
};

// Like ISwitchControl except it puts up a popup menu instead of cycling through states on click
class ISwitchPopUpControl : public ISwitchControl
{
public:
  ISwitchPopUpControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& bitmap, IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone)
  : ISwitchControl(plug, x, y, paramIdx, bitmap, blendMethod)
  {
    mDisablePrompt = false;
  }
  
  ~ISwitchPopUpControl() {}
  
  void OnMouseDown(int x, int y, const IMouseMod& mod) override;
};

// A switch where each frame of the bitmap contains images for multiple button states. The Control's mRect will be divided into clickable areas.
class ISwitchFramesControl : public ISwitchControl
{
public:
  ISwitchFramesControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& bitmap, bool imagesAreHorizontal = false, IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone);
  ~ISwitchFramesControl() {}
  
  void OnMouseDown(int x, int y, const IMouseMod& mod) override;

protected:
  WDL_TypedBuf<IRECT> mRECTs;
};

// On/off switch that has a target area only.
class IInvisibleSwitchControl : public IControl
{
public:
  IInvisibleSwitchControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx);
  ~IInvisibleSwitchControl() {}

  void OnMouseDown(int x, int y, const IMouseMod& mod) override;
};

// A set of buttons that maps to a single selection.  Bitmap has 2 states, off and on.
class IRadioButtonsControl : public IControl
{
public:
  IRadioButtonsControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, int nButtons, IBitmap& bitmap, EDirection direction = kVertical, bool reverse = false);
  ~IRadioButtonsControl() {}

  void OnMouseDown(int x, int y, const IMouseMod& mod) override;
  void Draw(IGraphics& graphics) override;

protected:
  WDL_TypedBuf<IRECT> mRECTs;
  IBitmap mBitmap;
};

// A switch that reverts to 0.0 when released.
class IContactControl : public ISwitchControl
{
public:
  IContactControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& bitmap)
    : ISwitchControl(plug, x, y, paramIdx, bitmap) {}
  ~IContactControl() {}

  void OnMouseUp(int x, int y, const IMouseMod& mod) override;
  virtual void Draw(IGraphics& graphics) override {}
};

// A fader. The bitmap snaps to a mouse click or drag.
class IFaderControl : public IControl
{
public:
  IFaderControl(IPlugBaseGraphics& plug, int x, int y, int len, int paramIdx, IBitmap& bitmap,
                EDirection direction = kVertical, bool onlyHandle = false);
  ~IFaderControl() {}

  int GetLength() const { return mLen; }
  // Size of the handle in pixels.
  int GetHandleHeadroom() const { return mHandleHeadroom; }
  // Size of the handle in terms of the control value.
  double GetHandleValueHeadroom() const { return (double) mHandleHeadroom / (double) mLen; }
  // Where is the handle right now?
  IRECT GetHandleRECT(double value = -1.0) const;

  virtual void OnMouseDown(int x, int y, const IMouseMod& mod) override;
  virtual void OnMouseDrag(int x, int y, int dX, int dY, const IMouseMod& mod) override;
  virtual void OnMouseWheel(int x, int y, const IMouseMod& mod, int d) override;

  virtual void Draw(IGraphics& graphics) override;
  
  virtual bool IsHit(int x, int y) override;
  virtual void OnRescale() override;

protected:
  virtual void SnapToMouse(int x, int y);
  int mLen, mHandleHeadroom;
  IBitmap mBitmap;
  EDirection mDirection;
  bool mOnlyHandle; // if true only by clicking on the handle do you click the slider
  
};

// Parent for knobs, to handle mouse action and ballistics.
class IKnobControl : public IControl
{
public:
  IKnobControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
  : IControl(plug, rect, paramIdx), mDirection(direction), mGearing(gearing) {}
  virtual ~IKnobControl() {}

  void SetGearing(double gearing) { mGearing = gearing; }
  virtual void OnMouseDrag(int x, int y, int dX, int dY, const IMouseMod& mod) override;
  virtual void OnMouseWheel(int x, int y, const IMouseMod& mod, int d) override;

protected:
  EDirection mDirection;
  double mGearing;
};

// A knob that is just a line.
class IKnobLineControl : public IKnobControl
{
public:
  IKnobLineControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, const IColor& color, double innerRadius = 10, double outerRadius = 20.,
                   double minAngle = -0.75 * PI, double maxAngle = 0.75 * PI,
                   EDirection direction = kVertical, double gearing = DEFAULT_GEARING);
  ~IKnobLineControl() {}

  void Draw(IGraphics& graphics) override;

protected:
  IColor mColor;
  float mMinAngle, mMaxAngle, mInnerRadius, mOuterRadius;
};

// A rotating knob.  The bitmap rotates with any mouse drag.
class IKnobRotaterControl : public IKnobControl
{
public:
  IKnobRotaterControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& bitmap, double minAngle = -0.75 * PI, double maxAngle = 0.75 * PI, int yOffsetZeroDeg = 0, EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
  : IKnobControl(plug, IRECT(x, y, bitmap), paramIdx, direction, gearing)
  , mBitmap(bitmap), mMinAngle(minAngle), mMaxAngle(maxAngle), mYOffset(yOffsetZeroDeg) {}
  ~IKnobRotaterControl() {}

  void Draw(IGraphics& graphics) override;

protected:
  IBitmap mBitmap;
  double mMinAngle, mMaxAngle;
  int mYOffset;
};

// A multibitmap knob.  The bitmap cycles through states as the mouse drags.
class IKnobMultiControl : public IKnobControl
{
public:
  IKnobMultiControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& bitmap, EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
  : IKnobControl(plug, IRECT(x, y, bitmap), paramIdx, direction, gearing), mBitmap(bitmap) {}
  ~IKnobMultiControl() {}

  void Draw(IGraphics& graphics) override;
  virtual void OnRescale() override;

protected:
  IBitmap mBitmap;
};

// A knob that consists of a static base, a rotating mask, and a rotating top.
// The bitmaps are assumed to be symmetrical and identical sizes.
class IKnobRotatingMaskControl : public IKnobControl
{
public:
  IKnobRotatingMaskControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& base, IBitmap& mask, IBitmap& top, double minAngle = -0.75 * PI, double maxAngle = 0.75 * PI, EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
    : IKnobControl(plug, IRECT(x, y, base), paramIdx, direction, gearing),
      mBase(base), mMask(mask), mTop(top), mMinAngle(minAngle), mMaxAngle(maxAngle) {}
  ~IKnobRotatingMaskControl() {}

  void Draw(IGraphics& graphics) override;

protected:
  IBitmap mBase, mMask, mTop;
  double mMinAngle, mMaxAngle;
};

// Bitmap shows when value = 0, then toggles its target area to the whole bitmap
// and waits for another click to hide itself.
class IBitmapOverlayControl : public ISwitchControl
{
public:
  IBitmapOverlayControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& bitmap, IRECT targetArea)
  : ISwitchControl(plug, x, y, paramIdx, bitmap)
  , mTargetArea(targetArea) {}

  IBitmapOverlayControl(IPlugBaseGraphics& plug, int x, int y, IBitmap& bitmap, IRECT targetArea)
  : ISwitchControl(plug, x, y, -1, bitmap)
  , mTargetArea(targetArea) {}

  ~IBitmapOverlayControl() {}

  void Draw(IGraphics& graphics) override;

protected:
  IRECT mTargetArea;  // Keep this around to swap in & out.
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

// If paramIdx is specified, the text is automatically set to the output
// of Param::GetDisplayForHost().  If showParamLabel = true, Param::GetLabelForHost() is appended.
class ICaptionControl : public ITextControl
{
public:
  ICaptionControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, IText& text, bool showParamLabel = true);
  ~ICaptionControl() {}

  virtual void OnMouseDown(int x, int y, const IMouseMod& mod) override;
  virtual void OnMouseDblClick(int x, int y, const IMouseMod& mod) override;

  void Draw(IGraphics& graphics) override;

protected:
  bool mShowParamLabel;
};

class IURLControl : public IControl
{
public:
  IURLControl(IPlugBaseGraphics& plug, IRECT rect, const char* URL, const char* backupURL = 0, const char* errMsgOnFailure = 0);
  ~IURLControl() {}

  void OnMouseDown(int x, int y, const IMouseMod& mod);
  virtual void Draw(IGraphics& graphics) {}

protected:
  char mURL[MAX_URL_LEN], mBackupURL[MAX_URL_LEN], mErrMsg[MAX_NET_ERR_MSG_LEN];
};

class IFileSelectorControl : public IControl
{
public:
  enum EFileSelectorState { kFSNone, kFSSelecting, kFSDone };

  IFileSelectorControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, IBitmap& bitmap, EFileAction action, const char* dir = "", const char* extensions = "")
  : IControl(plug, rect, paramIdx)
  , mBitmap(bitmap) , mFileAction(action), mDir(dir), mExtensions(extensions), mState(kFSNone) {}
  ~IFileSelectorControl() {}

  void OnMouseDown(int x, int y, const IMouseMod& mod) override;

  void GetLastSelectedFileForPlug(WDL_String& pStr);
  void SetLastSelectedFileFromPlug(const char* file);

  void Draw(IGraphics& graphics) override;
  bool IsDirty() override;

protected:
  IBitmap mBitmap;
  WDL_String mDir, mFile, mExtensions;
  EFileAction mFileAction;
  EFileSelectorState mState;
};
