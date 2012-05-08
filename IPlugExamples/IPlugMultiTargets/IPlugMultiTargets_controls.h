class ITestPopupMenu : public IControl
{
private:
  IPopupMenu mMainMenu, mSubMenu;

public:
  ITestPopupMenu(IPlugBase *pPlug, IRECT pR)
    : IControl(pPlug, pR, -1)
  {
    mMainMenu.AddItem("first item");
    mMainMenu.AddItem("second item");
    mMainMenu.AddItem("third item");

    mSubMenu.AddItem("first item");
    mSubMenu.AddItem("second item");
    mSubMenu.AddItem("third item");

    mMainMenu.AddItem("sub menu", &mSubMenu);
  }

  bool Draw(IGraphics* pGraphics)
  {
    return pGraphics->FillIRect(&COLOR_WHITE, &mRECT);;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    doPopupMenu();

    Redraw(); // seems to need this
    SetDirty();
  }

  void doPopupMenu()
  {
    IPopupMenu* selectedMenu = mPlug->GetGUI()->CreateIPopupMenu(&mMainMenu, &mRECT);

    if (selectedMenu == &mMainMenu)
    {
      int itemChosen = selectedMenu->GetChosenItemIdx();
      selectedMenu->CheckItemAlone(itemChosen);
      DBGMSG("item chosen, main menu %i\n", itemChosen);
    }
    else if (selectedMenu == &mSubMenu)
    {
      int itemChosen = selectedMenu->GetChosenItemIdx();
      selectedMenu->CheckItemAlone(itemChosen);
      DBGMSG("item chosen, sub menu %i\n", itemChosen);
    }
    else
    {
      DBGMSG("nothing chosen\n");
    }
  }
};

class ITestPopupMenuB : public IControl
{
private:
  IPopupMenu mMainMenu;

public:
  ITestPopupMenuB(IPlugBase *pPlug, IRECT pR)
    : IControl(pPlug, pR, -1)
  {
    mMainMenu.SetMultiCheck(true);
    mMainMenu.AddItem("first item");
    mMainMenu.AddItem("second item");
    mMainMenu.AddItem("third item");
  }

  bool Draw(IGraphics* pGraphics)
  {
    return pGraphics->FillIRect(&COLOR_WHITE, &mRECT);;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    doPopupMenu();

    Redraw(); // seems to need this
    SetDirty();
  }

  void doPopupMenu()
  {
    IPopupMenu* selectedMenu = mPlug->GetGUI()->CreateIPopupMenu(&mMainMenu, &mRECT);

    if(selectedMenu)
    {
      int idx = selectedMenu->GetChosenItemIdx();
      selectedMenu->CheckItem(idx, !selectedMenu->IsItemChecked(idx));

      WDL_String checkedItems;

      checkedItems.Append("checked: ", 1024);

      for (int i = 0; i < selectedMenu->GetNItems(); i++)
      {
        checkedItems.AppendFormatted(1024, "%i ", selectedMenu->IsItemChecked(i));
      }

      DBGMSG("%s\n", checkedItems.Get());
    }
  }
};

class IPresetMenu : public IControl
{
private:
  WDL_String mDisp;
public:
  IPresetMenu(IPlugBase *pPlug, IRECT pR)
    : IControl(pPlug, pR, -1)
  {
    mTextEntryLength = MAX_PRESET_NAME_LEN - 3;
    mText = IText(14, &COLOR_BLACK, "Arial", IText::kStyleNormal, IText::kAlignNear);
  }

  bool Draw(IGraphics* pGraphics)
  {
    int pNumber = mPlug->GetCurrentPresetIdx();
    mDisp.SetFormatted(32, "%02d: %s", pNumber+1, mPlug->GetPresetName(pNumber));

    pGraphics->FillIRect(&COLOR_WHITE, &mRECT);

    if (CSTR_NOT_EMPTY(mDisp.Get()))
    {
      return pGraphics->DrawIText(&mText, mDisp.Get(), &mRECT);
    }

    return true;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    if (pMod->R)
    {
      const char* pname = mPlug->GetPresetName(mPlug->GetCurrentPresetIdx());
      mPlug->GetGUI()->CreateTextEntry(this, &mText, &mRECT, pname);
    }
    else
    {
      doPopupMenu();
    }

    Redraw(); // seems to need this
    SetDirty();
  }

  void doPopupMenu()
  {
    int numItems = mPlug->NPresets();
    IPopupMenu menu;

    IGraphics* gui = mPlug->GetGUI();

    int currentPresetIdx = mPlug->GetCurrentPresetIdx();

    for(int i = 0; i< numItems; i++)
    {
      const char* str = mPlug->GetPresetName(i);
      if (i == currentPresetIdx)
        menu.AddItem(str, -1, IPopupMenuItem::kChecked);
      else
        menu.AddItem(str);
    }

    menu.SetPrefix(2);

    if(gui->CreateIPopupMenu(&menu, &mRECT))
    {
      int itemChosen = menu.GetChosenItemIdx();

      if (itemChosen > -1)
      {
        mPlug->RestorePreset(itemChosen);
        mPlug->InformHostOfProgramChange();
        mPlug->DirtyParameters();
      }
    }
  }

  void TextFromTextEntry(const char* txt)
  {
    WDL_String safeName;
    safeName.Set(txt, MAX_PRESET_NAME_LEN);

    mPlug->ModifyCurrentPreset(safeName.Get());
    mPlug->InformHostOfProgramChange();
    mPlug->DirtyParameters();
    SetDirty(false);
  }
};

class IPopUpMenuControl : public IControl
{
public:
  IPopUpMenuControl(IPlugBase *pPlug, IRECT pR, int paramIdx)
    : IControl(pPlug, pR, paramIdx)
  {
    mDisablePrompt = false;
    mDblAsSingleClick = true;
    mText = IText(14);
  }

  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->FillIRect(&COLOR_WHITE, &mRECT);

