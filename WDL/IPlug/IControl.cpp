#include "IControl.h"
#include "math.h"
#include "Log.h"

const float GRAYED_ALPHA = 0.25f;

void IControl::SetValueFromPlug(double value)
{
  if (mDefaultValue < 0.0)
  {
    mDefaultValue = mValue = value;
  }

  if (mValue != value)
  {
    mValue = value;
    SetDirty(false);
    Redraw();
  }
}

void IControl::SetValueFromUserInput(double value)
{
  if (mValue != value)
  {
    mValue = value;
    SetDirty();
    Redraw();
  }
}

void IControl::SetDirty(bool pushParamToPlug)
{
  mValue = BOUNDED(mValue, mClampLo, mClampHi);
  mDirty = true;
  if (pushParamToPlug && mPlug && mParamIdx >= 0)
  {
    mPlug->SetParameterFromGUI(mParamIdx, mValue);
    IParam* pParam = mPlug->GetParam(mParamIdx);
    
    if (mValDisplayControl) 
    {
      WDL_String plusLabel;
      char str[32];
      pParam->GetDisplayForHost(str);
      plusLabel.Set(str, 32);
      plusLabel.Append(" ", 32);
      plusLabel.Append(pParam->GetLabelForHost(), 32);
      
      ((ITextControl*)mValDisplayControl)->SetTextFromPlug(plusLabel.Get());
    }
    
    if (mNameDisplayControl) 
    {
      ((ITextControl*)mNameDisplayControl)->SetTextFromPlug((char*) pParam->GetNameForHost());
    }
  }
}

void IControl::SetClean()
{
  mDirty = mRedraw;
  mRedraw = false;
}

void IControl::Hide(bool hide)
{
  mHide = hide;
  mRedraw = true;
  SetDirty(false);
}

void IControl::GrayOut(bool gray)
{
  mGrayed = gray;
  mBlend.mWeight = (gray ? GRAYED_ALPHA : 1.0f);
  SetDirty(false);
}

void IControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
  #ifdef PROTOOLS
  if (pMod->A && mDefaultValue >= 0.0)
  {
    mValue = mDefaultValue;
    SetDirty();
  }
  #endif
  
  if (pMod->R) {
		PromptUserInput();
	}
}

void IControl::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
  #ifdef PROTOOLS
  PromptUserInput();
  #else
  if (mDefaultValue >= 0.0)
  {
    mValue = mDefaultValue;
    SetDirty();
  }
  #endif
}

void IControl::OnMouseWheel(int x, int y, IMouseMod* pMod, int d)
{
  #ifdef PROTOOLS
  if (pMod->C)
  {
    mValue += 0.001 * d;
  }
  #else
  if (pMod->C || pMod->S)
  {
    mValue += 0.001 * d;
  }
  #endif
  else
  {
    mValue += 0.01 * d;
  }
  
  SetDirty();
}

#define PARAM_EDIT_W 30
#define PARAM_EDIT_H 16

void IControl::PromptUserInput()
{
  if (mParamIdx >= 0 && !mDisablePrompt)
  {
    if (mPlug->GetParam(mParamIdx)->GetNDisplayTexts()) // popup menu
    {
      mPlug->GetGUI()->PromptUserInput(this, mPlug->GetParam(mParamIdx), &mRECT );
    }
    else // text entry
    {
      int cX = (int) mRECT.MW();
      int cY = (int) mRECT.MH();
      int halfW = int(float(PARAM_EDIT_W)/2.f);
      int halfH = int(float(PARAM_EDIT_H)/2.f);

      IRECT txtRECT = IRECT(cX - halfW, cY - halfH, cX + halfW,cY + halfH);
      mPlug->GetGUI()->PromptUserInput(this, mPlug->GetParam(mParamIdx), &txtRECT );
    }

    Redraw();
  }
}

void IControl::PromptUserInput(IRECT* pTextRect)
{
  if (mParamIdx >= 0 && !mDisablePrompt)
  {
    mPlug->GetGUI()->PromptUserInput(this, mPlug->GetParam(mParamIdx), pTextRect);
    Redraw();
  }
}

