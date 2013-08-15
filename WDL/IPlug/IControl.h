#ifndef _ICONTROL_
#define _ICONTROL_

#include "IPlugBase.h"
#include "IGraphics.h"

// A control is anything on the GUI, it could be a static bitmap, or
// something that moves or changes.  The control could manipulate
// bitmaps or do run-time vector drawing, or whatever.
//
// Some controls respond to mouse actions, either by moving a bitmap,
// transforming a bitmap, or cycling through a set of bitmaps.
// Other controls are readouts only.

#define DEFAULT_TEXT_ENTRY_LEN 7

class IControl
{
public:
  // If paramIdx is > -1, this control will be associated with a plugin parameter.
  IControl(IPlugBase* pPlug, IRECT pR, int paramIdx = -1, IChannelBlend blendMethod = IChannelBlend::kBlendNone)
    : mPlug(pPlug), mRECT(pR), mTargetRECT(pR), mParamIdx(paramIdx), mValue(0.0), mDefaultValue(-1.0),
      mBlend(blendMethod), mDirty(true), mHide(false), mGrayed(false), mDisablePrompt(true), mDblAsSingleClick(false),
      mClampLo(0.0), mClampHi(1.0), mMOWhenGreyed(false), mTextEntryLength(DEFAULT_TEXT_ENTRY_LEN), 
      mValDisplayControl(0), mNameDisplayControl(0), mTooltip(NULL) {}

  virtual ~IControl() {}

  virtual void OnMouseDown(int x, int y, IMouseMod* pMod);
  virtual void OnMouseUp(int x, int y, IMouseMod* pMod) {}
  virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod) {}
  virtual void OnMouseDblClick(int x, int y, IMouseMod* pMod);
  virtual void OnMouseWheel(int x, int y, IMouseMod* pMod, int d);
  virtual bool OnKeyDown(int x, int y, int key) { return false; }

  // For efficiency, mouseovers/mouseouts are ignored unless you call IGraphics::HandleMouseOver.
  virtual void OnMouseOver(int x, int y, IMouseMod* pMod) {}
  virtual void OnMouseOut() {}

  // By default, mouse double click has its own handler.  A control can set mDblAsSingleClick to true to change,
  // which maps double click to single click for this control (and also causes the mouse to be
  // captured by the control on double click).
  bool MouseDblAsSingleClick() { return mDblAsSingleClick; }

  virtual bool Draw(IGraphics* pGraphics) = 0;

  // Ask the IGraphics object to open an edit box so the user can enter a value for this control.
  void PromptUserInput();
  void PromptUserInput(IRECT* pTextRect);
  
  inline void SetTooltip(const char* tooltip) { mTooltip = tooltip; }
  inline const char* GetTooltip() const { return mTooltip; }

  int ParamIdx() { return mParamIdx; }
  IParam *GetParam() { return mPlug->GetParam(mParamIdx); }
  virtual void SetValueFromPlug(double value);
  void SetValueFromUserInput(double value);
  double GetValue() { return mValue; }

  IText* GetText() { return &mText; }
  int GetTextEntryLength() { return mTextEntryLength; }
  void SetText(IText* txt) { mText = *txt; }
  IRECT* GetRECT() { return &mRECT; }       // The draw area for this control.
  IRECT* GetTargetRECT() { return &mTargetRECT; } // The mouse target area (default = draw area).
  void SetTargetArea(IRECT pR) { mTargetRECT = pR; }
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
  
protected:
  int mTextEntryLength;
  IText mText;
  IPlugBase* mPlug;
  IRECT mRECT, mTargetRECT;
  int mParamIdx;
  
  WDL_TypedBuf<AuxParam> mAuxParams;
  double mValue, mDefaultValue, mClampLo, mClampHi;
  bool mDirty, mHide, mGrayed, mRedraw, mDisablePrompt, mClamped, mDblAsSingleClick, mMOWhenGreyed;
  IChannelBlend mBlend;
  IControl* mValDisplayControl;
  IControl* mNameDisplayControl;
  const char* mTooltip;
};

enum EDirection { kVertical, kHorizontal };

