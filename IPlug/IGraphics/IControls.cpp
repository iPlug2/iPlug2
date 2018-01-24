#include "IControls.h"

void ISwitchControl::OnMouseDown(float x, float y, const IMouseMod& mod)
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

void ISwitchControl::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  OnMouseDown(x, y, mod);
}

void ISwitchPopUpControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  PromptUserInput();
  
  SetDirty();
}

ISwitchFramesControl::ISwitchFramesControl(IPlugBaseGraphics& plug, float x, float y, int paramIdx, IBitmap& bitmap, bool imagesAreHorizontal, IBlend::EType blendType)
: ISwitchControl(plug, x, y, paramIdx, bitmap, blendType)
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

void ISwitchFramesControl::OnMouseDown(float x, float y, const IMouseMod& mod)
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
:   IControl(plug, rect, paramIdx, IBlend::kBlendClobber)
{
  mDisablePrompt = true;
}

void IInvisibleSwitchControl::OnMouseDown(float x, float y, const IMouseMod& mod)
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
      float dX = (double) (rect.W() - nButtons * bitmap.W) / (double) (nButtons - 1);
      float x = mRECT.R - bitmap.W - dX;
      float y = mRECT.T;
      
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + bitmap.W, y + h);
        x -= bitmap.W + dX;
      }
    }
    else
    {
      float dY = (double) (rect.H() - nButtons * h) /  (double) (nButtons - 1);
      float x = mRECT.L;
      float y = mRECT.B - h - dY;
      
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + bitmap.W, y + h);
        y -= h + dY;
      }
    }
    
  }
  else
  {
    float x = mRECT.L, y = mRECT.T;
    
    if (direction == kHorizontal)
    {
      float dX = (double) (rect.W() - nButtons * bitmap.W) / (double) (nButtons - 1);
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + bitmap.W, y + h);
        x += bitmap.W + dX;
      }
    }
    else
    {
      float dY = (double) (rect.H() - nButtons * h) /  (double) (nButtons - 1);
      for (int i = 0; i < nButtons; ++i)
      {
        mRECTs.Get()[i] = IRECT(x, y, x + bitmap.W, y + h);
        y += h + dY;
      }
    }
  }
}

void IRadioButtonsControl::OnMouseDown(float x, float y, const IMouseMod& mod)
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

void IContactControl::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  mValue = 0.0;
  SetDirty();
}

IFaderControl::IFaderControl(IPlugBaseGraphics& plug, float x, float y, int len, int paramIdx, IBitmap& bitmap, EDirection direction, bool onlyHandle)
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

void IFaderControl::OnMouseDown(float x, float y, const IMouseMod& mod)
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

void IFaderControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  return SnapToMouse(x, y);
}

void IFaderControl::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
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

void IFaderControl::SnapToMouse(float x, float y)
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

bool IFaderControl::IsHit(float x, float y) const
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

void IKnobControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
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

void IKnobControl::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
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

IVKnobControl::IVKnobControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, const IColor& color, float rMin, float rMax, float aMin, float aMax, EDirection direction, double gearing)
: IKnobControl(plug, rect, paramIdx, direction, gearing)
, mColor(color)
, mAngleMin(aMin)
, mAngleMax(aMax)
, mInnerRadius(rMin)
, mOuterRadius(rMax)
{
  if (mOuterRadius == 0.0f)
  {
    mOuterRadius = 0.5f * (float) rect.W();
  }
  
  mBlend = IBlend(IBlend::kBlendClobber);
}

void IVKnobControl::Draw(IGraphics& graphics)
{
  const float v = mAngleMin + (mValue * (mAngleMax - mAngleMin));
  const float cx = mRECT.MW(), cy = mRECT.MH();
  const float radius = (mRECT.W()/2.f) - 2.f;
  graphics.DrawCircle(mColor, cx, cy, radius, &BLEND_50);
  graphics.FillArc(mColor, cx, cy, radius, mAngleMin, v, &BLEND_50);
  graphics.DrawRadialLine(mColor, cx, cy, v, mInnerRadius * radius, mOuterRadius * radius);
}

//void IKnobRotaterControl::Draw(IGraphics& graphics)
//{
//  int cX = (mRECT.L + mRECT.R) / 2;
//  int cY = (mRECT.T + mRECT.B) / 2;
//  double angle = mMinAngle + mValue * (mMaxAngle - mMinAngle);
//  graphics.DrawRotatedBitmap(mBitmap, cX, cY, angle, mYOffset, &mBlend);
//}

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

//void IKnobRotatingMaskControl::Draw(IGraphics& graphics)
//{
//  double angle = mMinAngle + mValue * (mMaxAngle - mMinAngle);
//  graphics.DrawRotatedMask(mBase, mMask, mTop, mRECT.L, mRECT.T, angle, &mBlend);
//}

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

ICaptionControl::ICaptionControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, IText& text, bool showParamLabel)
: ITextControl(plug, rect, text)
, mShowParamLabel(showParamLabel)
{
  mParamIdx = paramIdx;
}

void ICaptionControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mod.L || mod.R)
  {
    PromptUserInput();
  }
}

void ICaptionControl::OnMouseDblClick(float x, float y, const IMouseMod& mod)
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
, mURL(URL)
, mBackupURL(backupURL)
, mErrMsg(errMsgOnFailure)
{
  assert(strlen(URL) < MAX_URL_LEN);
  assert(strlen(backupURL) < MAX_URL_LEN);
  assert(strlen(errMsgOnFailure) < MAX_NET_ERR_MSG_LEN);
}

void IURLControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  bool opened = false;
  opened = mPlug.GetGUI()->OpenURL(mURL.Get(), mErrMsg.Get());
  
  if (!opened && mBackupURL.GetLength() > 0)
  {
    opened = mPlug.GetGUI()->OpenURL(mBackupURL.Get(), mErrMsg.Get());
  }
}

void IFileSelectorControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mPlug.GetGUI())
  {
    mState = kFSSelecting;
    SetDirty(false);
    
    mPlug.GetGUI()->PromptForFile(mFile, mDir, mFileAction, mExtensions.Get());
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
