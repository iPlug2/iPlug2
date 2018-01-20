#pragma once

/**
 * @file
 * @brief A collection of IControls for common UI widgets, such as knobs, sliders, switches
 */

#include "IControl.h"

/**
 * \defgroup Controls IGraphics::IControls
 * @{
 */

/** A switch. Click to cycle through the bitmap states. */
class ISwitchControl : public IBitmapControl
{
public:
  ISwitchControl(IPlugBaseGraphics& plug, float x, float y, int paramIdx, IBitmap& bitmap, IBlend::EType blendMethod = IBlend::kBlendNone)
  : IBitmapControl(plug, x, y, paramIdx, bitmap, blendMethod) {}
  ~ISwitchControl() {}
  
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
};

/** A vector switch control. Click to cycle through states. */
class IVSwitchControl : public IControl
{
public:
  IVSwitchControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, int numStates = 0, const IColor& fgColor = COLOR_BLACK, const IColor& bgColor = COLOR_WHITE, EDirection dir = kVertical, IBlend::EType blendMethod = IBlend::kBlendNone)
  : IControl(plug, rect, paramIdx, blendMethod)
  , mFGColor(fgColor)
  , mBGColor(bgColor)
  , mState(0)
  , mDirection(dir)
  {
    if(paramIdx > -1)
    {
      mNumStates = (int) mPlug.GetParam(paramIdx)->GetRange() + 1;
    }
    else
    {
      assert(numStates > 2);
      mNumStates = numStates;
    }
  }
  
  ~IVSwitchControl() {}
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mState = (mState + 1) % mNumStates;
    mValue = mState / double (mNumStates-1);
    SetDirty();
  }
  
  void Draw(IGraphics& graphics)  override
  {
    graphics.FillRect(mBGColor, mRECT);
    
    IRECT handle;
    
    if(mDirection == kHorizontal)
      handle = mRECT.SubRectHorizontal(mNumStates, mState);
    if(mDirection == kVertical)
      handle = mRECT.SubRectVertical(mNumStates, mState);
  
    graphics.FillRect(mFGColor, handle);
  }
  
private:
  int mState;
  int mNumStates;
  IColor mFGColor;
  IColor mBGColor;
  EDirection mDirection;
};

/** Like ISwitchControl except it puts up a popup menu instead of cycling through states on click. */
class ISwitchPopUpControl : public ISwitchControl
{
public:
  ISwitchPopUpControl(IPlugBaseGraphics& plug, float x, float y, int paramIdx, IBitmap& bitmap, IBlend::EType blendMethod = IBlend::kBlendNone)
  : ISwitchControl(plug, x, y, paramIdx, bitmap, blendMethod)
  {
    mDisablePrompt = false;
  }
  
  ~ISwitchPopUpControl() {}
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
};

/** A switch where each frame of the bitmap contains images for multiple button states. The Control's mRect will be divided into clickable areas. */
class ISwitchFramesControl : public ISwitchControl
{
public:
  ISwitchFramesControl(IPlugBaseGraphics& plug, float x, float y, int paramIdx, IBitmap& bitmap, bool imagesAreHorizontal = false, IBlend::EType blendMethod = IBlend::kBlendNone);
  ~ISwitchFramesControl() {}
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  
protected:
  WDL_TypedBuf<IRECT> mRECTs;
};

/** On/Off switch that has a target area only. */
class IInvisibleSwitchControl : public IControl
{
public:
  IInvisibleSwitchControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx);
  ~IInvisibleSwitchControl() {}
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
};

/** A set of buttons that maps to a single selection. The Bitmap has 2 states, Off and On. */
class IRadioButtonsControl : public IControl
{
public:
  IRadioButtonsControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, int nButtons, IBitmap& bitmap, EDirection direction = kVertical, bool reverse = false);
  ~IRadioButtonsControl() {}
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void Draw(IGraphics& graphics) override;
  
protected:
  WDL_TypedBuf<IRECT> mRECTs;
  IBitmap mBitmap;
};

