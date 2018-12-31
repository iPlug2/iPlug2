/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <cmath>

#include "IControl.h"
#include "IPlugParameter.h"

// avoid some UNICODE issues with VST3 SDK and WDL dirscan
#if defined VST3_API && defined OS_WIN
  #ifdef FindFirstFile
    #undef FindFirstFile
    #undef FindNextFile
    #undef WIN32_FIND_DATA
    #undef PWIN32_FIND_DATA
    #define FindFirstFile FindFirstFileA
    #define FindNextFile FindNextFileA
    #define WIN32_FIND_DATA WIN32_FIND_DATAA
    #define LPWIN32_FIND_DATA LPWIN32_FIND_DATAA
  #endif
#endif

#include "dirscan.h"

void DefaultAnimationFunc(IControl* pCaller)
{
  auto progress = pCaller->GetAnimationProgress();
  
  if(progress > 1.)
  {
    pCaller->OnEndAnimation();
    return;
  }
};

void FlashCircleAnimationFunc(IControl* pCaller)
{
  auto progress = pCaller->GetAnimationProgress();
  
  if(progress > 1.) {
    pCaller->OnEndAnimation();
    return;
  }
  
  dynamic_cast<IVectorBase*>(pCaller)->SetFlashCircleRadius((float) progress);
  
  pCaller->SetDirty(false);
};

void DefaultClickActionFunc(IControl* pCaller) { pCaller->SetAnimation(DefaultAnimationFunc, DEFAULT_ANIMATION_DURATION); };
void FlashCircleClickActionFunc(IControl* pCaller) { pCaller->SetAnimation(FlashCircleAnimationFunc, DEFAULT_ANIMATION_DURATION); }

IControl::IControl(IGEditorDelegate& dlg, IRECT bounds, int paramIdx, IActionFunction actionFunc)
: mDelegate(dlg)
, mRECT(bounds)
, mTargetRECT(bounds)
, mParamIdx(paramIdx)
, mActionFunc(actionFunc)
{
}

IControl::IControl(IGEditorDelegate& dlg, IRECT bounds, IActionFunction actionFunc)
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

  //don't update this control from delegate, if this control is being captured (i.e. if host is automating the control, mouse is more important
  IControl* capturedControl = GetUI()->GetCapturedControl();
  
  if (mValue != value && capturedControl != this)
  {
    mValue = value;
    SetDirty(false);
  }
}

void IControl::SetValueFromUserInput(double value)
{
  if (mValue != value)
  {
    mValue = value;
    SetDirty();
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
      mDelegate.SendParameterValueFromUI(mParamIdx, mValue);
      GetUI()->UpdatePeers(this);
      
      const IParam* pParam = GetParam();

      if (mValDisplayControl)
      {
        WDL_String display;
        pParam->GetDisplayForHost(display);
        ((ITextControl*)mValDisplayControl)->SetStr(display.Get());
      }

      if (mNameDisplayControl)
      {
        ((ITextControl*)mNameDisplayControl)->SetStr(pParam->GetNameForHost());
      }
    }
    
    if (mActionFunc != nullptr)
      mActionFunc(this);
  }
}

bool IControl::IsDirty()
{
  if(GetAnimationFunction()) {
    mAnimationFunc(this);
    return true;
  }
  
  return mDirty;
}

void IControl::Hide(bool hide)
{
  mHide = hide;
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

void IControl::OnPopupMenuSelection(IPopupMenu* pSelectedMenu)
{
  if (pSelectedMenu != nullptr && mParamIdx >= 0 && !mDisablePrompt)
  {
    SetValueFromUserInput(GetParam()->ToNormalized( (double) pSelectedMenu->GetChosenItemIdx() ));
  }
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
    
    SetDirty(false);
  }
}

