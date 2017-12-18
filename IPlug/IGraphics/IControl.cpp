#include "IControl.h"
#include <cmath>
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
  if (pushParamToPlug && mParamIdx >= 0)
  {
    mPlug.SetParameterFromGUI(mParamIdx, mValue);
    IParam* pParam = mPlug.GetParam(mParamIdx);
    
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

void IControl::OnMouseDown(int x, int y, const IMouseMod& mod)
{
  #ifdef PROTOOLS
  if (mod.A && mDefaultValue >= 0.0)
  {
    mValue = mDefaultValue;
    SetDirty();
  }
  #endif
  
  if (mod.R) {
		PromptUserInput();
	}
}

void IControl::OnMouseDblClick(int x, int y, const IMouseMod& mod)
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

void IControl::PromptUserInput()
{
  if (mParamIdx >= 0 && !mDisablePrompt)
  {
    if (mPlug.GetParam(mParamIdx)->GetNDisplayTexts()) // popup menu
    {
      mPlug.GetGUI()->PromptUserInput(this, mPlug.GetParam(mParamIdx), mRECT );
    }
    else // text entry
    {
      int cX = (int) mRECT.MW();
      int cY = (int) mRECT.MH();
      int halfW = int(float(PARAM_EDIT_W)/2.f);
      int halfH = int(float(PARAM_EDIT_H)/2.f);

      IRECT txtRECT = IRECT(cX - halfW, cY - halfH, cX + halfW,cY + halfH);
      mPlug.GetGUI()->PromptUserInput(this, mPlug.GetParam(mParamIdx), txtRECT );
    }

    Redraw();
  }
}

void IControl::PromptUserInput(IRECT& textRect)
{
  if (mParamIdx >= 0 && !mDisablePrompt)
  {
    mPlug.GetGUI()->PromptUserInput(this, mPlug.GetParam(mParamIdx), textRect);
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
    mPlug.SetParameterFromGUI(auxParam->mParamIdx, auxParam->mValue);
  }
}

void IPanelControl::Draw(IGraphics& graphics)
{
  graphics.FillIRect(mColor, mRECT, &mBlend);
}

void IBitmapControl::Draw(IGraphics& graphics)
{
  int i = 1;
  if (mBitmap.N > 1)
  {
    i = 1 + int(0.5 + mValue * (double) (mBitmap.N - 1));
    i = BOUNDED(i, 1, mBitmap.N);
  }
  
  graphics.DrawBitmap(mBitmap, mRECT, i, &mBlend);
}

void IBitmapControl::OnRescale()
{
  mBitmap = GetGUI()->GetScaledBitmap(mBitmap);
}

void ISwitchControl::OnMouseDown(int x, int y, const IMouseMod& mod)
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

void ISwitchControl::OnMouseDblClick(int x, int y, const IMouseMod& mod)
{
  OnMouseDown(x, y, mod);
}

void ISwitchPopUpControl::OnMouseDown(int x, int y, const IMouseMod& mod)
{
  PromptUserInput();

  SetDirty();
}

ISwitchFramesControl::ISwitchFramesControl(IPlugBaseGraphics& plug, int x, int y, int paramIdx, IBitmap& bitmap, bool imagesAreHorizontal, IChannelBlend::EBlendMethod blendMethod)
  : ISwitchControl(plug, x, y, paramIdx, bitmap, blendMethod)
{
  mDisablePrompt = false;
  
  for(int i = 0; i < bitmap.N; i++)
  {
    if (imagesAreHorizontal)
      mRECTs.Add(mRECT.SubRectHorizontal(bitmap.N, i));
    else
      mRECTs.Add(mRECT.SubRectVertical(bitmap.N, i));
  }
}

void ISwitchFramesControl::OnMouseDown(int x, int y, const IMouseMod& mod)
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

IInvisibleSwitchControl::IInvisibleSwitchControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx)
  :   IControl(plug, rect, paramIdx, IChannelBlend::kBlendClobber)
{
  mDisablePrompt = true;
}

void IInvisibleSwitchControl::OnMouseDown(int x, int y, const IMouseMod& mod)
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

IRadioButtonsControl::IRadioButtonsControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, int nButtons,
    IBitmap& bitmap, EDirection direction, bool reverse)
  :   IControl(plug, rect, paramIdx), mBitmap(bitmap)
{
  mRECTs.Resize(nButtons);
  int h = int((double) bitmap.H / (double) bitmap.N);
  
  if (reverse) 
  {
    if (direction == kHorizontal)
    {
      int dX = int((double) (rect.W() - nButtons * bitmap.W) / (double) (nButtons - 1));
      int x = mRECT.R - bitmap.W - dX;
      int y = mRECT.T;
      
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + bitmap.W, y + h);
        x -= bitmap.W + dX;
      }
    }
    else
    {
      int dY = int((double) (rect.H() - nButtons * h) /  (double) (nButtons - 1));
      int x = mRECT.L;
      int y = mRECT.B - h - dY;
      
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + bitmap.W, y + h);
        y -= h + dY;
      }
    }
    
  }
  else
  {
    int x = mRECT.L, y = mRECT.T;
    
    if (direction == kHorizontal)
    {
      int dX = int((double) (rect.W() - nButtons * bitmap.W) / (double) (nButtons - 1));
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + bitmap.W, y + h);
        x += bitmap.W + dX;
      }
    }
    else
    {
      int dY = int((double) (rect.H() - nButtons * h) /  (double) (nButtons - 1));
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + bitmap.W, y + h);
        y += h + dY;
      }
    }
  }
}

