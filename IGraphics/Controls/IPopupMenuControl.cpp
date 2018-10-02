#include "IPopupMenuControl.h"


IPopupMenuControl::IPopupMenuControl(IGEditorDelegate& dlg, int paramIdx, IText text, IRECT collapsedBounds, IRECT expandedBounds)
: IControl(dlg, collapsedBounds, paramIdx)
, mSpecifiedCollapsedBounds(collapsedBounds)
, mSpecifiedExpandedBounds(expandedBounds)
{
  SetActionFunction([&](IControl* pCaller) {
    
    int duration = DEFAULT_ANIMATION_DURATION;
    
    if(mState == kSubMenuAppearing)
      duration = DEFAULT_ANIMATION_DURATION * 2;
    
    SetAnimation([&](IControl* pCaller) {
      auto progress = pCaller->GetAnimationProgress();
      
      if(progress > 1.) {
        pCaller->OnEndAnimation();
        return;
      }
      
      if(mState == kExpanding)
      {
        if(mAppearingMenuPanel != nullptr)
          mAppearingMenuPanel->mBlend.mWeight = (float) progress * mOpacity;
      }
      else if(mState == kSubMenuAppearing)
      {
        if(mAppearingMenuPanel != nullptr)
          mAppearingMenuPanel->mBlend.mWeight = (float) (progress > 0.9) * mOpacity;
      }
      else if(mState == kCollapsing)
      {
        for (auto i = 0; i < mMenuPanels.GetSize(); i++) {
          mMenuPanels.Get(i)->mBlend.mWeight = (float) (1.-progress) * mOpacity;
        }
      }
      else if(mState == kIdling) // Idling is a special stage, to force the menu to redraw, before complete collapse
      {
        for (auto i = 0; i < mMenuPanels.GetSize(); i++) {
          mMenuPanels.Get(i)->mBlend.mWeight = 0.f;
        }
      }
    },
    duration);
  });
  
  mText = text;
  mDirty = false;
}

IPopupMenuControl::~IPopupMenuControl()
{
  mMenuPanels.Empty(true);
}

void IPopupMenuControl::Draw(IGraphics& g)
{
  assert(mMenu != nullptr);
  
  for (auto mr = 0; mr < mMenuPanels.GetSize(); mr++)
  {
    MenuPanel* pMenuPanel = mMenuPanels.Get(mr);
    
    if(pMenuPanel->mShouldDraw)
    {
      DrawPanelBackground(g, pMenuPanel->mTargetRECT, &pMenuPanel->mBlend); // mTargetRECT = inner area
      DrawPanelShadow(g, pMenuPanel->mRECT, &pMenuPanel->mBlend);
      
      int nItems = pMenuPanel->mMenu.NItems();
      int nCells = pMenuPanel->mCellBounds.GetSize();
      int startCell = 0;
      int endCell = nCells-1;
      int cellOffset = 0;

      if(nItems > nCells)
      {
        if(pMenuPanel->mScrollItemOffset > 0)
        {
          IRECT* pCellRect = pMenuPanel->mCellBounds.Get(0);
          bool sel = mMouseCellBounds == pCellRect || pCellRect == pMenuPanel->mHighlightedCell;

          DrawCellBackground(g, *pCellRect, nullptr, sel, &pMenuPanel->mBlend);
          DrawUpArrow(g, *pCellRect, sel, &pMenuPanel->mBlend);
          
          startCell++;
        }
        
        if(pMenuPanel->mScrollItemOffset < nItems-nCells)
        {
          IRECT* pCellRect = pMenuPanel->mCellBounds.Get(nCells-1);
          bool sel = mMouseCellBounds == pCellRect || pCellRect == pMenuPanel->mHighlightedCell;
          
          DrawCellBackground(g, *pCellRect, nullptr, sel, &pMenuPanel->mBlend);
          DrawDownArrow(g, *pCellRect, sel, &pMenuPanel->mBlend);
          
          endCell--; // last one is going to be an arrow
        }
      }
      
      for(auto i = startCell; i <= endCell; i++)
      {
        IRECT* pCellRect = pMenuPanel->mCellBounds.Get(i);
        IPopupMenu::Item* pMenuItem = pMenuPanel->mMenu.GetItem(startCell + pMenuPanel->mScrollItemOffset + cellOffset++);
    
        if(!pMenuItem)
          return;
    
        if(pMenuItem->GetIsSeparator())
          DrawSeparator(g, *pCellRect, &pMenuPanel->mBlend);
        else
        {
          bool sel = mMouseCellBounds == pCellRect || pCellRect == pMenuPanel->mHighlightedCell;

          if(pMenuPanel->mClickedCell)
          {
            if(mState != kFlickering)
              DrawCellBackground(g, *pCellRect, pMenuItem, sel, &pMenuPanel->mBlend);
          }
          else
            DrawCellBackground(g, *pCellRect, pMenuItem, sel, &pMenuPanel->mBlend);

          DrawCellText(g, *pCellRect, pMenuItem, sel, &pMenuPanel->mBlend);
          
          if(pMenuItem->GetChecked())
            DrawTick(g, *pCellRect, pMenuItem, sel, &pMenuPanel->mBlend);
          
          if(pMenuItem->GetSubmenu())
            DrawSubMenuArrow(g, *pCellRect, pMenuItem, sel, &pMenuPanel->mBlend);
        }
      }
    }
  }
}

void IPopupMenuControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if(GetState() == kExpanded)
  {
    mMouseCellBounds = mActiveMenuPanel->HitTestCells(x, y);
    CollapseEverything();
  }
  else
    IControl::OnMouseDown(x, y, mod);
}

void IPopupMenuControl::OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
{
  mMouseCellBounds = mActiveMenuPanel->HitTestCells(x, y);
  SetDirty(false);
}

void IPopupMenuControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  mMouseCellBounds = mActiveMenuPanel->HitTestCells(x, y);
  
  // if the mouse event was outside of the active MenuPanel - could be on another menu or completely outside
  if(mMouseCellBounds == nullptr)
  {
    MenuPanel* pMousedMenuPanel = nullptr;
    
    const int nPanels = mMenuPanels.GetSize();
    
    for (auto p = nPanels-1; p >= 0; p--)
    {
      MenuPanel* pMenuPanel = mMenuPanels.Get(p);
      
      if(pMenuPanel->mShouldDraw && pMenuPanel->mRECT.Contains(x, y))
      {
        pMousedMenuPanel = pMenuPanel;
        break;
      }
    }
    
    if(pMousedMenuPanel != nullptr)
    {
      mActiveMenuPanel = pMousedMenuPanel;
      mMouseCellBounds = mActiveMenuPanel->HitTestCells(x, y);
    }
  }
  
  //  if(mMouseCellBounds != mPrevMouseCellBounds) //FIXME: shouldn't need to redraw all the time
//  {
    CalculateMenuPanels(x, y);
  
    if(mActiveMenuPanel->mScroller)
    {
      if(mMouseCellBounds == mActiveMenuPanel->mCellBounds.Get(0))
      {
        mActiveMenuPanel->ScrollUp();
      }
      else if (mMouseCellBounds == mActiveMenuPanel->mCellBounds.Get((mActiveMenuPanel->mCellBounds.GetSize()-1)))
      {
        mActiveMenuPanel->ScrollDown();
      }
    }
  
    SetDirty(false);
//  }
//  else
//    SetClean();
  
//  mPrevMouseCellBounds = mMouseCellBounds;
}

void IPopupMenuControl::OnMouseOut()
{
  mMouseCellBounds = nullptr;
}

void IPopupMenuControl::DrawPanelBackground(IGraphics& g, const IRECT& bounds, IBlend* blend)
{
  g.FillRoundRect(COLOR_WHITE, bounds, mRoundness, blend);
}

void IPopupMenuControl::DrawPanelShadow(IGraphics& g, const IRECT& bounds, IBlend* blend)
{
  g.DrawBoxShadow(bounds, mRoundness, 2., mDropShadowSize, blend);
}