/** A switch that reverts to 0.0 when released. */
class IContactControl : public ISwitchControl
{
public:
  IContactControl(IPlugBaseGraphics& plug, float x, float y, int paramIdx, IBitmap& bitmap)
  : ISwitchControl(plug, x, y, paramIdx, bitmap) {}
  ~IContactControl() {}
  
  void OnMouseUp(float x, float y, const IMouseMod& mod) override;
};

/** A fader with a bitmap for the handle. The bitmap snaps to a mouse click or drag. */
class IFaderControl : public IControl
{
public:
  IFaderControl(IPlugBaseGraphics& plug, float x, float y, int len, int paramIdx, IBitmap& bitmap,
                EDirection direction = kVertical, bool onlyHandle = false);
  ~IFaderControl() {}
  
  int GetLength() const { return mLen; }
  int GetHandleHeadroom() const { return mHandleHeadroom; }
  double GetHandleValueHeadroom() const { return (double) mHandleHeadroom / (double) mLen; }
  IRECT GetHandleRECT(double value = -1.0) const;
  virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  virtual void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  virtual void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
  virtual void Draw(IGraphics& graphics) override;
  virtual bool IsHit(float x, float y) const override;
  virtual void OnRescale() override;
  
protected:
  virtual void SnapToMouse(float x, float y);
  int mLen, mHandleHeadroom;
  IBitmap mBitmap;
  EDirection mDirection;
  bool mOnlyHandle;
  
};

/** Parent for knobs, to handle mouse action and ballistics. */
class IKnobControl : public IControl
{
public:
  IKnobControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
  : IControl(plug, rect, paramIdx), mDirection(direction), mGearing(gearing) {}
  virtual ~IKnobControl() {}
  
  void SetGearing(double gearing) { mGearing = gearing; }
  virtual void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override;
  virtual void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
  
protected:
  EDirection mDirection;
  double mGearing;
};

/** A knob that is just a line. */
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

/** A rotating knob.  The bitmap rotates with any mouse drag. */
class IKnobRotaterControl : public IKnobControl
{
public:
  IKnobRotaterControl(IPlugBaseGraphics& plug, float x, float y, int paramIdx, IBitmap& bitmap, double minAngle = -0.75 * PI, double maxAngle = 0.75 * PI, int yOffsetZeroDeg = 0, EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
  : IKnobControl(plug, IRECT(x, y, bitmap), paramIdx, direction, gearing)
  , mBitmap(bitmap), mMinAngle(minAngle), mMaxAngle(maxAngle), mYOffset(yOffsetZeroDeg) {}
  ~IKnobRotaterControl() {}
  
  void Draw(IGraphics& graphics) override;
  
protected:
  IBitmap mBitmap;
  double mMinAngle, mMaxAngle;
  int mYOffset;
};

/** A multibitmap knob.  The bitmap cycles through states as the mouse drags. */
class IKnobMultiControl : public IKnobControl
{
public:
  IKnobMultiControl(IPlugBaseGraphics& plug, float x, float y, int paramIdx, IBitmap& bitmap, EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
  : IKnobControl(plug, IRECT(x, y, bitmap), paramIdx, direction, gearing), mBitmap(bitmap) {}
  ~IKnobMultiControl() {}
  
  void Draw(IGraphics& graphics) override;
  virtual void OnRescale() override;
  
protected:
  IBitmap mBitmap;
};

/** A knob that consists of a static base, a rotating mask, and a rotating top.
 *  The bitmaps are assumed to be symmetrical and identical sizes.
*/
class IKnobRotatingMaskControl : public IKnobControl
{
public:
  IKnobRotatingMaskControl(IPlugBaseGraphics& plug, float x, float y, int paramIdx, IBitmap& base, IBitmap& mask, IBitmap& top, double minAngle = -0.75 * PI, double maxAngle = 0.75 * PI, EDirection direction = kVertical, double gearing = DEFAULT_GEARING)
  : IKnobControl(plug, IRECT(x, y, base), paramIdx, direction, gearing),
  mBase(base), mMask(mask), mTop(top), mMinAngle(minAngle), mMaxAngle(maxAngle) {}
  ~IKnobRotatingMaskControl() {}
  