// Fills a rectangle with a colour
class IPanelControl : public IControl
{
public:
  IPanelControl(IPlugBase *pPlug, IRECT pR, const IColor* pColor)
    : IControl(pPlug, pR), mColor(*pColor) {}

  bool Draw(IGraphics* pGraphics);

protected:
  IColor mColor;
};

// Draws a bitmap, or one frame of a stacked bitmap depending on the current value.
class IBitmapControl : public IControl
{
public:
  IBitmapControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap,
                 IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone)
    : IControl(pPlug, IRECT(x, y, pBitmap), paramIdx, blendMethod), mBitmap(*pBitmap) {}

  IBitmapControl(IPlugBase* pPlug, int x, int y, IBitmap* pBitmap,
                 IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone)
    : IControl(pPlug, IRECT(x, y, pBitmap), -1, blendMethod), mBitmap(*pBitmap) {}

  virtual ~IBitmapControl() {}

  virtual bool Draw(IGraphics* pGraphics);

protected:
  IBitmap mBitmap;
};

// A switch.  Click to cycle through the bitmap states.
class ISwitchControl : public IBitmapControl
{
public:
  ISwitchControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap,
                 IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone)
    : IBitmapControl(pPlug, x, y, paramIdx, pBitmap, blendMethod) {}
  ~ISwitchControl() {}

  void OnMouseDblClick(int x, int y, IMouseMod* pMod);
  void OnMouseDown(int x, int y, IMouseMod* pMod);
};

// Like ISwitchControl except it puts up a popup menu instead of cycling through states on click
class ISwitchPopUpControl : public ISwitchControl
{
public:
  ISwitchPopUpControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap,
                 IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone)
  : ISwitchControl(pPlug, x, y, paramIdx, pBitmap, blendMethod)
  {
    mDisablePrompt = false;
  }
  
  ~ISwitchPopUpControl() {}
  
  void OnMouseDown(int x, int y, IMouseMod* pMod);
};

// A switch where each frame of the bitmap contains images for multiple button states. The Control's mRect will be divided into clickable areas.
class ISwitchFramesControl : public ISwitchControl
{
public:
  ISwitchFramesControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap, bool imagesAreHorizontal = false,
                       IChannelBlend::EBlendMethod blendMethod = IChannelBlend::kBlendNone);
  
  ~ISwitchFramesControl() {}
  
  void OnMouseDown(int x, int y, IMouseMod* pMod);
  
protected:
  WDL_TypedBuf<IRECT> mRECTs;
};

// On/off switch that has a target area only.
class IInvisibleSwitchControl : public IControl
{
public:
  IInvisibleSwitchControl(IPlugBase* pPlug, IRECT pR, int paramIdx);
  ~IInvisibleSwitchControl() {}

  void OnMouseDown(int x, int y, IMouseMod* pMod);

  virtual bool Draw(IGraphics* pGraphics) { return true; }
};

// A set of buttons that maps to a single selection.  Bitmap has 2 states, off and on.
class IRadioButtonsControl : public IControl
{
public:
  IRadioButtonsControl(IPlugBase* pPlug, IRECT pR, int paramIdx, int nButtons, IBitmap* pBitmap,
                       EDirection direction = kVertical, bool reverse = false);
  ~IRadioButtonsControl() {}

  void OnMouseDown(int x, int y, IMouseMod* pMod);
  bool Draw(IGraphics* pGraphics);

protected:
  WDL_TypedBuf<IRECT> mRECTs;
  IBitmap mBitmap;
};

// A switch that reverts to 0.0 when released.
class IContactControl : public ISwitchControl
{
public:
  IContactControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap)
    : ISwitchControl(pPlug, x, y, paramIdx, pBitmap) {}
  ~IContactControl() {}

  void OnMouseUp(int x, int y, IMouseMod* pMod);
};

// A fader. The bitmap snaps to a mouse click or drag.
class IFaderControl : public IControl
{
public:
  IFaderControl(IPlugBase* pPlug, int x, int y, int len, int paramIdx, IBitmap* pBitmap,
                EDirection direction = kVertical, bool onlyHandle = false);
  ~IFaderControl() {}