void IRadioButtonsControl::OnMouseDown(int x, int y, const IMouseMod& mod)
{
  #ifdef PROTOOLS
  if (mod.A)
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
  if (mod.R)
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

void IRadioButtonsControl::Draw(IGraphics& graphics)
{
  int i, n = mRECTs.GetSize();
  int active = int(0.5 + mValue * (double) (n - 1));
  active = BOUNDED(active, 0, n - 1);
  for (i = 0; i < n; ++i)
  {
    if (i == active)
    {
      graphics.DrawBitmap(mBitmap, mRECTs.Get()[i], 2, &mBlend);
    }
    else
    {
      graphics.DrawBitmap(mBitmap, mRECTs.Get()[i], 1, &mBlend);
    }
  }
}

void IContactControl::OnMouseUp(int x, int y, const IMouseMod& mod)
{
  mValue = 0.0;
  SetDirty();
}

IFaderControl::IFaderControl(IPlugBaseGraphics& plug, int x, int y, int len, int paramIdx, IBitmap& bitmap, EDirection direction, bool onlyHandle)
: IControl(plug, IRECT(), paramIdx)
, mLen(len), mBitmap(bitmap), mDirection(direction), mOnlyHandle(onlyHandle)
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

void IFaderControl::OnMouseDown(int x, int y, const IMouseMod& mod)
{
  #ifdef PROTOOLS
  if (mod.A)
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
  if (mod.R)
  {
    PromptUserInput();
    return;
  }

  return SnapToMouse(x, y);
}

void IFaderControl::OnMouseDrag(int x, int y, int dX, int dY, const IMouseMod& mod)
{
  return SnapToMouse(x, y);
}

void IFaderControl::OnMouseWheel(int x, int y, const IMouseMod& mod, int d)
{
#ifdef PROTOOLS
  if (mod.C)
  {
    mValue += 0.001 * d;
  }
#else
  if (mod.C || mod.S)
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

void IFaderControl::Draw(IGraphics& graphics)
{
  IRECT r = GetHandleRECT();
  graphics.DrawBitmap(mBitmap, r, 1, &mBlend);
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

void IFaderControl::OnRescale()
{
  mBitmap = GetGUI()->GetScaledBitmap(mBitmap);
}

void IKnobControl::OnMouseDrag(int x, int y, int dX, int dY, const IMouseMod& mod)
{
  double gearing = mGearing;
  
  #ifdef PROTOOLS
    #ifdef OS_WIN
      if (mod.C) gearing *= 10.0;
    #else
      if (mod.R) gearing *= 10.0;
    #endif
  #else
    if (mod.C || mod.S) gearing *= 10.0;
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

void IKnobControl::OnMouseWheel(int x, int y, const IMouseMod& mod, int d)
{
#ifdef PROTOOLS
  if (mod.C)
  {
    mValue += 0.001 * d;
  }
#else
  if (mod.C || mod.S)
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

IKnobLineControl::IKnobLineControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, const IColor& color, double innerRadius, double outerRadius, double minAngle, double maxAngle, EDirection direction, double gearing)
  : IKnobControl(plug, rect, paramIdx, direction, gearing)
  , mColor(color)
{
  mMinAngle = (float) minAngle;
  mMaxAngle = (float) maxAngle;
  mInnerRadius = (float) innerRadius;
  mOuterRadius = (float) outerRadius;
  if (mOuterRadius == 0.0f)
  {
    mOuterRadius = 0.5f * (float) rect.W();
  }
  mBlend = IChannelBlend(IChannelBlend::kBlendClobber);
}

void IKnobLineControl::Draw(IGraphics& graphics)
{
  const float v = mMinAngle + mValue * (mMaxAngle - mMinAngle);
  const float sinV = sinf(v);
  const float cosV = cosf(v);
  const float cx = mRECT.MW(), cy = mRECT.MH();
  const float x1 = cx + mInnerRadius * sinV, y1 = cy - mInnerRadius * cosV;
  const float x2 = cx + mOuterRadius * sinV, y2 = cy - mOuterRadius * cosV;

  graphics.DrawCircle(mColor, cx, cy, (mRECT.W()/2.) - 2.);
  graphics.DrawLine(mColor, x1, y1, x2, y2, &mBlend, true);
}

void IKnobRotaterControl::Draw(IGraphics& graphics)
{
  int cX = (mRECT.L + mRECT.R) / 2;
  int cY = (mRECT.T + mRECT.B) / 2;
  double angle = mMinAngle + mValue * (mMaxAngle - mMinAngle);
  graphics.DrawRotatedBitmap(mBitmap, cX, cY, angle, mYOffset, &mBlend);
}

// Same as IBitmapControl::Draw.
void IKnobMultiControl::Draw(IGraphics& graphics)
{
  int i = 1 + int(0.5 + mValue * (double) (mBitmap.N - 1));
  i = BOUNDED(i, 1, mBitmap.N);
  graphics.DrawBitmap(mBitmap, mRECT, i, &mBlend);
}

void IKnobMultiControl::OnRescale()
{
  mBitmap = GetGUI()->GetScaledBitmap(mBitmap);
}

void IKnobRotatingMaskControl::Draw(IGraphics& graphics)
{
  double angle = mMinAngle + mValue * (mMaxAngle - mMinAngle);
  graphics.DrawRotatedMask(mBase, mMask, mTop, mRECT.L, mRECT.T, angle, &mBlend);
}

void IBitmapOverlayControl::Draw(IGraphics& graphics)
{
  if (mValue < 0.5)
  {
    mTargetRECT = mTargetArea;
    return;  // Don't draw anything.
  }
  else
  {
    mTargetRECT = mRECT;
    IBitmapControl::Draw(graphics);
  }
}

void ITextControl::SetTextFromPlug(const char* str)
{
  if (strcmp(mStr.Get(), str))
  {
    SetDirty(false);
    mStr.Set(str);
  }
}

void ITextControl::Draw(IGraphics& graphics)
{
  char* cStr = mStr.Get();
  if (CSTR_NOT_EMPTY(cStr))
  {
    graphics.DrawIText(mText, cStr, mRECT);
  }
}

ICaptionControl::ICaptionControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, IText& text, bool showParamLabel)
: ITextControl(plug, rect, text)
, mShowParamLabel(showParamLabel)
{
  mParamIdx = paramIdx;
}

void ICaptionControl::OnMouseDown(int x, int y, const IMouseMod& mod)
{
  if (mod.L || mod.R)
  {
    PromptUserInput();
  }
}

void ICaptionControl::OnMouseDblClick(int x, int y, const IMouseMod& mod)
{
  PromptUserInput();
}

void ICaptionControl::Draw(IGraphics& graphics)
{
  IParam* pParam = mPlug.GetParam(mParamIdx);
  char cStr[32];
  pParam->GetDisplayForHost(cStr);
  mStr.Set(cStr);

  if (mShowParamLabel)
  {
    mStr.Append(" ");
    mStr.Append(pParam->GetLabelForHost());
  }

  ITextControl::Draw(graphics);
}

IURLControl::IURLControl(IPlugBaseGraphics& plug, IRECT rect, const char* URL, const char* backupURL, const char* errMsgOnFailure)
  : IControl(plug, rect)
{
  memset(mURL, 0, MAX_URL_LEN);
  memset(mBackupURL, 0, MAX_URL_LEN);
  memset(mErrMsg, 0, MAX_NET_ERR_MSG_LEN);

  if (CSTR_NOT_EMPTY(URL))
  {
    strcpy(mURL, URL);
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

void IURLControl::OnMouseDown(int x, int y, const IMouseMod& mod)
{
  bool opened = false;

  if (CSTR_NOT_EMPTY(mURL))
  {
    opened = mPlug.GetGUI()->OpenURL(mURL, mErrMsg);
  }

  if (!opened && CSTR_NOT_EMPTY(mBackupURL))
  {
    opened = mPlug.GetGUI()->OpenURL(mBackupURL, mErrMsg);
  }
}

void IFileSelectorControl::OnMouseDown(int x, int y, const IMouseMod& mod)
{
  if (mPlug.GetGUI())
  {
    mState = kFSSelecting;
    SetDirty(false);

    mPlug.GetGUI()->PromptForFile(mFile, mFileAction, &mDir, mExtensions.Get());
    mValue += 1.0;
    if (mValue > 1.0)
    {
      mValue = 0.0;
    }
    mState = kFSDone;
    SetDirty();
  }
}

void IFileSelectorControl::Draw(IGraphics& graphics)
{
  if (mState == kFSSelecting)
  {
    graphics.DrawBitmap(mBitmap, mRECT, 0, 0);
  }
}

void IFileSelectorControl::GetLastSelectedFileForPlug(WDL_String& str)
{
  str.Set(mFile.Get());
}

void IFileSelectorControl::SetLastSelectedFileFromPlug(const char* file)
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