void IPopupMenuControl::DrawCellBackground(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item* pItem, bool sel, IBlend* blend)
{
  if(sel)
    g.FillRect(COLOR_BLUE, bounds.GetHPadded(mPadding), blend);
}

void IPopupMenuControl::DrawCellText(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item* pItem, bool sel, IBlend* blend)
{
  IRECT tickRect = IRECT(bounds.L, bounds.T, bounds.L + TICK_SIZE, bounds.B).GetCentredInside(TICK_SIZE);
  IRECT textRect = IRECT(tickRect.R + TEXT_HPAD, bounds.T, bounds.R - TEXT_HPAD, bounds.B);
  
  if(sel)
    mText.mFGColor = COLOR_WHITE;
  else
  {
    if(pItem->GetEnabled())
      mText.mFGColor = COLOR_BLACK;
    else
      mText.mFGColor = COLOR_GRAY;
  }
  
  mText.mAlign = IText::kAlignNear;
  g.DrawText(mText, pItem->GetText(), textRect, blend);
}

void IPopupMenuControl::DrawTick(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item* pItem, bool sel, IBlend* blend)
{
  IRECT tickRect = IRECT(bounds.L, bounds.T, bounds.L + TICK_SIZE, bounds.B).GetCentredInside(TICK_SIZE);
  g.FillRoundRect(sel ? COLOR_WHITE : COLOR_BLACK, tickRect.GetCentredInside(TICK_SIZE/2.), 2, blend);
}


void IPopupMenuControl::DrawSubMenuArrow(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item* pItem, bool sel, IBlend* blend)
{
  IRECT tri = IRECT(bounds.R-ARROW_SIZE, bounds.T+2, bounds.R-2, bounds.B-2); // FIXME: triangle doesn't look good at all font sizes
  g.FillTriangle(sel ? COLOR_WHITE : COLOR_BLACK, tri.L, tri.T, tri.L, tri.B, tri.R, tri.MH(), blend);
}

void IPopupMenuControl::DrawUpArrow(IGraphics& g, const IRECT& bounds, bool sel, IBlend* blend)
{
  IRECT tri = IRECT(bounds.MW()-ARROW_SIZE, bounds.T+2, bounds.MW()+ARROW_SIZE, bounds.B-2).GetPadded(-2); // FIXME: triangle doesn't look good at all font sizes
  g.FillTriangle(sel ? COLOR_WHITE : COLOR_BLACK, tri.L, tri.B, tri.MW(), tri.T, tri.R, tri.B, blend);
}

void IPopupMenuControl::DrawDownArrow(IGraphics& g, const IRECT& bounds, bool sel, IBlend* blend)
{
  IRECT tri = IRECT(bounds.MW()-ARROW_SIZE, bounds.T+2, bounds.MW()+ARROW_SIZE, bounds.B-2).GetPadded(-2); // FIXME: triangle doesn't look good at all font sizes
  g.FillTriangle(sel ? COLOR_WHITE : COLOR_BLACK, tri.L, tri.T, tri.MW(), tri.B, tri.R, tri.T, blend);
}

void IPopupMenuControl::DrawSeparator(IGraphics& g, const IRECT& bounds, IBlend* blend)
{
  if(blend->mWeight > 0.9)
    g.FillRect(COLOR_MID_GRAY, bounds, &BLEND_25);
}

IPopupMenu* IPopupMenuControl::CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller)
{
  mMenu = &menu;
  mCaller = pCaller;
  
  Expand(bounds.L, bounds.T);
  
  return mMenu;
}

IRECT IPopupMenuControl::GetLargestCellRectForMenu(IPopupMenu& menu, float x, float y) const
{
  IRECT span;
  
  for (auto i = 0; i < menu.NItems(); ++i)
  {
    IPopupMenu::Item* pItem = menu.GetItem(i);
    IRECT textBounds;
    
    IGraphics* pGraphics =  const_cast<IPopupMenuControl*>(this)->GetUI();
    
    pGraphics->MeasureText(mText, pItem->GetText(), textBounds);
    span = span.Union(textBounds);
  }
  
  span.HPad(TEXT_HPAD); // add some padding because we don't want to be flush to the edges
  span.Pad(-TICK_SIZE, 0, ARROW_SIZE, 0);
  
  return IRECT(x, y, x + span.W(), y + span.H());
}

