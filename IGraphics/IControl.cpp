#include <cmath>

#include "IControl.h"
#include "IPlugParameter.h"

#include "dirscan.h"

void DefaultAnimationFunc(IControl* pCaller)
{
  auto progress = pCaller->GetAnimationProgress();
  
  if(progress > 1.)
  {
    pCaller->OnEndAnimation();
    return;
  }
  
  pCaller->Animate(progress);
};

void DefaultClickActionFunc(IControl* pCaller)
{
  pCaller->SetAnimation(DefaultAnimationFunc, DEFAULT_ANIMATION_DURATION);
};

IControl::IControl(IEditorDelegate& dlg, IRECT bounds, int paramIdx, IActionFunction actionFunc)
: mDelegate(dlg)
, mRECT(bounds)
, mTargetRECT(bounds)
, mParamIdx(paramIdx)
, mActionFunc(actionFunc)
{
}

IControl::IControl(IEditorDelegate& dlg, IRECT bounds, IActionFunction actionFunc)
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
      mDelegate.SendParameterValueFromUI(mParamIdx, mValue);
      GetUI()->UpdatePeers(this);
      const IParam* pParam = mDelegate.GetParam(mParamIdx);

      if (mValDisplayControl)
      {
        WDL_String display;
        pParam->GetDisplayForHost(display);
        ((ITextControl*)mValDisplayControl)->SetTextFromDelegate(display.Get());
      }

      if (mNameDisplayControl)
      {
        ((ITextControl*)mNameDisplayControl)->SetTextFromDelegate((char*) pParam->GetNameForHost());
      }
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
    mDelegate.SendParameterValueFromUI(pAuxParam->mParamIdx, pAuxParam->mValue);
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

  SetDirty(); // will send parameter value to delegate
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
  g.FillRect(mBGColor, mRECT);
  
  if (mStr.GetLength())
    g.DrawText(mText, mStr.Get(), mRECT);
}

ICaptionControl::ICaptionControl(IEditorDelegate& dlg, IRECT bounds, int paramIdx, const IText& text, bool showParamLabel)
: ITextControl(dlg, bounds, "", paramIdx, text)
, mShowParamLabel(showParamLabel)
{
  assert(paramIdx > kNoParameter);

  mParamIdx = paramIdx;
  
  if(GetParam()->Type() == IParam::kTypeEnum)
  {
    mIsListControl = true;
  }
  
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

  ITextControl::Draw(g);
  
  if(mIsListControl) {
    IRECT triRect = mRECT.FracRectHorizontal(0.2, true).GetCentredInside(IRECT(0, 0, 8, 5));
    g.FillTriangle(COLOR_DARK_GRAY, triRect.L, triRect.T, triRect.R, triRect.T, triRect.MW(), triRect.B, GetMouseIsOver() ? 0 : &BLEND_50);
  }
}

ISwitchControlBase::ISwitchControlBase(IEditorDelegate& dlg, IRECT bounds, int paramIdx, IActionFunction actionFunc,
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
#endif
}

IPopupMenuControlBase::IPopupMenuControlBase(IEditorDelegate& dlg, int paramIdx, IRECT collapsedBounds, IRECT expandedBounds, EDirection direction)
: IControl(dlg, collapsedBounds, paramIdx)
, mSpecifiedCollapsedBounds(collapsedBounds)
, mSpecifiedExpandedBounds(expandedBounds)
, mDirection(direction)
{
  SetActionFunction(DefaultClickActionFunc);
  
  mText = IText(COLOR_BLACK, 24, nullptr, IText::kStyleNormal, IText::kAlignNear);
  mDirty = false;
}