IControl::AuxParam* IControl::GetAuxParam(int idx)
{
  assert(idx > -1 && idx < mAuxParams.GetSize());
  return mAuxParams.Get() + idx;
}

int IControl::AuxParamIdx(int paramIdx)
{
  for (int i=0;i<mAuxParams.GetSize();i++)
  {
    if(GetAuxParam(i)->mParamIdx == paramIdx)
      return i;
  }
  
  return -1;
}

void IControl::AddAuxParam(int paramIdx)
{
  mAuxParams.Add(AuxParam(paramIdx));
}

void IControl::SetAuxParamValueFromPlug(int auxParamIdx, double value)
{
  AuxParam* auxParam = GetAuxParam(auxParamIdx);
  
  if (auxParam->mValue != value)
  {
    auxParam->mValue = value;
    SetDirty(false);
    Redraw();
  }
}

void IControl::SetAllAuxParamsFromGUI()
{
  for (int i=0;i<mAuxParams.GetSize();i++)
  {
    AuxParam* auxParam = GetAuxParam(i);
    mPlug->SetParameterFromGUI(auxParam->mParamIdx, auxParam->mValue);
  }
}

bool IPanelControl::Draw(IGraphics* pGraphics)
{
  pGraphics->FillIRect(&mColor, &mRECT, &mBlend);
  return true;
}

bool IBitmapControl::Draw(IGraphics* pGraphics)
{
  int i = 1;
  if (mBitmap.N > 1)
  {
    i = 1 + int(0.5 + mValue * (double) (mBitmap.N - 1));
    i = BOUNDED(i, 1, mBitmap.N);
  }
  return pGraphics->DrawBitmap(&mBitmap, &mRECT, i, &mBlend);
}

void ISwitchControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
  if (mBitmap.N > 1)
  {
    mValue += 1.0 / (double) (mBitmap.N - 1);
  }
  else
  {
    mValue += 1.0;
  }

  if (mValue > 1.001)
  {
    mValue = 0.0;
  }
  SetDirty();
}

void ISwitchControl::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
  OnMouseDown(x, y, pMod);
}

void ISwitchPopUpControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
  PromptUserInput();

  SetDirty();
}

ISwitchFramesControl::ISwitchFramesControl(IPlugBase* pPlug, int x, int y, int paramIdx, IBitmap* pBitmap, bool imagesAreHorizontal, IChannelBlend::EBlendMethod blendMethod)
  : ISwitchControl(pPlug, x, y, paramIdx, pBitmap, blendMethod)
{
  mDisablePrompt = false;
  
  for(int i = 0; i < pBitmap->N; i++)
  {
    if (imagesAreHorizontal)
      mRECTs.Add(mRECT.SubRectHorizontal(pBitmap->N, i)); 
    else
      mRECTs.Add(mRECT.SubRectVertical(pBitmap->N, i)); 
  }
}

void ISwitchFramesControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
  int n = mRECTs.GetSize();
  
  for (int i = 0; i < n; i++) 
  {
    if (mRECTs.Get()[i].Contains(x, y)) 
    {
      mValue = (double) i / (double) (n - 1);
      break;
    }
  }
  
  SetDirty();
}

IInvisibleSwitchControl::IInvisibleSwitchControl(IPlugBase* pPlug, IRECT pR, int paramIdx)
  :   IControl(pPlug, pR, paramIdx, IChannelBlend::kBlendClobber)
{
  mDisablePrompt = true;
}

void IInvisibleSwitchControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
  if (mValue < 0.5)
  {
    mValue = 1.0;
  }
  else
  {
    mValue = 0.0;
  }
  SetDirty();
}