  void Draw(IGraphics& graphics) override;
  
protected:
  IBitmap mBase, mMask, mTop;
  double mMinAngle, mMaxAngle;
};

/** Bitmap shows when value = 0, then toggles its target area to the whole bitmap and waits for another click to hide itself. */
class IBitmapOverlayControl : public ISwitchControl
{
public:
  IBitmapOverlayControl(IPlugBaseGraphics& plug, float x, float y, int paramIdx, IBitmap& bitmap, IRECT targetArea)
  : ISwitchControl(plug, x, y, paramIdx, bitmap)
  , mTargetArea(targetArea) {}
  
  IBitmapOverlayControl(IPlugBaseGraphics& plug, float x, float y, IBitmap& bitmap, IRECT targetArea)
  : ISwitchControl(plug, x, y, kNoParameter, bitmap)
  , mTargetArea(targetArea) {}
  
  ~IBitmapOverlayControl() {}
  
  void Draw(IGraphics& graphics) override;
  
protected:
  IRECT mTargetArea;
};

/** Displays the value of a parameter.
 * If paramIdx is specified, the text is automatically set to the output of Param::GetDisplayForHost().
 * If showParamLabel = true, Param::GetLabelForHost() is appended.  
*/
class ICaptionControl : public ITextControl
{
public:
  ICaptionControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, IText& text, bool showParamLabel = true);
  ~ICaptionControl() {}
  
  virtual void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  virtual void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;
  
  void Draw(IGraphics& graphics) override;
  
protected:
  bool mShowParamLabel;
};

/** Clickable URL area */
class IURLControl : public IControl
{
public:
  IURLControl(IPlugBaseGraphics& plug, IRECT rect, const char* URL, const char* backupURL = 0, const char* errMsgOnFailure = 0);
  ~IURLControl() {}
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  void Draw(IGraphics& graphics) override {}
  
protected:
  WDL_String mURL, mBackupURL, mErrMsg;
};

/** A control to allow selection of a file from the file system */
// TODO: does this actually work?
class IFileSelectorControl : public IControl
{
public:
  enum EFileSelectorState { kFSNone, kFSSelecting, kFSDone };
  
  IFileSelectorControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, IBitmap& bitmap, EFileAction action, const char* dir = "", const char* extensions = "")
  : IControl(plug, rect, paramIdx)
  , mBitmap(bitmap)
  , mFileAction(action)
  , mDir(dir)
  , mExtensions(extensions) 
  {}
  ~IFileSelectorControl() {}
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  
  void GetLastSelectedFileForPlug(WDL_String& str);
  void SetLastSelectedFileFromPlug(const char* file);
  
  void Draw(IGraphics& graphics) override;
  bool IsDirty() override;
  
protected:
  IBitmap mBitmap;
  WDL_String mDir, mFile, mExtensions;
  EFileAction mFileAction;
  EFileSelectorState mState = kFSNone;
};

/** Display monospace bitmap font text */
// TODO: fix Centre/Right aligned behaviour when string exceeds bounds or should wrap onto new line
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
  
  void Draw(IGraphics& graphics) override
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

class IXYPad : public IControl
{
public:
  IXYPad(IPlugBaseGraphics& pPlug, IRECT rect, int handleRadius, int paramIdxX, int paramIdxY, const IColor& hcOn = COLOR_WHITE, const IColor& hcOff = COLOR_BLACK)
  : IControl(pPlug, rect)
  , mHandleRadius(handleRadius)
  , mHandleColorOn(hcOn)
  , mHandleColorOff(hcOff)
  , mCurrentHandleColor(hcOff)
  {
    AddAuxParam(paramIdxX);
    AddAuxParam(paramIdxY);
  }
  
  void Draw(IGraphics& graphics) override
  {
    const double xpos = GetAuxParam(0)->mValue * mRECT.W();
    const double ypos = GetAuxParam(1)->mValue * mRECT.H();
    
    graphics.DrawLine(mCurrentHandleColor, xpos+mRECT.L, mRECT.T, xpos+mRECT.L, mRECT.B, 0);
    graphics.DrawLine(mCurrentHandleColor, mRECT.L, ypos+mRECT.T, mRECT.R, ypos+mRECT.T, 0);
    graphics.FillCircle(mCurrentHandleColor, xpos+mRECT.L, ypos+mRECT.T, mHandleRadius, 0);
}