  int GetLength() const { return mLen; }
  // Size of the handle in pixels.
  int GetHandleHeadroom() const { return mHandleHeadroom; }
  // Size of the handle in terms of the control value.
  double GetHandleValueHeadroom() const { return (double) mHandleHeadroom / (double) mLen; }
  // Where is the handle right now?
  IRECT GetHandleRECT(double value = -1.0) const;

  virtual void OnMouseDown(int x, int y, IMouseMod* pMod);
  virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);

  virtual bool Draw(IGraphics* pGraphics);
  
  virtual bool IsHit(int x, int y);

protected:
  virtual void SnapToMouse(int x, int y);
  int mLen, mHandleHeadroom;
  IBitmap mBitmap;
  EDirection mDirection;
  bool mOnlyHandle; // if true only by clicking on the handle do you click the slider
};

const double DEFAULT_GEARING = 4.0;

// Parent for knobs, to handle mouse action and ballistics.
class IKnobControl : public IControl
{
public:
  IKnobControl(IPlugBase* pPlug, IRECT pR, int paramIdx, EDirection direction = kVertical,
               double gearing = DEFAULT_GEARING)
    : IControl(pPlug, pR, paramIdx), mDirection(direction), mGearing(gearing) {}
  virtual ~IKnobControl() {}

  void SetGearing(double gearing) { mGearing = gearing; }
  virtual void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod);

protected:
  EDirection mDirection;
  double mGearing;
};

// A knob that is just a line.
class IKnobLineControl : public IKnobControl
{
public:
  IKnobLineControl(IPlugBase* pPlug, IRECT pR, int paramIdx,
                   const IColor* pColor, double innerRadius = 0.0, double outerRadius = 0.0,
                   double minAngle = -0.75 * PI, double maxAngle = 0.75 * PI,
                   EDirection direction = kVertical, double gearing = DEFAULT_GEARING);
  ~IKnobLineControl() {}

  bool Draw(IGraphics* pGraphics);

protected:
  IColor mColor;
  float mMinAngle, mMaxAngle, mInnerRadius, mOuterRadius;
};

// A rotating knob.  The bitmap rotates with any mouse drag.
class IKnobRotaterControl : public IKnobControl
{
public:
  IKnobRotaterControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap,
                      double minAngle = -0.75 * PI, double maxAngle = 0.75 * PI, int yOffsetZeroDeg = 0,
                      EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
    : IKnobControl(pPlug, IRECT(x, y, pBitmap), paramIdx, direction, gearing),
      mBitmap(*pBitmap), mMinAngle(minAngle), mMaxAngle(maxAngle), mYOffset(yOffsetZeroDeg) {}
  ~IKnobRotaterControl() {}

  bool Draw(IGraphics* pGraphics);

protected:
  IBitmap mBitmap;
  double mMinAngle, mMaxAngle;
  int mYOffset;
};

// A multibitmap knob.  The bitmap cycles through states as the mouse drags.
class IKnobMultiControl : public IKnobControl
{
public:
  IKnobMultiControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap,
                    EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
    : IKnobControl(pPlug, IRECT(x, y, pBitmap), paramIdx, direction, gearing), mBitmap(*pBitmap) {}
  ~IKnobMultiControl() {}

  bool Draw(IGraphics* pGraphics);

protected:
  IBitmap mBitmap;
};

// A knob that consists of a static base, a rotating mask, and a rotating top.
// The bitmaps are assumed to be symmetrical and identical sizes.
class IKnobRotatingMaskControl : public IKnobControl
{
public:
  IKnobRotatingMaskControl(IPlugBase* pPlug, int x, int y, int paramIdx,
                           IBitmap* pBase, IBitmap* pMask, IBitmap* pTop,
                           double minAngle = -0.75 * PI, double maxAngle = 0.75 * PI,
                           EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
    : IKnobControl(pPlug, IRECT(x, y, pBase), paramIdx, direction, gearing),
      mBase(*pBase), mMask(*pMask), mTop(*pTop), mMinAngle(minAngle), mMaxAngle(maxAngle) {}
  ~IKnobRotatingMaskControl() {}

  bool Draw(IGraphics* pGraphics);

protected:
  IBitmap mBase, mMask, mTop;
  double mMinAngle, mMaxAngle;
};

// Bitmap shows when value = 0, then toggles its target area to the whole bitmap
// and waits for another click to hide itself.
class IBitmapOverlayControl : public ISwitchControl
{
public:
  IBitmapOverlayControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap, IRECT pTargetArea)
    : ISwitchControl(pPlug, x, y, paramIdx, pBitmap), mTargetArea(pTargetArea) {}