void IPopupMenuControl::CalculateMenuPanels(float x, float y)
{
  for(auto i = 0; i < mActiveMenuPanel->mCellBounds.GetSize(); i++)
  {
    IRECT* pCellRect = mActiveMenuPanel->mCellBounds.Get(i);
    IPopupMenu::Item* pMenuItem = mActiveMenuPanel->mMenu.GetItem(i);
    IPopupMenu* pSubMenu = pMenuItem->GetSubmenu();
    
    if(pCellRect == mMouseCellBounds)
    {
      if(pSubMenu != nullptr)
      {
        MenuPanel* pMenuPanelForThisMenu = nullptr;
        
        for (auto mr = 0; mr < mMenuPanels.GetSize(); mr++)
        {
          MenuPanel* pMenuPanel = mMenuPanels.Get(mr);
          
          if(&pMenuPanel->mMenu == pSubMenu)
          {
            pMenuPanelForThisMenu = pMenuPanel;
            pMenuPanel->mShouldDraw = true;
          }
          else
            pMenuPanel->mShouldDraw = false;
        }
      
        if(pMenuItem->GetEnabled())
          mActiveMenuPanel->mHighlightedCell = pCellRect;
        else
          mActiveMenuPanel->mHighlightedCell = nullptr;
      
        // There is no MenuPanel for this menu, make a new one
        if(pMenuPanelForThisMenu == nullptr) {
          pMenuPanelForThisMenu = mMenuPanels.Add(new MenuPanel(*this, GetUI()->GetBounds(), *pSubMenu, pCellRect->R + mPadding, pCellRect->T, mMenuPanels.Find(mActiveMenuPanel)));
        }
        
        for (auto mr = 0; mr < mMenuPanels.GetSize(); mr++)
        {
          MenuPanel* pMenuPanel = mMenuPanels.Get(mr);
          
          if(pMenuPanel->mShouldDraw)
          {
            IRECT drawRECT = pMenuPanel->mRECT;
            IRECT targetRECT = pMenuPanel->mTargetRECT;
            
            MenuPanel* pParentMenuPanel = mMenuPanels.Get(pMenuPanel->mParentIdx);
            
            while (pParentMenuPanel != nullptr)
            {
              pParentMenuPanel->mShouldDraw = true;
              drawRECT = drawRECT.Union(pParentMenuPanel->mTargetRECT);
              targetRECT = targetRECT.Union(pParentMenuPanel->mRECT);
              pParentMenuPanel = mMenuPanels.Get(pParentMenuPanel->mParentIdx);
            }
            
            SetTargetRECT(mTargetRECT.Union(targetRECT));
            SetRECT(mRECT.Union(drawRECT));
            
            if(mAppearingMenuPanel != pMenuPanel)
            {
              mState = kSubMenuAppearing;
              mAppearingMenuPanel = pMenuPanelForThisMenu;
            }
            
            SetDirty(true);
            break; // STOP LOOP!
            
          }
        }
      }
      else // (pSubMenu == nullptr)
      {
        for (auto mr = 0; mr < mMenuPanels.GetSize(); mr++)
        {
          MenuPanel* pMenuPanel = mMenuPanels.Get(mr);
          
          if(pMenuPanel->mParentIdx == mMenuPanels.Find(mActiveMenuPanel))
          {
            mActiveMenuPanel->mHighlightedCell = nullptr;
            pMenuPanel->mShouldDraw = false;
          }
        }
      }
    }
  }
  
  SetDirty(false);
}

void IPopupMenuControl::Expand(float x, float y)
{
  mState = kExpanding;
  GetUI()->UpdateTooltips(); //will disable
  
  mMenuPanels.Empty(true);
  mActiveMenuPanel = mAppearingMenuPanel = mMenuPanels.Add(new MenuPanel(*this, GetUI()->GetBounds(), *mMenu, x, y, -1));

  SetTargetRECT(mActiveMenuPanel->mTargetRECT);
  SetRECT(mActiveMenuPanel->mRECT);

  SetDirty(true); // triggers animation
}