  void OnMouseDown(float x, float y, const IMouseMod& pMod) override
  {
    mCurrentHandleColor = mHandleColorOn;
    return SnapToMouse(x, y);
  }
  
  void OnMouseUp(float x, float y, const IMouseMod& pMod) override
  {
    mCurrentHandleColor = mHandleColorOff;
  }
  
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& pMod) override
  {
    return SnapToMouse(x, y);
  }
  
  void SnapToMouse(float x, float y)
  {
    GetAuxParam(0)->mValue = BOUNDED((double)x / (double)mRECT.W(), 0, 1);
    GetAuxParam(1)->mValue = BOUNDED((double)y / (double)mRECT.H(), 0, 1);
    
    SetDirty();
  }
  
  void SetDirty(bool pushParamToPlug = true) override
  {
    mDirty = true;
    
    if (pushParamToPlug)
    {
      SetAllAuxParamsFromGUI();
    }
  }
private:
  int mHandleRadius;
  IColor mHandleColorOff;
  IColor mHandleColorOn;
  IColor mCurrentHandleColor;
};

//class ITestPopupMenu : public IControl
//{
//private:
//  IPopupMenu mMainMenu, mSubMenu;
//
//public:
//  ITestPopupMenu(IPlugBase *pPlug, IRECT pR)
//  : IControl(pPlug, pR, -1)
//  {
//    mMainMenu.AddItem("first item");
//    mMainMenu.AddItem("second item");
//    mMainMenu.AddItem("third item");
//
//    mSubMenu.AddItem("first item");
//    mSubMenu.AddItem("second item");
//    mSubMenu.AddItem("third item");
//
//    mMainMenu.AddItem("sub menu", &mSubMenu);
//  }
//
//  void Draw(IGraphics& graphics) override
//  {
//    return pGraphics->FillRect(&COLOR_WHITE, &mRECT);;
//  }
//
//  void OnMouseDown(float x, float y, IMouseMod* pMod) override
//  {
//    doPopupMenu();
//
//    Redraw(); // seems to need this
//    SetDirty();
//  }
//
//  void doPopupMenu()
//  {
//    IPopupMenu* selectedMenu = mPlug->GetGUI()->CreateIPopupMenu(&mMainMenu, &mRECT);
//
//    if (selectedMenu == &mMainMenu)
//    {
//      int itemChosen = selectedMenu->GetChosenItemIdx();
//      selectedMenu->CheckItemAlone(itemChosen);
//      DBGMSG("item chosen, main menu %i\n", itemChosen);
//    }
//    else if (selectedMenu == &mSubMenu)
//    {
//      int itemChosen = selectedMenu->GetChosenItemIdx();
//      selectedMenu->CheckItemAlone(itemChosen);
//      DBGMSG("item chosen, sub menu %i\n", itemChosen);
//    }
//    else
//    {
//      DBGMSG("nothing chosen\n");
//    }
//  }
//};
//
//class ITestPopupMenuB : public IControl
//{
//private:
//  IPopupMenu mMainMenu;
//
//public:
//  ITestPopupMenuB(IPlugBase *pPlug, IRECT pR)
//  : IControl(pPlug, pR, -1)
//  {
//    mMainMenu.SetMultiCheck(true);
//    mMainMenu.AddItem("first item");
//    mMainMenu.AddItem("second item");
//    mMainMenu.AddItem("third item");
//  }
//
//  void Draw(IGraphics& graphics) override
//  {
//    return pGraphics->FillRect(&COLOR_WHITE, &mRECT);;
//  }
//
//  void OnMouseDown(float x, float y, IMouseMod* pMod) override
//  {
//    doPopupMenu();
//
//    Redraw(); // seems to need this
//    SetDirty();
//  }
//
//  void doPopupMenu()
//  {
//    IPopupMenu* selectedMenu = mPlug->GetGUI()->CreateIPopupMenu(&mMainMenu, &mRECT);
//
//    if(selectedMenu)
//    {
//      int idx = selectedMenu->GetChosenItemIdx();
//      selectedMenu->CheckItem(idx, !selectedMenu->IsItemChecked(idx));
//
//      WDL_String checkedItems;
//
//      checkedItems.Append("checked: ", 1024);
//
//      for (int i = 0; i < selectedMenu->GetNItems(); i++)
//      {
//        checkedItems.AppendFormatted(1024, "%i ", selectedMenu->IsItemChecked(i));
//      }
//
//      DBGMSG("%s\n", checkedItems.Get());
//    }
//  }
//};
//
//class IPresetMenu : public IControl
//{
//private:
//  WDL_String mDisp;
//public:
//  IPresetMenu(IPlugBase *pPlug, IRECT pR)
//  : IControl(pPlug, pR, -1)
//  {
//    mTextEntryLength = MAX_PRESET_NAME_LEN - 3;
//    mText = IText(14, &COLOR_BLACK, "Arial", IText::kStyleNormal, IText::kAlignNear);
//  }
//
//  void Draw(IGraphics& graphics) override
//  {
//    int pNumber = mPlug->GetCurrentPresetIdx();
//    mDisp.SetFormatted(32, "%02d: %s", pNumber+1, mPlug->GetPresetName(pNumber));
//
//    pGraphics->FillRect(&COLOR_WHITE, &mRECT);
//
//    if (CSTR_NOT_EMPTY(mDisp.Get()))
//    {
//      graphics.DrawText(&mText, mDisp.Get(), &mRECT);
//    }
//  }
//
//  void OnMouseDown(float x, float y, IMouseMod* pMod) override
//  {
//    if (pMod->R)
//    {
//      const char* pname = mPlug->GetPresetName(mPlug->GetCurrentPresetIdx());
//      mPlug->GetGUI()->CreateTextEntry(this, &mText, &mRECT, pname);
//    }
//    else
//    {
//      doPopupMenu();
//    }
//
//    Redraw(); // seems to need this
//    SetDirty();
//  }
//
//  void doPopupMenu()
//  {
//    int numItems = mPlug->NPresets();
//    IPopupMenu menu;
//
//    IGraphics* gui = mPlug->GetGUI();
//
//    int currentPresetIdx = mPlug->GetCurrentPresetIdx();
//
//    for(int i = 0; i< numItems; i++)
//    {
//      const char* str = mPlug->GetPresetName(i);
//      if (i == currentPresetIdx)
//        menu.AddItem(str, -1, IPopupMenuItem::kChecked);
//      else
//        menu.AddItem(str);
//    }
//
//    menu.SetPrefix(2);
//
//    if(gui->CreateIPopupMenu(&menu, &mRECT))
//    {
//      int itemChosen = menu.GetChosenItemIdx();
//
//      if (itemChosen > -1)
//      {
//        mPlug->RestorePreset(itemChosen);
//        mPlug->InformHostOfProgramChange();
//        mPlug->DirtyParameters();
//      }
//    }
//  }
//
//  void TextFromTextEntry(const char* txt)
//  {
//    WDL_String safeName;
//    safeName.Set(txt, MAX_PRESET_NAME_LEN);
//
//    mPlug->ModifyCurrentPreset(safeName.Get());
//    mPlug->InformHostOfProgramChange();
//    mPlug->DirtyParameters();
//    SetDirty(false);
//  }
//};
//
//class IPopUpMenuControl : public IControl
//{
//public:
//  IPopUpMenuControl(IPlugBase *pPlug, IRECT pR, int paramIdx)
//  : IControl(pPlug, pR, paramIdx)
//  {
//    mDisablePrompt = false;
//    mDblAsSingleClick = true;
//    mText = IText(14);
//  }
//
//  void Draw(IGraphics& graphics) override
//  {
//    pGraphics->FillRect(&COLOR_WHITE, &mRECT);
//
//    char disp[32];
//    mPlug->GetParam(mParamIdx)->GetDisplayForHost(disp);
//
//    if (CSTR_NOT_EMPTY(disp))
//    {
//      graphics.DrawText(&mText, disp, &mRECT);
//    }
//  }
//
//  void OnMouseDown(float x, float y, IMouseMod* pMod) override
//  {
//    if (pMod->L)
//    {
//      PromptUserInput(&mRECT);
//    }
//
//    mPlug->GetGUI()->SetAllControlsDirty();
//  }
//
//  //void OnMouseWheel(float x, float y, IMouseMod* pMod, float d) override {} //TODO: popup menus seem to hog the mousewheel
//
//};
//
//// Key catcher is an icontrol but only its OnKeyDown() is called... after all the other controls have been tested to see if they want keyboard input
//class IKeyCatcher : public IControl
//{
//public:
//  IKeyCatcher(IPlugBase* pPlug, IRECT pR)
//  : IControl(pPlug, pR) {}
//
//  // this never gets called but is needed for an IControl
//  void Draw(IGraphics& graphics) { return false; }
//
//  bool OnKeyDown(float x, float y, int key) override
//  {
//    switch (key)
//    {
//        //case KEY_SPACE:
//        ///  DBGMSG("Space\n");
//        //  return true;
//      case KEY_LEFTARROW:;
//        DBGMSG("Left\n");
//        return true;
//      case KEY_RIGHTARROW:
//        DBGMSG("Right\n");
//        return true;
//      case KEY_UPARROW:;
//        DBGMSG("Up\n");
//        return true;
//      case KEY_DOWNARROW:
//        DBGMSG("Down\n");
//        return true;
//      default:
//        return false;
//    }
//  }
//};
//
//class ITempoDisplay : public IControl
//{
//private:
//  ITimeInfo* mTimeInfo;
//  WDL_String mDisplay;
//
//public:
//  ITempoDisplay(IPlugBase* pPlug, IRECT pR, IText* pText, ITimeInfo* pTimeInfo)
//  : IControl(pPlug, pR)
//  {
//    mText = *pText;
//    mTimeInfo = pTimeInfo;
//  }
//
//  void Draw(IGraphics& graphics) override
//  {
//    mDisplay.SetFormatted(80, "Tempo: %f, SamplePos: %i, PPQPos: %f", mTimeInfo->mTempo, (int) mTimeInfo->mSamplePos, mTimeInfo->mPPQPos);
//    graphics.DrawText(&mText, mDisplay.Get(), &mRECT);
//  }
//
//  bool IsDirty() { return true;}
//};
//
//class IKnobMultiControlText : public IKnobControl
//{
//private:
//  IRECT mTextRECT, mImgRECT;
//  IBitmap mBitmap;
//
//public:
//  IKnobMultiControlText(IPlugBase* pPlug, IRECT pR, int paramIdx, IBitmap* pBitmap, IText* pText)
//  : IKnobControl(pPlug, pR, paramIdx), mBitmap(*pBitmap)
//  {
//    mText = *pText;
//    mTextRECT = IRECT(mRECT.L, mRECT.B-20, mRECT.R, mRECT.B);
//    mImgRECT = IRECT(mRECT.L, mRECT.T, &mBitmap);
//    mDisablePrompt = false;
//  }
//
//  ~IKnobMultiControlText() {}
//
//  void Draw(IGraphics& graphics) override
//  {
//    int i = 1 + int(0.5 + mValue * (double) (mBitmap.N - 1));
//    i = BOUNDED(i, 1, mBitmap.N);
//    pGraphics->DrawBitmap(&mBitmap, &mImgRECT, i, &mBlend);
//    //pGraphics->FillRect(&COLOR_WHITE, &mTextRECT);
//
//    char disp[20];
//    mPlug->GetParam(mParamIdx)->GetDisplayForHost(disp);
//
//    if (CSTR_NOT_EMPTY(disp))
//    {
//      graphics.DrawText(&mText, disp, &mTextRECT);
//    }
//  }
//
//  void OnMouseDown(float x, float y, IMouseMod* pMod) override
//  {
//    if (mTextRECT.Contains(x, y)) PromptUserInput(&mTextRECT);
//#ifdef RTAS_API
//    else if (pMod->A)
//    {
//      if (mDefaultValue >= 0.0)
//      {
//        mValue = mDefaultValue;
//        SetDirty();
//      }
//    }
//#endif
//    else
//    {
//      OnMouseDrag(x, y, 0, 0, pMod);
//    }
//  }
//
//  void OnMouseDblClick(float x, float y, IMouseMod* pMod) override
//  {
//#ifdef PROTOOLS
//    PromptUserInput(&mTextRECT);
//#else
//    if (mDefaultValue >= 0.0)
//    {
//      mValue = mDefaultValue;
//      SetDirty();
//    }
//#endif
//  }
//
//};
//
//class IPeakMeterVert : public IControl
//{
//public:
//
//  IPeakMeterVert(IPlugBase* pPlug, IRECT pR)
//  : IControl(pPlug, pR)
//  {
//    mColor = COLOR_BLUE;
//  }
//
//  ~IPeakMeterVert() {}
//
//  void Draw(IGraphics& graphics) override
//  {
//    //IRECT(mRECT.L, mRECT.T, mRECT.W , mRECT.T + (mValue * mRECT.H));
//    pGraphics->FillRect(&COLOR_RED, &mRECT);
//
//    //pGraphics->FillRect(&COLOR_BLUE, &mRECT);
//
//    IRECT filledBit = IRECT(mRECT.L, mRECT.T, mRECT.R , mRECT.B - (mValue * mRECT.H()));
//    pGraphics->FillRect(&mColor, &filledBit);
//    return true;
//  }
//
//  bool IsDirty() { return true;}
//
//protected:
//  IColor mColor;
//};
//
//class IPeakMeterHoriz : public IPeakMeterVert
//{
//public:
//
//  void Draw(IGraphics& graphics) override
//  {
//    pGraphics->FillRect(&COLOR_BLUE, &mRECT);
//    IRECT filledBit = IRECT(mRECT.L, mRECT.T, mRECT.L + (mValue * mRECT.W() ) , mRECT.B );
//    pGraphics->FillRect(&mColor, &filledBit);
//    return true;
//  }
//};