  IBitmapOverlayControl(IPlugBase* pPlug, int x, int y, IBitmap* pBitmap, IRECT pTargetArea)
    : ISwitchControl(pPlug, x, y, -1, pBitmap), mTargetArea(pTargetArea) {}

  ~IBitmapOverlayControl() {}

  bool Draw(IGraphics* pGraphics);

protected:
  IRECT mTargetArea;  // Keep this around to swap in & out.
};

// Output text to the screen.
class ITextControl : public IControl
{
public:
  ITextControl(IPlugBase* pPlug, IRECT pR, IText* pText, const char* str = "")
    : IControl(pPlug, pR)
  {
    mText = *pText;
    mStr.Set(str);
  }
  ~ITextControl() {}

  void SetTextFromPlug(char* str);
  void ClearTextFromPlug() { SetTextFromPlug( (char *) ""); }

  bool Draw(IGraphics* pGraphics);

protected:
  WDL_String mStr;
};

// If paramIdx is specified, the text is automatically set to the output
// of Param::GetDisplayForHost().  If showParamLabel = true, Param::GetLabelForHost() is appended.
class ICaptionControl : public ITextControl
{
public:
  ICaptionControl(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, bool showParamLabel = true);
  ~ICaptionControl() {}

  virtual void OnMouseDown(int x, int y, IMouseMod* pMod);
  virtual void OnMouseDblClick(int x, int y, IMouseMod* pMod);

  bool Draw(IGraphics* pGraphics);

protected:
  bool mShowParamLabel;
};

#define MAX_URL_LEN 256
#define MAX_NET_ERR_MSG_LEN 1024

class IURLControl : public IControl
{
public:
  IURLControl(IPlugBase* pPlug, IRECT pR, const char* url, const char* backupURL = 0, const char* errMsgOnFailure = 0);
  ~IURLControl() {}

  void OnMouseDown(int x, int y, IMouseMod* pMod);
  bool Draw(IGraphics* pGraphics) { return true; }

protected:
  char mURL[MAX_URL_LEN], mBackupURL[MAX_URL_LEN], mErrMsg[MAX_NET_ERR_MSG_LEN];
};

// This is a weird control for a few reasons.
// - Although its numeric mValue is not meaningful, it needs to be associated with a plugin parameter
// so it can inform the plug when the file selection has changed. If the associated plugin parameter is
// declared after kNumParams in the EParams enum, the parameter will be a dummy for this purpose only.
// - Because it puts up a modal window, it needs to redraw itself twice when it's dirty,
// because moving the modal window will clear the first dirty state.
class IFileSelectorControl : public IControl
{
public:
  enum EFileSelectorState { kFSNone, kFSSelecting, kFSDone };

  IFileSelectorControl(IPlugBase* pPlug, IRECT pR, int paramIdx, IBitmap* pBitmap,
                       EFileAction action, char* dir = "", char* extensions = "")     // extensions = "txt wav" for example.
    : IControl(pPlug, pR, paramIdx), mBitmap(*pBitmap),
      mFileAction(action), mDir(dir), mExtensions(extensions), mState(kFSNone) {}
  ~IFileSelectorControl() {}

  void OnMouseDown(int x, int y, IMouseMod* pMod);

  void GetLastSelectedFileForPlug(WDL_String* pStr);
  void SetLastSelectedFileFromPlug(char* file);

  bool Draw(IGraphics* pGraphics);
  bool IsDirty();

protected:
  IBitmap mBitmap;
  WDL_String mDir, mFile, mExtensions;
  EFileAction mFileAction;
  EFileSelectorState mState;
};

#endif
