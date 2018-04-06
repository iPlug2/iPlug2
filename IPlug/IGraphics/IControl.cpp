#include <cmath>

#include "IControl.h"
#include "IPlugParameter.h"

void DefaultAnimationFunc(IControl* pCaller)
{
  auto progress = pCaller->GetAnimationProgress();
  
  if(progress > 1.)
  {
    pCaller->EndAnimation();
    return;
  }
  
  pCaller->Animate(progress);
};

void DefaultClickActionFunc(IControl* pCaller)
{
  pCaller->SetAnimation(DefaultAnimationFunc, DEFAULT_ANIMATION_DURATION);
};

IControl::IControl(IDelegate& dlg, IRECT bounds, int paramIdx, IActionFunction actionFunc)
: mDelegate(dlg)
, mRECT(bounds)
, mTargetRECT(bounds)
, mParamIdx(paramIdx)
, mActionFunc(actionFunc)
{
}

IControl::IControl(IDelegate& dlg, IRECT bounds, IActionFunction actionFunc)
: mDelegate(dlg)
, mRECT(bounds)
, mTargetRECT(bounds)
, mParamIdx(kNoParameter)
, mActionFunc(actionFunc)
{
}

void IControl::SetValueFromDelegate(double value)
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

void IControl::SetDirty(bool triggerAction)
{
  mValue = Clip(mValue, mClampLo, mClampHi);
  mDirty = true;
  
  if (triggerAction)
  {
    if(mParamIdx > kNoParameter)
    {
      mDelegate.SetParameterValueFromUI(mParamIdx, mValue);
      GetUI()->UpdatePeers(this);
  //    const IParam* pParam = mDelegate.GetParamFromUI(mParamIdx);

  //    if (mValDisplayControl)
  //    {
  //      WDL_String display;
  //      pParam->GetDisplayForHost(display);
  //      ((ITextControl*)mValDisplayControl)->SetTextFromDelegate(display.Get());
  //    }
  //
  //    if (mNameDisplayControl)
  //    {
  //      ((ITextControl*)mNameDisplayControl)->SetTextFromDelegate((char*) pParam->GetNameForHost());
  //    }
    }
    
    if (mActionFunc != nullptr)
      mActionFunc(this);
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
  SetDirty(false);
}

void IControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  #ifdef PROTOOLS
  if (mod.A && mDefaultValue >= 0.0)
  {
    mValue = mDefaultValue;
    SetDirty();
  }
  #endif

  if (mod.R)
		PromptUserInput();
}

void IControl::OnMouseDblClick(float x, float y, const IMouseMod& mod)
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
    if (GetParam()->NDisplayTexts()) // popup menu
    {
      GetUI()->PromptUserInput(*this, mRECT);
    }
    else // text entry
    {
      float cX = mRECT.MW();
      float cY = mRECT.MH();
      float halfW = float(PARAM_EDIT_W)/2.f;
      float halfH = float(PARAM_EDIT_H)/2.f;

      IRECT txtRECT = IRECT(cX - halfW, cY - halfH, cX + halfW,cY + halfH);
      GetUI()->PromptUserInput(*this, txtRECT);
    }

    Redraw();
  }
}

void IControl::PromptUserInput(IRECT& bounds)
{
  if (mParamIdx >= 0 && !mDisablePrompt)
  {
    GetUI()->PromptUserInput(*this, bounds);
    Redraw();
  }
}

IControl::AuxParam* IControl::GetAuxParam(int idx)
{
  assert(idx > -1 && idx < mAuxParams.GetSize());
  return mAuxParams.Get() + idx;
}