void IControl::PromptUserInput(const IRECT& bounds)
{
  if (mParamIdx >= 0 && !mDisablePrompt)
  {
    GetUI()->PromptUserInput(*this, bounds);
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
    return mDelegate.GetParam(mParamIdx);
  else
    return nullptr;
}

void IControl::SnapToMouse(float x, float y, EDirection direction, IRECT& bounds, float scalar /* TODO: scalar! */)
{
  bounds.Constrain(x, y);

  float val;

  //val = 1.f - (y - (mRECT.B - (mRECT.H()*lengthMult)) / 2) / ((mLen*lengthMult));
  
  if(direction == kVertical)
    val = 1.f - (y-bounds.T) / bounds.H();
  else
    //mValue = (double) (x - (mRECT.R - (mRECT.W()*lengthMult)) - mHandleHeadroom / 2) / (double) ((mLen*lengthMult) - mHandleHeadroom);
    val = (x-bounds.L) / bounds.W();

  mValue = round( val / 0.001 ) * 0.001;

  SetDirty(true); // will send parameter value to delegate
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

void ITextControl::SetStr(const char* str)
{
  if (strcmp(mStr.Get(), str))
  {
    SetDirty(false);
    mStr.Set(str);
  }
}

void ITextControl::Draw(IGraphics& g)
{
  g.FillRect(mBGColor, mRECT);
  
  if (mStr.GetLength())
    g.DrawText(mText, mStr.Get(), mRECT);
}

ICaptionControl::ICaptionControl(IGEditorDelegate& dlg, IRECT bounds, int paramIdx, const IText& text, bool showParamLabel)
: ITextControl(dlg, bounds, "", text)
, mShowParamLabel(showParamLabel)
{
  mParamIdx = paramIdx;
  
  if(GetParam()->Type() == IParam::kTypeEnum)
  {
    mIsListControl = true;
  }
  
  mDblAsSingleClick = true;
  mDisablePrompt = false;
  mIgnoreMouse = false;
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

  ITextControl::Draw(g);
  
  if(mIsListControl) {
    IRECT triRect = mRECT.FracRectHorizontal(0.2f, true).GetCentredInside(IRECT(0, 0, 8, 5));
    g.FillTriangle(COLOR_DARK_GRAY, triRect.L, triRect.T, triRect.R, triRect.T, triRect.MW(), triRect.B, GetMouseIsOver() ? 0 : &BLEND_50);
  }
}

IButtonControlBase::IButtonControlBase(IGEditorDelegate& dlg, IRECT bounds, IActionFunction actionFunc)
: IControl(dlg, bounds, kNoParameter, actionFunc)
{
}

void IButtonControlBase::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  mValue = 1.;
  SetDirty(true);
}

void IButtonControlBase::OnEndAnimation()
{
  mValue = 0.;
  IControl::OnEndAnimation();
}

ISwitchControlBase::ISwitchControlBase(IGEditorDelegate& dlg, IRECT bounds, int paramIdx, IActionFunction actionFunc,
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
    const double step = 1. / (double(mNumStates) - 1.);
    mValue += step;
    if(mValue > 1.)
      mValue = 0.;
  }
  
  mMouseDown = true;
  SetDirty(true);
}

void ISwitchControlBase::OnMouseUp(float x, float y, const IMouseMod& mod)
{
  mMouseDown = false;
  SetDirty(false);
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
  mItems.Empty(false);
}

int IDirBrowseControlBase::NItems()
{
  return mItems.GetSize();
}

void IDirBrowseControlBase::AddPath(const char* path, const char* label)
{
  assert(strlen(path));

  mPaths.Add(new WDL_String(path));
  mPathLabels.Add(new WDL_String(label));
}

void IDirBrowseControlBase::CollectSortedItems(IPopupMenu* pMenu)
{
  int nItems = pMenu->NItems();
  
  for (int i = 0; i < nItems; i++)
  {
    IPopupMenu::Item* pItem = pMenu->GetItem(i);
    
    if(pItem->GetSubmenu())
      CollectSortedItems(pItem->GetSubmenu());
    else
      mItems.Add(pItem);
  }
}

void IDirBrowseControlBase::SetUpMenu()
{
  mFiles.Empty(true);
  mItems.Empty(false);
  
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
  
  CollectSortedItems(&mMainMenu);
}

//void IDirBrowseControlBase::GetSelectedItemLabel(WDL_String& label)
//{
//  if (mSelectedMenu != nullptr) {
//    if(mSelectedIndex > -1)
//      label.Set(mSelectedMenu->GetItem(mSelectedIndex)->GetText());
//  }
//  else
//    label.Set("");
//}
//
//void IDirBrowseControlBase::GetSelectedItemPath(WDL_String& path)
//{
//  if (mSelectedMenu != nullptr) {
//    if(mSelectedIndex > -1) {
//      path.Set(mPaths.Get(0)->Get()); //TODO: what about multiple paths
//      path.AppendFormatted(1024, "/%s", mSelectedMenu->GetItem(mSelectedIndex)->GetText());
//      path.Append(mExtension.Get());
//    }
//  }
//  else
//    path.Set("");
//}

void IDirBrowseControlBase::ScanDirectory(const char* path, IPopupMenu& menuToAddTo)
{
#if !defined OS_IOS
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
          parentDirMenu.AddItem(d.GetCurrentFN(), pNewMenu, -2);
          ScanDirectory(subdir.Get(), *pNewMenu);
        }
        else
        {
          const char* a = strstr(f, mExtension.Get());
          if (a && a > f && strlen(a) == strlen(mExtension.Get()))
          {
            WDL_String menuEntry {f};
            
            if(!mShowFileExtensions)
              menuEntry.Set(f, (int) (a - f));
            
            IPopupMenu::Item* pItem = new IPopupMenu::Item(menuEntry.Get(), IPopupMenu::Item::kNoFlags, mFiles.GetSize());
            parentDirMenu.AddItem(pItem, -2 /* sort alphabetically */);
            WDL_String* pFullPath = new WDL_String("");
            d.GetCurrentFullFN(pFullPath);
            mFiles.Add(pFullPath);
          }
        }
      }
    } while (!d.Next());
    
    menuToAddTo = parentDirMenu;
  }
  
  if(!mShowEmptySubmenus)
    parentDirMenu.RemoveEmptySubmenus();

#endif
}