//class MultiSliderControlV: public IControl
//{
//public:
//  MultiSliderControlV(IPlugBase *pPlug,
//                      IRECT pR,
//                      int paramIdx,
//                      int numSliders,
//                      int handleWidth,
//                      const IColor* bgcolor,
//                      const IColor* fgcolor,
//                      const IColor* hlcolor)
//  : IControl(pPlug, pR, paramIdx)
//  {
//    mBgColor = *bgcolor;
//    mFgColor = *fgcolor;
//    mHlColor = *hlcolor;
//
//    mNumSliders = numSliders;
//    mHighlighted = -1;
//    mGrain = 0.001;
//    mSliderThatChanged = -1;
//
//    float sliderWidth = floor((float) mRECT.W() / (float) numSliders);
//
//    mSteps = new double[numSliders];
//
//    for(int i=0; i<numSliders; i++)
//    {
//      int lpos = (i * sliderWidth);
//      mSteps[i] = 0.;
//
//      mSliderBounds[i] = new IRECT(mRECT.L + lpos , mRECT.T, mRECT.L + lpos + sliderWidth, mRECT.B);
//    }
//
//    mHandleWidth = handleWidth;
//  }
//
//  ~MultiSliderControlV()
//  {
//    delete [] mSteps;
//
//    for(int i=0; i<mNumSliders; i++)
//    {
//      delete mSliderBounds[i];
//    }
//  }
//
//  void Draw(IGraphics& graphics) override
//  {
//    pGraphics->FillRect(&mBgColor, &mRECT);
//
//    for(int i=0; i<mNumSliders; i++)
//    {
//      float yPos = mSteps[i] * mRECT.H();
//      int top = mRECT.B - yPos;
//      //int bottom = top + 10;
//      int bottom = mRECT.B;
//
//      IColor * color = &mFgColor;
//      if(i == mHighlighted) color = &mHlColor;
//
//      IRECT srect = IRECT(mSliderBounds[i]->L, top, mSliderBounds[i]->R-1, bottom);
//      pGraphics->FillRect(color, &srect );
//    }
//  }
//
//  void OnMouseDown(float x, float y, IMouseMod* pMod) override
//  {
//    SnapToMouse(x, y);
//  }
//
//  void OnMouseUp(float x, float y, IMouseMod* pMod) override
//  {
//    //TODO: check this isn't going to cause problems... this will happen from the gui thread
//    mPlug->ModifyCurrentPreset();
//    mPlug->DirtyPTCompareState();
//  }
//
//  void OnMouseDrag(float x, float y, float dX, float dY, IMouseMod* pMod) override
//  {
//    SnapToMouse(x, y);
//  }
//
//  void SnapToMouse(float x, float y)
//  {
//    x = BOUNDED(x, mRECT.L, mSliderBounds[mNumSliders-1]->R-1);
//    y = BOUNDED(y, mRECT.T, mRECT.B-1);
//
//    float yValue =  (float) (y-mRECT.T) / (float) mRECT.H();
//
//    yValue = round( yValue / mGrain ) * mGrain;
//
//    int sliderTest = mNumSliders-1;
//    bool foundIntersection = false;
//
//    while (!foundIntersection)
//    {
//      foundIntersection = mSliderBounds[sliderTest]->Contains(x, y);
//
//      if (!foundIntersection && sliderTest !=0 ) sliderTest--;
//    }
//
//    if (foundIntersection)
//    {
//      //mHighlighted = sliderTest;
//      mSteps[sliderTest] = 1. - BOUNDED(yValue, 0., 1.);
//      mSliderThatChanged = sliderTest;
//      mPlug->OnParamChange(mParamIdx); // TODO: rethink this WRT threading
//    }
//    else
//    {
//      mSliderThatChanged = -1;
//      //mHighlighted = -1;
//    }
//
//    SetDirty();
//  }
//
//  void GetLatestChange(double* data)
//  {
//    data[mSliderThatChanged] = mSteps[mSliderThatChanged];
//  }
//
//  void GetState(double* data)
//  {
//    memcpy( data, mSteps, mNumSliders * sizeof(double));
//  }
//
//  void SetState(double* data)
//  {
//    memcpy(mSteps, data, mNumSliders * sizeof(double));
//
//    SetDirty();
//  }
//
//  void SetHighlight(int i)
//  {
//    mHighlighted = i;
//
//    SetDirty();
//  }
//
//private:
//  IColor mBgColor, mFgColor, mHlColor;
//  int mNumSliders;
//  int mHandleWidth;
//  int mSliderThatChanged;
//  double *mSteps;
//  double mGrain;
//  IRECT *mSliderBounds[MAXSLIDERS];
//  int mHighlighted;
//};
//
//
//class IVSliderControl: public IControl
//{
//public:
//  IVSliderControl(IPlugBase *pPlug,
//                  IRECT pR,
//                  int paramIdx,
//                  int handleWidth,
//                  const IColor* bgcolor,
//                  const IColor* fgcolor)
//  : IControl(pPlug, pR, paramIdx)
//  {
//    mBgColor = *bgcolor;
//    mFgColor = *fgcolor;
//    mHandleWidth = handleWidth;
//  }
//
//  void Draw(IGraphics& graphics) override
//  {
//    pGraphics->FillRect(&mBgColor, &mRECT);
//
//    float yPos = mValue * mRECT.H();
//    int top = mRECT.B - yPos;
//
//    IRECT innerRect = IRECT(mRECT.L+2, top, mRECT.R-2, mRECT.B);
//    pGraphics->FillRect(&mFgColor, &innerRect);
//
//    return true;
//  }
//
//  void OnMouseDown(float x, float y, IMouseMod* pMod) override
//  {
//    SnapToMouse(x, y);
//  }
//
//  void OnMouseDrag(float x, float y, float dX, float dY, IMouseMod* pMod) override
//  {
//    SnapToMouse(x, y);
//  }
//
//  void SnapToMouse(float x, float y)
//  {
//    x = BOUNDED(x, mRECT.L, mRECT.R-1);
//    y = BOUNDED(y, mRECT.T, mRECT.B-1);
//
//    float yValue = 1. - (float) (y-mRECT.T) / (float) mRECT.H();
//
//    mValue = round( yValue / 0.001 ) * 0.001;
//
//    mPlug->SetParameterFromGUI(mParamIdx, BOUNDED(mValue, 0., 1.));
//
//    SetDirty();
//  }
//
//private:
//  IColor mBgColor, mFgColor;
//  int mHandleWidth;
//};

/**@}*/
