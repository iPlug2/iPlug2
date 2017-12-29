#pragma once

#include "IControl.h"

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

//TODO: fix Centre/Right aligned behaviour when string exceeds bounds or should wrap onto new line

class IBitmapTextControl : public IControl
{
public:
  IBitmapTextControl(IPlugBaseGraphics& plug, IRECT rect, IBitmap& bitmap, const char* str = "", IText* text = 0, int charWidth = 6, int charHeight = 12, int charOffset = 0, bool multiLine = false, bool vCenter = true)
  : IControl(plug, rect)
  , mTextBitmap(bitmap)
  , mCharWidth(charWidth)
  , mCharHeight(charHeight)
  , mCharOffset(charOffset)
  , mMultiLine(multiLine)
  , mVCentre(vCenter)
  {
    mStr.Set(str);
  }
  
  ~IBitmapTextControl() {}
  
  void SetTextFromPlug(const char* str)
  {
    if (strcmp(mStr.Get(), str))
    {
      SetDirty(false);
      mStr.Set(str);
    }
  }
  
  void ClearTextFromPlug()
  {
    SetTextFromPlug("");
  }
  
  void Draw(IGraphics& graphics)
  {
    if (CSTR_NOT_EMPTY(mStr.Get()))
    {
      graphics.DrawBitmapedText(mTextBitmap, mRECT, mText, &mBlend, mStr.Get(), mVCentre, mMultiLine, mCharWidth, mCharHeight, mCharOffset);
    }
  }
  
protected:
  WDL_String mStr;
  int mCharWidth, mCharHeight, mCharOffset;
  IBitmap mTextBitmap;
  bool mMultiLine;
  bool mVCentre;
};