void IPopupMenuControl::CollapseEverything()
{
  IPopupMenu* pClickedMenu = &mActiveMenuPanel->mMenu;
  
  pClickedMenu->SetChosenItemIdx(-1);

  for(auto i = 0; i < mActiveMenuPanel->mCellBounds.GetSize(); i++)
  {
    IRECT* pR = mActiveMenuPanel->mCellBounds.Get(i);
    
    if(mMouseCellBounds == pR)
    {
      int itemChosen = mActiveMenuPanel->mScrollItemOffset + i;
      IPopupMenu::Item* pItem = pClickedMenu->GetItem(itemChosen);

      if(pItem->GetIsChoosable())
      {
        pClickedMenu->SetChosenItemIdx(itemChosen);
        mActiveMenuPanel->mClickedCell = pR;
      }
    }
  }
  
  if(pClickedMenu->GetFunction())
    pClickedMenu->ExecFunction();
  
  if(mCaller)
    mCaller->OnPopupMenuSelection(pClickedMenu); // TODO: In the synchronous pop-up menu handlers it's possible for mMenu to be null, that should also be possible here if nothing was selected
  
  mActiveMenuPanel = nullptr;

  mState = kFlickering;
  SetDirty(true); // triggers animation
}

void IPopupMenuControl::OnEndAnimation()
{
//  DBGMSG("state %i\n", mState);
  
  if(mState == kExpanding)
  {
    for (auto i = 0; i < mMenuPanels.GetSize(); i++) {
      mMenuPanels.Get(i)->mBlend.mWeight = mOpacity;
    }

    mState = kExpanded;
  }
  else if (mState == kFlickering)
  {
    mState = kCollapsing;
    SetDirty(true); // triggers animation again
    return; // don't cancel animation
  }
  else if (mState == kSubMenuAppearing)
  {
    for (auto i = 0; i < mMenuPanels.GetSize(); i++) {
      mMenuPanels.Get(i)->mBlend.mWeight = mOpacity;
    }
    
    mState = kExpanded;
  }
  else if(mState == kCollapsing)
  {
    mTargetRECT = mSpecifiedCollapsedBounds;
    
    for (auto i = 0; i < mMenuPanels.GetSize(); i++) {
      mMenuPanels.Get(i)->mBlend.mWeight = 0.;
    }
    
    mState = kIdling;
    mMouseCellBounds = nullptr;
    
    SetDirty(true); // triggers animation again
    return; // don't cancel animation
  }
  else if(mState == kIdling)
  {
    GetUI()->UpdateTooltips(); // will enable the tooltips
    
    mMenuPanels.Empty(true);
    mRECT = mSpecifiedCollapsedBounds;
    mState = kCollapsed;
  }
  
  IControl::OnEndAnimation();
}