IRadioButtonsControl::IRadioButtonsControl(IPlugBase* pPlug, IRECT pR, int paramIdx, int nButtons,
    IBitmap* pBitmap, EDirection direction, bool reverse)
  :   IControl(pPlug, pR, paramIdx), mBitmap(*pBitmap)
{
  mRECTs.Resize(nButtons);
  int h = int((double) pBitmap->H / (double) pBitmap->N);
  
  if (reverse) 
  {
    if (direction == kHorizontal)
    {
      int dX = int((double) (pR.W() - nButtons * pBitmap->W) / (double) (nButtons - 1));
      int x = mRECT.R - pBitmap->W - dX;
      int y = mRECT.T;
      
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + pBitmap->W, y + h);
        x -= pBitmap->W + dX;
      }
    }
    else
    {
      int dY = int((double) (pR.H() - nButtons * h) /  (double) (nButtons - 1));
      int x = mRECT.L;
      int y = mRECT.B - h - dY;
      
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + pBitmap->W, y + h);
        y -= h + dY;
      }
    }
    
  }
  else
  {
    int x = mRECT.L, y = mRECT.T;
    
    if (direction == kHorizontal)
    {
      int dX = int((double) (pR.W() - nButtons * pBitmap->W) / (double) (nButtons - 1));
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + pBitmap->W, y + h);
        x += pBitmap->W + dX;
      }
    }
    else
    {
      int dY = int((double) (pR.H() - nButtons * h) /  (double) (nButtons - 1));
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + pBitmap->W, y + h);
        y += h + dY;
      }
    }
  }
}

void IRadioButtonsControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
  #ifdef PROTOOLS
  if (pMod->A) 
  {
    if (mDefaultValue >= 0.0)
    {
      mValue = mDefaultValue;
      SetDirty();
      return;
    }
  }
  else
  #endif
  if (pMod->R)
  {
    PromptUserInput();
    return;
  }

  int i, n = mRECTs.GetSize();

  for (i = 0; i < n; ++i)
  {
    if (mRECTs.Get()[i].Contains(x, y))
    {
      mValue = (double) i / (double) (n - 1);
      break;
    }
  }

  SetDirty();
}

bool IRadioButtonsControl::Draw(IGraphics* pGraphics)
{
  int i, n = mRECTs.GetSize();
  int active = int(0.5 + mValue * (double) (n - 1));
  active = BOUNDED(active, 0, n - 1);
  for (i = 0; i < n; ++i)
  {
    if (i == active)
    {
      pGraphics->DrawBitmap(&mBitmap, &mRECTs.Get()[i], 2, &mBlend);
    }
    else
    {
      pGraphics->DrawBitmap(&mBitmap, &mRECTs.Get()[i], 1, &mBlend);
    }
  }
  return true;
}

void IContactControl::OnMouseUp(int x, int y, IMouseMod* pMod)
{
  mValue = 0.0;
  SetDirty();
}

IFaderControl::IFaderControl(IPlugBase* pPlug, int x, int y, int len, int paramIdx, IBitmap* pBitmap, EDirection direction, bool onlyHandle)
  : IControl(pPlug, IRECT(), paramIdx), mLen(len), mBitmap(*pBitmap), mDirection(direction), mOnlyHandle(onlyHandle)
{
  if (direction == kVertical)
  {
    mHandleHeadroom = mBitmap.H;
    mRECT = mTargetRECT = IRECT(x, y, x + mBitmap.W, y + len);
  }
  else
  {
    mHandleHeadroom = mBitmap.W;
    mRECT = mTargetRECT = IRECT(x, y, x + len, y + mBitmap.H);
  }
}

IRECT IFaderControl::GetHandleRECT(double value) const
{
  if (value < 0.0)
  {
    value = mValue;
  }
  IRECT r(mRECT.L, mRECT.T, mRECT.L + mBitmap.W, mRECT.T + mBitmap.H);
  if (mDirection == kVertical)
  {
    int offs = int((1.0 - value) * (double) (mLen - mHandleHeadroom));
    r.T += offs;
    r.B += offs;
  }
  else
  {
    int offs = int(value * (double) (mLen - mHandleHeadroom));
    r.L += offs;
    r.R += offs;
  }
  return r;
}

void IFaderControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
  #ifdef PROTOOLS
  if (pMod->A) 
  {
    if (mDefaultValue >= 0.0)
    {
      mValue = mDefaultValue;
      SetDirty();
      return;
    }
  }
  else
  #endif
  if (pMod->R)
  {
    PromptUserInput();
    return;
  }

  return SnapToMouse(x, y);
}

