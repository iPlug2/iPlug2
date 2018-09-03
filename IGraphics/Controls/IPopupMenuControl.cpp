#include "IPopupMenuControl.h"

IPopupMenuControl::IPopupMenuControl(IGEditorDelegate& dlg, int paramIdx, IText text, IRECT collapsedBounds, IRECT expandedBounds, EDirection direction)
: IControl(dlg, collapsedBounds, paramIdx)
, mSpecifiedCollapsedBounds(collapsedBounds)
, mSpecifiedExpandedBounds(expandedBounds)
, mDirection(direction)
{
  SetActionFunction([&](IControl* pCaller){
    
    SetAnimation([&](IControl* pCaller) {
      auto progress = pCaller->GetAnimationProgress();
      
      if(progress > 1.) {
        pCaller->OnEndAnimation();
        return;
      }
      
      if(mState == kExpanding)
        mBlend.mWeight = (float) progress * mOpacity;
      else if(mState == kCollapsing)
        mBlend.mWeight = (float) (1.-progress) * mOpacity;
      
      GetUI()->SetAllControlsDirty();
    },
                 DEFAULT_ANIMATION_DURATION);
  });
  
  
  mText = text;
  mDirty = false;
}


void IPopupMenuControl::Draw(IGraphics& g)
{
  assert(mMenu != nullptr);
  
  DrawBackground(g, mTargetRECT); // mTargetRECT = inner area
  DrawShadow(g, mRECT);
  
  for(auto i = 0; i < mExpandedCellBounds.GetSize(); i++)
  {
    IRECT* pCellRect = mExpandedCellBounds.Get(i);
    IPopupMenu::Item* pMenuItem = mMenu->GetItem(i);
    
    if(!pMenuItem)
      return;
    
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

void IPopupMenuControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(GetState() == kExpanded)
  {
    mMouseCellBounds = HitTestCells(x, y);
    Collapse();
  }
  else
    IControl::OnMouseDown(x, y, mod);
}

void IPopupMenuControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  mMouseCellBounds = HitTestCells(x, y);
  SetDirty(false);
}

void IPopupMenuControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  mMouseCellBounds = HitTestCells(x, y);
  SetDirty(false);
}

void IPopupMenuControl::OnMouseOut()
{
  mMouseCellBounds = nullptr;
}

void IPopupMenuControl::DrawBackground(IGraphics& g, const IRECT& bounds)
{
  g.FillRoundRect(COLOR_WHITE, bounds, mRoundness, &mBlend);
}

void IPopupMenuControl::DrawShadow(IGraphics& g, const IRECT& bounds)
{
  g.DrawBoxShadow(bounds, mRoundness, 2., mDropShadowSize, &mBlend);
}

void IPopupMenuControl::DrawCell(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem)
{
}

void IPopupMenuControl::DrawHighlightCell(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem)
{
  g.FillRect(COLOR_BLUE, bounds.GetHPadded(mPadding), &mBlend);
}

void IPopupMenuControl::DrawCellText(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem)
{
  IRECT textRect = bounds.GetHPadded(-TEXT_PAD);
  mText.mFGColor = COLOR_BLACK;
  mText.mAlign = IText::kAlignNear;
  g.DrawText(mText, menuItem.GetText(), textRect, &mBlend);
}

void IPopupMenuControl::DrawHighlightCellText(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item& menuItem)
{
  IRECT textRect = bounds.GetHPadded(-TEXT_PAD);
  mText.mFGColor = COLOR_WHITE;
  g.DrawText(mText, menuItem.GetText(), textRect, &mBlend);
}

void IPopupMenuControl::DrawSeparator(IGraphics& g, const IRECT& bounds)
{
  if(mBlend.mWeight > 0.9)
    g.FillRect(COLOR_MID_GRAY, bounds, &BLEND_25);
}

IPopupMenu* IPopupMenuControl::CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller)
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

void IPopupMenuControl::Expand()
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

void IPopupMenuControl::Collapse()
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

void IPopupMenuControl::OnEndAnimation()
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