void IPopupMenuControlBase::Draw(IGraphics& g)
{
  assert(mMenu != nullptr);
  
  DrawBackground(g, mTargetRECT); // mTargetRECT = inner area
  DrawShadow(g, mRECT);

  for(auto i = 0; i < mExpandedCellBounds.GetSize(); i++)
  {
    IRECT* pCellRect = mExpandedCellBounds.Get(i);
    IPopupMenu::Item* pMenuItem = mMenu->GetItem(i);

    if(pMenuItem->GetIsSeparator())
      DrawSeparator(g, *pCellRect);
    else
    {
      if(mMouseCellBounds == pCellRect)
      {
        DrawHighlightCell(g, *pCellRect, *pMenuItem);
        DrawHighlightCellText(g, *pCellRect, *pMenuItem);
      }
      else
      {
        DrawCell(g, *pCellRect, *pMenuItem);
        DrawCellText(g, *pCellRect, *pMenuItem);
      }
    }
  }
}

void IPopupMenuControlBase::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(GetState() == kExpanded)
  {
    mMouseCellBounds = HitTestCells(x, y);
    Collapse();
  }
  else
    IControl::OnMouseDown(x, y, mod);
}

void IPopupMenuControlBase::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  mMouseCellBounds = HitTestCells(x, y);
  SetDirty(false);
}

void IPopupMenuControlBase::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  mMouseCellBounds = HitTestCells(x, y);
  SetDirty(false);
}

void IPopupMenuControlBase::OnMouseOut()
{
  mMouseCellBounds = nullptr;
  
  if(GetState() == kExpanded)
    Collapse();
}

void IPopupMenuControlBase::DrawBackground(IGraphics& g, const IRECT& bounds)
{
  g.FillRoundRect(COLOR_WHITE, bounds, mRoundness, &mBlend);
}

void IPopupMenuControlBase::DrawShadow(IGraphics& g, const IRECT& bounds)
{
  g.DrawBoxShadow(bounds, mRoundness, 2., mDropShadowSize, &mBlend);
}

void IPopupMenuControlBase::DrawCell(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem)
{
}

void IPopupMenuControlBase::DrawHighlightCell(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem)
{
  g.FillRect(COLOR_BLUE, bounds.GetHPadded(mPadding), &mBlend);
}

void IPopupMenuControlBase::DrawCellText(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem)
{
  IRECT textRect = bounds.GetHPadded(-TEXT_PAD);
  mText.mFGColor = COLOR_BLACK;
  g.DrawText(mText, menuItem.GetText(), textRect, &mBlend);
}

void IPopupMenuControlBase::DrawHighlightCellText(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem)
{
  IRECT textRect = bounds.GetHPadded(-TEXT_PAD);
  mText.mFGColor = COLOR_WHITE;
  g.DrawText(mText, menuItem.GetText(), textRect, &mBlend);
}

void IPopupMenuControlBase::DrawSeparator(IGraphics& g, const IRECT& bounds)
{
  g.FillRect(COLOR_LIGHT_GRAY, bounds, &mBlend);
}

IPopupMenu* IPopupMenuControlBase::CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller)
{
  mMenu = &menu;
  mCaller = pCaller;

  IRECT span = {0, 0, bounds.W(), bounds.H() }; // this IRECT will be used to calculate the maximum dimensions of the longest text item in the menu

  for (auto i = 0; i < mMenu->NItems(); ++i)
  {
    IRECT textBounds;
    GetUI()->MeasureText(mText, mMenu->GetItem(i)->GetText(), textBounds);
    span = span.Union(textBounds);
  }
  
  // if the widest string is wider than the requested bounds
  if(span.W() > bounds.W())
  {
    span.HPad(TEXT_PAD); // add some padding because we don't want to be flush to the edges
    mSingleCellBounds = IRECT(bounds.L, bounds.T, bounds.L + span.W(), bounds.T + span.H());
  }
  else
    mSingleCellBounds = bounds;
  
  Expand();
  
  return mMenu;
}