int IControl::GetAuxParamIdx(int paramIdx)
{
  for (int i=0;i<NAuxParams();i++)
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

void IControl::SetAuxParamValueFromDelegate(int auxParam, double value)
{
  AuxParam* pAuxParam = GetAuxParam(auxParam);

  if (pAuxParam->mValue != value)
  {
    pAuxParam->mValue = value;
    SetDirty(false);
    Redraw();
  }
}

void IControl::SetAllAuxParamsFromGUI()
{
  for (int i=0;i<mAuxParams.GetSize();i++)
  {
    AuxParam* pAuxParam = GetAuxParam(i);
    mDelegate.SetParameterValueFromUI(pAuxParam->mParamIdx, pAuxParam->mValue);
  }
}

void IControl::SetPTParameterHighlight(bool isHighlighted, int color)
{
  switch (color)
  {
    case 0: //AAX_eHighlightColor_Red
      mPTHighlightColor = COLOR_RED;
      break;
    case 1: //AAX_eHighlightColor_Blue
      mPTHighlightColor = COLOR_BLUE;
      break;
    case 2: //AAX_eHighlightColor_Green
      mPTHighlightColor = COLOR_GREEN;
      break;
    case 3: //AAX_eHighlightColor_Yellow
      mPTHighlightColor = COLOR_YELLOW;
      break;
    default:
      break;
  }

  mPTisHighlighted = isHighlighted;
  SetDirty(false);
}

void IControl::DrawPTHighlight(IGraphics& g)
{
  if (mPTisHighlighted)
  {
    g.FillCircle(mPTHighlightColor, mRECT.R-5, mRECT.T+5, 2);
  }
}

const IParam* IControl::GetParam()
{
  if(mParamIdx >= 0)
    return mDelegate.GetParamObjectFromUI(mParamIdx);
  else
    return nullptr;
}

void IControl::SnapToMouse(float x, float y, EDirection direction, IRECT& bounds, float scalar /* TODO:! */)
{
  bounds.Constrain(x, y);

  float val;

  if(direction == kVertical)
    val = 1.f - (y-bounds.T) / bounds.H(); //mValue = 1.0 - (double) (y - (mRECT.B - (mRECT.H()*lengthMult)) - mHandleHeadroom / 2) / (double) ((mLen*lengthMult) - mHandleHeadroom);
  else
    val = 1.f - (x-bounds.B) / bounds.W(); //mValue = (double) (x - (mRECT.R - (mRECT.W()*lengthMult)) - mHandleHeadroom / 2) / (double) ((mLen*lengthMult) - mHandleHeadroom);

  mValue = round( val / 0.001 ) * 0.001;

  SetDirty(); // will send parameter value to delegate
}

void IControl::GetJSON(WDL_String& json, int idx) const
{
  json.AppendFormatted(8192, "{");
  json.AppendFormatted(8192, "\"id\":%i, ", idx);
//  json.AppendFormatted(8192, "\"class\":\"%s\", ", typeid(*this).name());
//  json.AppendFormatted(8192, "\"min\":%f, ", GetMin());
//  json.AppendFormatted(8192, "\"max\":%f, ", GetMax());
//  json.AppendFormatted(8192, "\"default\":%f, ", GetDefault());
  json.AppendFormatted(8192, "}");
}

void IBitmapControl::Draw(IGraphics& g)
{
  int i = 1;
  if (mBitmap.N() > 1)
  {
    i = 1 + int(0.5 + mValue * (double) (mBitmap.N() - 1));
    i = Clip(i, 1, mBitmap.N());
  }

  g.DrawBitmap(mBitmap, mRECT, i, &mBlend);
}

void IBitmapControl::OnRescale()
{
  mBitmap = GetUI()->GetScaledBitmap(mBitmap);
}

void ITextControl::SetTextFromDelegate(const char* str)
{
  if (strcmp(mStr.Get(), str))
  {
    SetDirty(false);
    mStr.Set(str);
  }
}

void ITextControl::Draw(IGraphics& g)
{
  char* cStr = mStr.Get();
  if (CStringHasContents(cStr))
  {
    g.DrawText(mText, cStr, mRECT);
  }
}

ICaptionControl::ICaptionControl(IDelegate& dlg, IRECT bounds, int paramIdx, const IText& text, bool showParamLabel)
: ITextControl(dlg, bounds, text)
, mShowParamLabel(showParamLabel)
{
  assert(paramIdx > kNoParameter);

  mParamIdx = paramIdx;
  mDblAsSingleClick = true;
  mDisablePrompt = false;
}

void ICaptionControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mod.L || mod.R)
  {
    PromptUserInput(mRECT);
  }
}