void IFaderControl::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
  return SnapToMouse(x, y);
}

void IFaderControl::SnapToMouse(int x, int y)
{
  if (mDirection == kVertical)
  {
    mValue = 1.0 - (double) (y - mRECT.T - mHandleHeadroom / 2) / (double) (mLen - mHandleHeadroom);
  }
  else
  {
    mValue = (double) (x - mRECT.L - mHandleHeadroom / 2) / (double) (mLen - mHandleHeadroom);
  }
  SetDirty();
}

bool IFaderControl::Draw(IGraphics* pGraphics)
{
  IRECT r = GetHandleRECT();
  return pGraphics->DrawBitmap(&mBitmap, &r, 1, &mBlend);
}

bool IFaderControl::IsHit(int x, int y) 
{
  if(mOnlyHandle)
  {
    IRECT r = GetHandleRECT();
    return r.Contains(x, y); 
  }
  else 
  {
    return mTargetRECT.Contains(x, y); 
  }
}

void IKnobControl::OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
{
  double gearing = mGearing;
  
  #ifdef PROTOOLS
    #ifdef OS_WIN
      if (pMod->C) gearing *= 10.0;
    #else
      if (pMod->R) gearing *= 10.0;
    #endif
  #else
    if (pMod->C || pMod->S) gearing *= 10.0;
  #endif
  
  if (mDirection == kVertical)
  {
    mValue += (double) dY / (double) (mRECT.T - mRECT.B) / gearing;
  }
  else
  {
    mValue += (double) dX / (double) (mRECT.R - mRECT.L) / gearing;
  }

  SetDirty();
}

IKnobLineControl::IKnobLineControl(IPlugBase* pPlug, IRECT pR, int paramIdx,
                                   const IColor* pColor, double innerRadius, double outerRadius,
                                   double minAngle, double maxAngle, EDirection direction, double gearing)
  :   IKnobControl(pPlug, pR, paramIdx, direction, gearing),
      mColor(*pColor)
{
  mMinAngle = (float) minAngle;
  mMaxAngle = (float) maxAngle;
  mInnerRadius = (float) innerRadius;
  mOuterRadius = (float) outerRadius;
  if (mOuterRadius == 0.0f)
  {
    mOuterRadius = 0.5f * (float) pR.W();
  }
  mBlend = IChannelBlend(IChannelBlend::kBlendClobber);
}

bool IKnobLineControl::Draw(IGraphics* pGraphics)
{
  double v = mMinAngle + mValue * (mMaxAngle - mMinAngle);
  float sinV = (float) sin(v);
  float cosV = (float) cos(v);
  float cx = mRECT.MW(), cy = mRECT.MH();
  float x1 = cx + mInnerRadius * sinV, y1 = cy - mInnerRadius * cosV;
  float x2 = cx + mOuterRadius * sinV, y2 = cy - mOuterRadius * cosV;
  return pGraphics->DrawLine(&mColor, x1, y1, x2, y2, &mBlend, true);
}

bool IKnobRotaterControl::Draw(IGraphics* pGraphics)
{
  int cX = (mRECT.L + mRECT.R) / 2;
  int cY = (mRECT.T + mRECT.B) / 2;
  double angle = mMinAngle + mValue * (mMaxAngle - mMinAngle);
  return pGraphics->DrawRotatedBitmap(&mBitmap, cX, cY, angle, mYOffset, &mBlend);
}

// Same as IBitmapControl::Draw.
bool IKnobMultiControl::Draw(IGraphics* pGraphics)
{
  int i = 1 + int(0.5 + mValue * (double) (mBitmap.N - 1));
  i = BOUNDED(i, 1, mBitmap.N);
  return pGraphics->DrawBitmap(&mBitmap, &mRECT, i, &mBlend);
}

bool IKnobRotatingMaskControl::Draw(IGraphics* pGraphics)
{
  double angle = mMinAngle + mValue * (mMaxAngle - mMinAngle);
  return pGraphics->DrawRotatedMask(&mBase, &mMask, &mTop, mRECT.L, mRECT.T, angle, &mBlend);
}