void IPopupMenuControlBase::Expand()
{
  mExpandedCellBounds.Empty(true);
  
  float left = mSingleCellBounds.L + mPadding;
  float top = mSingleCellBounds.T + mPadding;
  
  IRECT contextBounds = GetUI()->GetBounds();
  
  if(mDirection == kVertical)
  {
    for (auto i = 0; i < mMenu->NItems(); ++i)
    {
      IPopupMenu::Item* pMenuItem = mMenu->GetItem(i);
      float right, bottom;
      float toAddX, toAddY; // the increments, different depending on item type
      
      if (pMenuItem->GetIsSeparator())
      {
        toAddX = CellWidth();
        toAddY = mSeparatorSize;
      }
      else
      {
        toAddX = CellWidth();
        toAddY = CellHeight();
      }
      
      if((top + toAddY + mPadding) > contextBounds.B) // it's gonna go off the bottom
      {
        if(mScrollIfTooBig)
        {
          //TODO: scroll
        }
        else // new column
        {
          left += mSingleCellBounds.W() + mCellGap;
          top = mSingleCellBounds.T + mPadding;
        }
      }
      
      if((left + toAddX + mPadding) > contextBounds.R) // it's gonna go off the right hand side
      {
        assert(true);
        // TODO: the whole thing is gonna have to go the other way mate
      }

      if (pMenuItem->GetSubmenu())
      {
        assert(true);
        //TODO: submenu
      }
      
      right = left + toAddX;
      bottom = top + toAddY;
      
      IRECT* pR = new IRECT(left, top, right, bottom);
      mExpandedCellBounds.Add(pR);
      top = bottom + mCellGap;
    }
    
    IRECT span = *mExpandedCellBounds.Get(0);
    
    for(auto i = 1; i < mExpandedCellBounds.GetSize(); i++)
    {
      span = span.Union(*mExpandedCellBounds.Get(i));
    }
    
    if (mSpecifiedExpandedBounds.W())
    {
      mTargetRECT = mSpecifiedExpandedBounds.GetPadded(mPadding); // pad the unioned cell rects)
      mRECT = mSpecifiedExpandedBounds.GetPadded(mDropShadowSize + mPadding);
    }
    else
    {
      mTargetRECT = span.GetPadded(mPadding); // pad the unioned cell rects)
      mRECT = span.GetPadded(mDropShadowSize + mPadding);
    }
    
    mState = kExpanding;
    GetUI()->UpdateTooltips(); //will disable
  }
//  else // TODO: horizontal
//  {
//
//  }
  
  SetDirty(true); // triggers animation
}

void IPopupMenuControlBase::Collapse()
{
  mMenu->SetChosenItemIdx(-1);
  
  for(auto i = 0; i < mExpandedCellBounds.GetSize(); i++)
  {
    if(mMouseCellBounds == mExpandedCellBounds.Get(i))
    {
      mMenu->SetChosenItemIdx(i);
    }
  }
  
  if(mMenu->GetFunction())
    mMenu->ExecFunction();
  
  if(mCaller)
    mCaller->OnPopupMenuSelection(mMenu); // TODO: In the synchronous pop-up menu handlers it's possible for mMenu to be null, that should also be possible here if nothing was selected
  
  mState = kCollapsing;
  SetDirty(true); // triggers animation
}

void IPopupMenuControlBase::Animate(double progress)
{
  if(mState == kExpanding)
    mBlend.mWeight = progress * mOpacity;
  else if(mState == kCollapsing)
    mBlend.mWeight = (1.-progress) * mOpacity;

  GetUI()->SetAllControlsDirty();
}

void IPopupMenuControlBase::OnEndAnimation()
{
  if(mState == kExpanding)
  {
    mBlend.mWeight = mOpacity;
    mState = kExpanded;
  }
  else if(mState == kCollapsing)
  {
    mTargetRECT = mRECT = mSpecifiedCollapsedBounds;
    GetUI()->UpdateTooltips(); // will enable the tooltips
    mBlend.mWeight = 0.;
    mState = kCollapsed;
  }
  
  IControl::OnEndAnimation();
}