void ICaptionControl::Draw(IGraphics& g)
{
  const IParam* pParam = GetParam();

  if(pParam)
  {
    pParam->GetDisplayForHost(mStr);

    if (mShowParamLabel)
    {
      mStr.Append(" ");
      mStr.Append(pParam->GetLabelForHost());
    }
  }

  return ITextControl::Draw(g);
}

ISwitchControlBase::ISwitchControlBase(IDelegate& dlg, IRECT bounds, int paramIdx, std::function<void(IControl*)> actionFunc,
  int numStates)
  : IControl(dlg, bounds, paramIdx, actionFunc)
{
  if (paramIdx > kNoParameter)
    mNumStates = (int) GetParam()->GetRange() + 1;
  else
    mNumStates = numStates;

  assert(mNumStates > 1);
}

void ISwitchControlBase::OnMouseDown(float x, float y, const IMouseMod& mod)
{  
  if (mNumStates == 2)
    mValue = !mValue;
  else
  {
    const float step = 1.f / float(mNumStates) - 1.f;
    mValue += step;
    mValue = fmod(1., mValue);
  }

  SetDirty();
}

void IKnobControlBase::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
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
    mValue += (double)dY / (double)(mRECT.T - mRECT.B) / gearing;
  }
  else
  {
    mValue += (double)dX / (double)(mRECT.R - mRECT.L) / gearing;
  }

  SetDirty();
}

void IKnobControlBase::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
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

IDirBrowseControlBase::~IDirBrowseControlBase()
{
  mFiles.Empty(true);
  mPaths.Empty(true);
  mPathLabels.Empty(true);
}

int IDirBrowseControlBase::NItems()
{
  return mFiles.GetSize();
}

void IDirBrowseControlBase::AddPath(const char * path, const char * label)
{
  mPaths.Add(new WDL_String(path));
  mPathLabels.Add(new WDL_String(label));
}

void IDirBrowseControlBase::SetUpMenu()
{
  mFiles.Empty(true);
  mMainMenu.Clear();
  mSelectedIndex = -1;

  int idx = 0;

  if (mPaths.GetSize() == 1)
  {
    ScanDirectory(mPaths.Get(0)->Get(), mMainMenu);
  }
  else
  {
    for (int p = 0; p<mPaths.GetSize(); p++)
    {
      IPopupMenu* pNewMenu = new IPopupMenu();
      mMainMenu.AddItem(mPathLabels.Get(p)->Get(), idx++, pNewMenu);
      ScanDirectory(mPaths.Get(p)->Get(), *pNewMenu);
    }
  }
}

void IDirBrowseControlBase::GetSelecteItemPath(WDL_String& path)
{
  if (mSelectedMenu != nullptr) {
    path.Append(mPaths.Get(0)->Get()); //TODO: what about multiple paths
    path.Append(mSelectedMenu->GetItem(mSelectedIndex)->GetText());
    path.Append(mExtension.Get());
  }
  else
    path.Set("");
}

void IDirBrowseControlBase::ScanDirectory(const char* path, IPopupMenu& menuToAddTo)
{
  WDL_DirScan d;
  IPopupMenu& parentDirMenu = menuToAddTo;

  if (!d.First(path))
  {
    do
    {
      const char* f = d.GetCurrentFN();
      if (f && f[0] != '.')
      {
        if (d.GetCurrentIsDirectory())
        {
          WDL_String subdir;
          d.GetCurrentFullFN(&subdir);
          IPopupMenu* pNewMenu = new IPopupMenu();
          parentDirMenu.AddItem(d.GetCurrentFN(), pNewMenu);
          ScanDirectory(subdir.Get(), *pNewMenu);
        }
        else
        {
          const char* a = strstr(f, mExtension.Get());
          if (a && a > f && strlen(a) == strlen(mExtension.Get()))
          {
            WDL_String menuEntry = WDL_String(f, (int) (a - f));
            parentDirMenu.AddItem(new IPopupMenu::Item(menuEntry.Get(), IPopupMenu::Item::kNoFlags, mFiles.GetSize()));
            WDL_String* pFullPath = new WDL_String("");
            d.GetCurrentFullFN(pFullPath);
            mFiles.Add(pFullPath);
          }
        }
      }
    } while (!d.Next());

    menuToAddTo = parentDirMenu;
  }
}
