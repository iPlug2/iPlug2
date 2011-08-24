class IKeyCatcher : public IControl
{ 
public:
  IKeyCatcher(IPlugBase* pPlug, IRECT pR)
  : IControl(pPlug, pR) {}
  
  // this never gets called but is needed for an IControl
  bool Draw(IGraphics* pGraphics) { return false; }
  
  bool OnKeyDown(int x, int y, int key)
  {
    switch (key) {
      case KEY_SPACE:
        DBGMSG("Space\n");
        return true;
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
	:	IKnobControl(pPlug, pR, paramIdx), mBitmap(*pBitmap)
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
    
    if (CSTR_NOT_EMPTY(disp)) {
      return pGraphics->DrawIText(&mText, disp, &mTextRECT);
    }
    return true;
  }
  
	void OnMouseDown(int x, int y, IMouseMod* pMod)
	{
    if (mTextRECT.Contains(x, y)) PromptUserInput(&mTextRECT);
    else {
      OnMouseDrag(x, y, 0, 0, pMod);
    }
	}
  
  void OnMouseDblClick(int x, int y, IMouseMod* pMod)
  {
#ifdef RTAS_API
    PromptUserInput(&mTextRECT);
#else
    if (mDefaultValue >= 0.0) {
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