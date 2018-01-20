#include <cmath>

#include "IControl.h"
#include "Log.h"

IControl::IControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, IBlend blendType)
: mPlug(plug)
, mRECT(rect)
, mTargetRECT(rect)
, mParamIdx(paramIdx)
, mBlend(blendType)
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
    mPlug.SetParameterFromUI(auxParam->mParamIdx, auxParam->mValue);
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
    graphics.FillCircle(mPTHighlightColor, mRECT.R-5, mRECT.T+5, 2, &mBlend, true);
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
  graphics.DrawSVG(mSVG, mRECT);
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
    graphics.DrawIText(mText, cStr, mRECT);
  }
}
