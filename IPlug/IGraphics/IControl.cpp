#include <cmath>

#include "IControl.h"

IControl::IControl(IPlugBaseGraphics& plug, IRECT rect, int param, IActionFunction actionFunc)
: mPlug(plug)
, mRECT(rect)
, mTargetRECT(rect)
, mParamIdx(param)
, mActionFunc(actionFunc)
{
}

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
    mPlug.SetParameterFromUI(mParamIdx, mValue);
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

void IControl::OnMouseDown(float x, float y, const IMouseMod& mod)
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
    if (mPlug.GetParam(mParamIdx)->GetNDisplayTexts()) // popup menu
    {
      mPlug.GetGUI()->PromptUserInput(this, mPlug.GetParam(mParamIdx), mRECT);
    }
    else // text entry
    {
      float cX = mRECT.MW();
      float cY = mRECT.MH();
      float halfW = float(PARAM_EDIT_W)/2.f;
      float halfH = float(PARAM_EDIT_H)/2.f;

      IRECT txtRECT = IRECT(cX - halfW, cY - halfH, cX + halfW,cY + halfH);
      mPlug.GetGUI()->PromptUserInput(this, mPlug.GetParam(mParamIdx), txtRECT);
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

int IControl::AuxParamIdx(int param)
{
  for (int i=0;i<mAuxParams.GetSize();i++)
  {
    if(GetAuxParam(i)->mParamIdx == param)
      return i;
  }
  
  return -1;
}

void IControl::AddAuxParam(int param)
{
  mAuxParams.Add(AuxParam(param));
}

void IControl::SetAuxParamValueFromPlug(int auxParam, double value)
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
    mPlug.SetParameterFromUI(pAuxParam->mParamIdx, pAuxParam->mValue);
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

void IControl::DrawPTHighlight(IGraphics& graphics)
{
  if (mPTisHighlighted)
  {
    graphics.FillCircle(mPTHighlightColor, mRECT.R-5, mRECT.T+5, 2, &mBlend);
  }
}

void IControl::GetJSON(WDL_String& json, int idx) const
{
  json.AppendFormatted(8192, "{");
  json.AppendFormatted(8192, "\"id\":%i, ", idx);
//  json.AppendFormatted(8192, "\"class\":\"%s\", ", typeid(*this).name());
//  json.AppendFormatted(8192, "\"min\":%f, ", GetMin());
//  json.AppendFormatted(8192, "\"max\":%f, ", GetMax());
//  json.AppendFormatted(8192, "\"default\":%f, ", GetDefault());
  json.AppendFormatted(8192, "\"rate\":\"audio\"");
  json.AppendFormatted(8192, "}");
}

void IPanelControl::Draw(IGraphics& graphics)
{
  graphics.FillRect(mColor, mRECT, &mBlend);
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

void ISVGControl::Draw(IGraphics& graphics)
{
  graphics.DrawRotatedSVG(mSVG, mRECT.MW(), mRECT.MH(), mRECT.W(), mRECT.H(), 78  * PI / 180.0);
    //graphics.DrawSVG(mSVG, mRECT);
};

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
    graphics.DrawText(mText, cStr, mRECT);
  }
}

IButtonControlBase::IButtonControlBase(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, std::function<void(IControl*)> actionFunc,
  uint32_t numStates)
  : IControl(plug, rect, paramIdx, actionFunc)
{
  if (paramIdx > -1)
    mNumStates = (uint32_t)mPlug.GetParam(paramIdx)->GetRange() + 1;
  else
    mNumStates = numStates;

  assert(mNumStates > 1);
}

void IButtonControlBase::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (mNumStates == 2)
    mValue = !mValue;
  else
  {
    const float step = 1.f / float(mNumStates) - 1.f;
    mValue += step;
    mValue = fmod(1., mValue);
  }

  if (mActionFunc != nullptr)
    mActionFunc(this);

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
    ScanDirectory(mPaths.Get(0)->Get(), &mMainMenu);
  }
  else
  {
    for (int p = 0; p<mPaths.GetSize(); p++)
    {
      IPopupMenu* pNewMenu = new IPopupMenu();
      mMainMenu.AddItem(mPathLabels.Get(p)->Get(), idx++, pNewMenu);

      IPopupMenu* pMenuToAddTo = pNewMenu;
      ScanDirectory(mPaths.Get(p)->Get(), pMenuToAddTo);
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

void IDirBrowseControlBase::ScanDirectory(const char* path, IPopupMenu* pMenuToAddTo)
{
  WDL_DirScan d;
  IPopupMenu* pParentDirMenu = pMenuToAddTo;

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
          pMenuToAddTo->AddItem(d.GetCurrentFN(), pNewMenu);
          ScanDirectory(subdir.Get(), pNewMenu);
        }
        else
        {
          const char* a = strstr(f, mExtension.Get());
          if (a && a > f && strlen(a) == strlen(mExtension.Get()))
          {
            WDL_String menuEntry = WDL_String(f, (int) (a - f));
            pParentDirMenu->AddItem(new IPopupMenu::Item(menuEntry.Get(), IPopupMenu::Item::kNoFlags, mFiles.GetSize()));
            WDL_String* pFullPath = new WDL_String("");
            d.GetCurrentFullFN(pFullPath);
            mFiles.Add(pFullPath);
          }
        }
      }
    } while (!d.Next());

    pMenuToAddTo = pParentDirMenu;
  }
}