IPopupMenuControl::MenuPanel::MenuPanel(IPopupMenuControl& control, const IRECT& contextBounds, IPopupMenu& menu, float x, float y, int parentIdx)
: mMenu(menu)
, mParentIdx(parentIdx)
{
  mSingleCellBounds = control.GetLargestCellRectForMenu(menu, x, y);
  
  float left = x + control.mPadding;
  float top = y + control.mPadding;
  
  // cell height can change depending on if cell is a separator or not
  auto GetIncrements = [&](IPopupMenu::Item* pMenuItem, float& incX, float& incY)
  {
    incX = CellWidth();

    if (pMenuItem->GetIsSeparator())
      incY = control.mSeparatorSize;
    else
      incY = CellHeight();
  };
  
  for (auto i = 0; i < menu.NItems(); ++i)
  {
    IPopupMenu::Item* pMenuItem = menu.GetItem(i);
    float right, bottom;
    float toAddX, toAddY; // the increments, different depending on item type
    bool newColumn = false;
    
    GetIncrements(pMenuItem, toAddX, toAddY);
    
    if(control.mMaxColumnItems > 0 && i > 1)
      newColumn = !(i % control.mMaxColumnItems);
    
    if((top + toAddY + control.mPadding) > contextBounds.B || newColumn) // it's gonna go off the bottom
    {
      if(control.mScrollIfTooBig)
      {
        const float maxTop = contextBounds.T + control.mPadding + control.mDropShadowSize;
        const float maxBottom = contextBounds.B - control.mPadding;// - control.mDropShadowSize;
        const float maxH = (maxBottom - maxTop);
        mScrollMaxRows = (maxH  / (CellHeight() + control.mCellGap)); // maximum cell rows (full height, not with separators)
        
        // clear everything added so far
        mCellBounds.Empty(true);
        GetIncrements(menu.GetItem(0), toAddX, toAddY);
        
        if(menu.NItems() < mScrollMaxRows)
        {
          top = maxTop; // FIXME: menus that will go off the bottom, but have fewer rows than mScrollMaxRows, should not start at the top of the screen
//            top = (y + owner.mPadding + CellHeight()) - (menu.NItems() * CellHeight());
          for (auto r = 0; r < menu.NItems(); r++)
          {
            GetIncrements(menu.GetItem(r), toAddX, toAddY);
            bottom = top + toAddY;
            right = left + toAddX;
            mCellBounds.Add(new IRECT(left, top, right, bottom));
            top = bottom + control.mCellGap;
            bottom = top + toAddY;
          }
        }
        else
        {
          mScroller = true;
          top = maxTop;
          
          for (auto r = 0; r < mScrollMaxRows; r++)
          {
            GetIncrements(menu.GetItem(r), toAddX, toAddY);
            bottom = top + toAddY;
            right = left + toAddX;
            mCellBounds.Add(new IRECT(left, top, right, bottom));
            top = bottom + control.mCellGap;
            bottom = top + toAddY;
          }
        }
        break;
      }
      else // new column
      {
        left += mSingleCellBounds.W() + control.mCellGap;
        top = mSingleCellBounds.T + control.mPadding;
      }
    }
    
    right = left + toAddX;
    bottom = top + toAddY;
    
    mCellBounds.Add(new IRECT(left, top, right, bottom));
    top = bottom + control.mCellGap;
  }
  
  IRECT span;
  
  if(mCellBounds.GetSize())
  {
    span = *mCellBounds.Get(0);
    
    for(auto i = 1; i < mCellBounds.GetSize(); i++)
    {
      span = span.Union(*mCellBounds.Get(i));
    }
  }
  
  const float maxR = (contextBounds.R - control.mPadding - control.mDropShadowSize);
  
  // check if it's gone off the right hand side
  if(span.R > maxR)
  {
    const float shiftLeft = span.R-maxR;
    
    // shift all cell rects left
    for(auto i = 0; i < mCellBounds.GetSize(); i++)
    {
      mCellBounds.Get(i)->Shift(-shiftLeft, 0, -shiftLeft, 0);
    }
    
    // recalculate span
    span = *mCellBounds.Get(0);
    
    for(auto i = 1; i < mCellBounds.GetSize(); i++)
    {
      span = span.Union(*mCellBounds.Get(i));
    }
    
    //FIXME: this shift is not quite working
  }
  
  //    if (mSpecifiedExpandedBounds.W())
  //    {
  //      mTargetRECT = mSpecifiedExpandedBounds.GetPadded(mPadding); // pad the unioned cell rects)
  //      mRECT = mSpecifiedExpandedBounds.GetPadded(mDropShadowSize + mPadding);
  //    }
  
  mTargetRECT = span.GetPadded(control.mPadding); // pad the unioned cell rects)
  mRECT = span.GetPadded(control.mDropShadowSize + control.mPadding);
}

IPopupMenuControl::MenuPanel::~MenuPanel()
{
  mCellBounds.Empty(true);
}

IRECT* IPopupMenuControl::MenuPanel::HitTestCells(float x, float y) const
{
  for(auto i = 0; i < mCellBounds.GetSize(); i++)
  {
    IRECT* pR = mCellBounds.Get(i);
    if(pR->Contains(x, y) && mMenu.GetItem(i)->GetEnabled())
      return pR;
  }
  return nullptr;
}