    char disp[32];
    mPlug->GetParam(mParamIdx)->GetDisplayForHost(disp);

    if (CSTR_NOT_EMPTY(disp))
    {
      return pGraphics->DrawIText(&mText, disp, &mRECT);
    }

    return true;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    if (pMod->L)
    {
      PromptUserInput(&mRECT);
    }

    mPlug->GetGUI()->SetAllControlsDirty();
  }

  //void OnMouseWheel(int x, int y, IMouseMod* pMod, int d){} //TODO: popup menus seem to hog the mousewheel

};

// Key catcher is an icontrol but only its OnKeyDown() is called... after all the other controls have been tested to see if they want keyboard input
class IKeyCatcher : public IControl
{
public:
  IKeyCatcher(IPlugBase* pPlug, IRECT pR)
    : IControl(pPlug, pR) {}

  // this never gets called but is needed for an IControl
  bool Draw(IGraphics* pGraphics) { return false; }

  bool OnKeyDown(int x, int y, int key)
  {
    switch (key)
    {
        //case KEY_SPACE:
        ///  DBGMSG("Space\n");
        //  return true;
      case KEY_LEFTARROW:;
        DBGMSG("Left\n");
        return true;
      case KEY_RIGHTARROW:
        DBGMSG("Right\n");
        return true;
      case KEY_UPARROW:;
        DBGMSG("Up\n");
        return true;
      case KEY_DOWNARROW:
        DBGMSG("Down\n");
        return true;
      default:
        return false;
    }
  }
};

class ITempoDisplay : public IControl
{
private:
  ITimeInfo* mTimeInfo;
  WDL_String mDisplay;

public:
  ITempoDisplay(IPlugBase* pPlug, IRECT pR, IText* pText, ITimeInfo* pTimeInfo)
    : IControl(pPlug, pR)
  {
    mText = *pText;
    mTimeInfo = pTimeInfo;
  }

  bool Draw(IGraphics* pGraphics)
  {
    mDisplay.SetFormatted(80, "Tempo: %f, SamplePos: %i, PPQPos: %f", mTimeInfo->mTempo, (int) mTimeInfo->mSamplePos, mTimeInfo->mPPQPos);
    return pGraphics->DrawIText(&mText, mDisplay.Get(), &mRECT);
  }

  bool IsDirty() { return true;}
};

class IKnobMultiControlText : public IKnobControl
{
private:
  IRECT mTextRECT, mImgRECT;
  IBitmap mBitmap;

public:
  IKnobMultiControlText(IPlugBase* pPlug, IRECT pR, int paramIdx, IBitmap* pBitmap, IText* pText)
    : IKnobControl(pPlug, pR, paramIdx), mBitmap(*pBitmap)
  {
    mText = *pText;
    mTextRECT = IRECT(mRECT.L, mRECT.B-20, mRECT.R, mRECT.B);
    mImgRECT = IRECT(mRECT.L, mRECT.T, &mBitmap);
    mDisablePrompt = false;
  }

  ~IKnobMultiControlText() {}

  bool Draw(IGraphics* pGraphics)
  {
    int i = 1 + int(0.5 + mValue * (double) (mBitmap.N - 1));
    i = BOUNDED(i, 1, mBitmap.N);
    pGraphics->DrawBitmap(&mBitmap, &mImgRECT, i, &mBlend);
    //pGraphics->FillIRect(&COLOR_WHITE, &mTextRECT);

    char disp[20];
    mPlug->GetParam(mParamIdx)->GetDisplayForHost(disp);

    if (CSTR_NOT_EMPTY(disp))
    {
      return pGraphics->DrawIText(&mText, disp, &mTextRECT);
    }
    return true;
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    if (mTextRECT.Contains(x, y)) PromptUserInput(&mTextRECT);
#ifdef RTAS_API
    else if (pMod->A)
    {
      if (mDefaultValue >= 0.0)
      {
        mValue = mDefaultValue;
        SetDirty();
      }
    }
#endif
    else
    {
      OnMouseDrag(x, y, 0, 0, pMod);
    }
  }

  void OnMouseDblClick(int x, int y, IMouseMod* pMod)
  {
#ifdef RTAS_API
    PromptUserInput(&mTextRECT);
#else
    if (mDefaultValue >= 0.0)
    {
      mValue = mDefaultValue;
      SetDirty();
    }
#endif
  }

};

class IPeakMeterVert : public IControl
{
public:

  IPeakMeterVert(IPlugBase* pPlug, IRECT pR)
    : IControl(pPlug, pR)
  {
    mColor = COLOR_BLUE;
  }

  ~IPeakMeterVert() {}

  bool Draw(IGraphics* pGraphics)
  {
    //IRECT(mRECT.L, mRECT.T, mRECT.W , mRECT.T + (mValue * mRECT.H));
    pGraphics->FillIRect(&COLOR_RED, &mRECT);

    //pGraphics->FillIRect(&COLOR_BLUE, &mRECT);

    IRECT filledBit = IRECT(mRECT.L, mRECT.T, mRECT.R , mRECT.B - (mValue * mRECT.H()));
    pGraphics->FillIRect(&mColor, &filledBit);
    return true;
  }

  bool IsDirty() { return true;}

protected:
  IColor mColor;
};

class IPeakMeterHoriz : public IPeakMeterVert
{
public:

  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->FillIRect(&COLOR_BLUE, &mRECT);
    IRECT filledBit = IRECT(mRECT.L, mRECT.T, mRECT.L + (mValue * mRECT.W() ) , mRECT.B );
    pGraphics->FillIRect(&mColor, &filledBit);
    return true;
  }
};