bool IBitmapOverlayControl::Draw(IGraphics* pGraphics)
{
  if (mValue < 0.5)
  {
    mTargetRECT = mTargetArea;
    return true;  // Don't draw anything.
  }
  else
  {
    mTargetRECT = mRECT;
    return IBitmapControl::Draw(pGraphics);
  }
}

void ITextControl::SetTextFromPlug(char* str)
{
  if (strcmp(mStr.Get(), str))
  {
    SetDirty(false);
    mStr.Set(str);
  }
}

bool ITextControl::Draw(IGraphics* pGraphics)
{
  char* cStr = mStr.Get();
  if (CSTR_NOT_EMPTY(cStr))
  {
    return pGraphics->DrawIText(&mText, cStr, &mRECT);
  }
  return true;
}

ICaptionControl::ICaptionControl(IPlugBase* pPlug, IRECT pR, int paramIdx, IText* pText, bool showParamLabel)
  :   ITextControl(pPlug, pR, pText), mShowParamLabel(showParamLabel)
{
  mParamIdx = paramIdx;
}

void ICaptionControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
  if (pMod->L || pMod->R)
  {
    PromptUserInput();
  }
}

void ICaptionControl::OnMouseDblClick(int x, int y, IMouseMod* pMod)
{
  PromptUserInput();
}

bool ICaptionControl::Draw(IGraphics* pGraphics)
{
  IParam* pParam = mPlug->GetParam(mParamIdx);
  char cStr[32];
  pParam->GetDisplayForHost(cStr);
  mStr.Set(cStr);

  if (mShowParamLabel)
  {
    mStr.Append(" ");
    mStr.Append(pParam->GetLabelForHost());
  }

  return ITextControl::Draw(pGraphics);
}

IURLControl::IURLControl(IPlugBase* pPlug, IRECT pR, const char* url, const char* backupURL, const char* errMsgOnFailure)
  : IControl(pPlug, pR)
{
  memset(mURL, 0, MAX_URL_LEN);
  memset(mBackupURL, 0, MAX_URL_LEN);
  memset(mErrMsg, 0, MAX_NET_ERR_MSG_LEN);

  if (CSTR_NOT_EMPTY(url))
  {
    strcpy(mURL, url);
  }
  if (CSTR_NOT_EMPTY(backupURL))
  {
    strcpy(mBackupURL, backupURL);
  }
  if (CSTR_NOT_EMPTY(errMsgOnFailure))
  {
    strcpy(mErrMsg, errMsgOnFailure);
  }
}

void IURLControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
  bool opened = false;

  if (CSTR_NOT_EMPTY(mURL))
  {
    opened = mPlug->GetGUI()->OpenURL(mURL, mErrMsg);
  }

  if (!opened && CSTR_NOT_EMPTY(mBackupURL))
  {
    opened = mPlug->GetGUI()->OpenURL(mBackupURL, mErrMsg);
  }
}

void IFileSelectorControl::OnMouseDown(int x, int y, IMouseMod* pMod)
{
  if (mPlug && mPlug->GetGUI())
  {
    mState = kFSSelecting;
    SetDirty(false);

    mPlug->GetGUI()->PromptForFile(&mFile, mFileAction, &mDir, mExtensions.Get());
    mValue += 1.0;
    if (mValue > 1.0)
    {
      mValue = 0.0;
    }
    mState = kFSDone;
    SetDirty();
  }
}

bool IFileSelectorControl::Draw(IGraphics* pGraphics)
{
  if (mState == kFSSelecting)
  {
    pGraphics->DrawBitmap(&mBitmap, &mRECT, 0, 0);
  }
  return true;
}

void IFileSelectorControl::GetLastSelectedFileForPlug(WDL_String* pStr)
{
  pStr->Set(mFile.Get());
}

void IFileSelectorControl::SetLastSelectedFileFromPlug(char* file)
{
  mFile.Set(file);
}

bool IFileSelectorControl::IsDirty()
{
  if (mDirty)
  {
    return true;
  }

  if (mState == kFSDone)
  {
    mState = kFSNone;
    return true;
  }
  return false;
}
