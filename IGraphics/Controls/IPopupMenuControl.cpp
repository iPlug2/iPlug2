/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

/**
 * @file
 * @brief IPopupMenuControl implementation
 * @ingroup SpecialControls
 */

#include "IPopupMenuControl.h"

// TODO: drop shadow on non-nanovg backends too slow
#ifndef ENABLE_SHADOW
  #define ENABLE_SHADOW 0
#endif

#ifdef IGRAPHICS_NANOVG
#include "nanovg.h"
#endif

using namespace iplug;
using namespace igraphics;

IPopupMenuControl::IPopupMenuControl(int paramIdx, IText text, IRECT collapsedBounds, IRECT expandedBounds)
: IControl(collapsedBounds, paramIdx)
, mSpecifiedCollapsedBounds(collapsedBounds)
, mSpecifiedExpandedBounds(expandedBounds)
{
  SetActionFunction([&](IControl* pCaller) {
    
    int duration = DEFAULT_ANIMATION_DURATION;
    
    if(mState == kSubMenuAppearing)
      duration = DEFAULT_ANIMATION_DURATION * 2;
    
#pragma mark animations
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
  mHide = true;
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
      DrawPanelShadow(g, pMenuPanel);
      DrawPanelBackground(g, pMenuPanel); 
      
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

          //TODO: Title indent?
          DrawCellText(g, *pCellRect, pMenuItem, sel, &pMenuPanel->mBlend);
          
          if(pMenuItem->GetChecked())
            DrawTick(g, *pCellRect, pMenuItem, sel, &pMenuPanel->mBlend);
          
          if(pMenuItem->GetSubmenu())
            DrawSubMenuArrow(g, *pCellRect, pMenuItem, sel, &pMenuPanel->mBlend);
        }
      }
    }
  }
  
  if(mCallOut && mMenuPanels.GetSize())
    DrawCalloutArrow(g, mCalloutArrowBounds, &mMenuPanels.Get(0)->mBlend);
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
  if(mActiveMenuPanel)
  {
    mMouseCellBounds = mActiveMenuPanel->HitTestCells(x, y);
    SetDirty(false);
  }
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

void IPopupMenuControl::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
{
  //FIXME:
//  if(mActiveMenuPanel)
//  {
//    if(mActiveMenuPanel->mScroller)
//    {
//      if(d > 0.)
//        mActiveMenuPanel->ScrollUp();
//      else
//        mActiveMenuPanel->ScrollDown();
//    }
//
//    SetDirty(false);
//  }
}

void IPopupMenuControl::DrawCalloutArrow(IGraphics& g, const IRECT& bounds, IBlend* pBlend)
{
  switch (mCalloutArrowDir) {
    case kNorth:
      g.FillTriangle(COLOR_WHITE, bounds.MW(), bounds.T, bounds.L, bounds.B, bounds.R, bounds.B, pBlend);
      break;
    case kEast:
      g.FillTriangle(COLOR_WHITE, bounds.L, bounds.MH(), bounds.R, bounds.T, bounds.R, bounds.B, pBlend);
      break;
    case kSouth:
      g.FillTriangle(COLOR_WHITE, bounds.MW(), bounds.B, bounds.L, bounds.T, bounds.R, bounds.T, pBlend);
      break;
    case kWest:
      g.FillTriangle(COLOR_WHITE, bounds.R, bounds.MH(), bounds.L, bounds.T, bounds.L, bounds.B, pBlend);
      break;
    default:
      break;
  }
}

void IPopupMenuControl::DrawPanelBackground(IGraphics& g, MenuPanel* panel)
{
  // mTargetRECT = inner area
  g.FillRoundRect(COLOR_WHITE, panel->mTargetRECT, mRoundness, &panel->mBlend);
}

void IPopupMenuControl::DrawPanelShadow(IGraphics& g, MenuPanel* panel)
{
  const float yDrop = 2.0;
  IRECT inner = panel->mRECT.GetPadded(-mDropShadowSize);
    
#ifdef IGRAPHICS_NANOVG
  auto NanoVGColor = [](const IColor& color, const IBlend* pBlend = nullptr) {
    NVGcolor c;
    c.r = (float)color.R / 255.0f;
    c.g = (float)color.G / 255.0f;
    c.b = (float)color.B / 255.0f;
    c.a = (BlendWeight(pBlend) * color.A) / 255.0f;
    return c;
  };

  NVGcontext* vg = (NVGcontext*) g.GetDrawContext();
  NVGpaint shadowPaint = nvgBoxGradient(vg, inner.L, inner.T + yDrop, inner.W(), inner.H(), mRoundness * 2.f, 20.f, NanoVGColor(COLOR_BLACK_DROP_SHADOW, &panel->mBlend), NanoVGColor(COLOR_TRANSPARENT, nullptr));
  nvgBeginPath(vg);
  nvgRect(vg, panel->mRECT.L, panel->mRECT.T, panel->mRECT.W(), panel->mRECT.H());
  nvgFillPaint(vg, shadowPaint);
  nvgGlobalCompositeOperation(vg, NVG_SOURCE_OVER);
  nvgFill(vg);
  nvgBeginPath(vg); // Clear the paths
#elif ENABLE_SHADOW
  if (!g.CheckLayer(panel->mShadowLayer))
  {
    g.StartLayer(this, panel->mRECT);
    g.FillRoundRect(COLOR_BLACK, inner, mRoundness);
    panel->mShadowLayer = g.EndLayer();
    g.ApplyLayerDropShadow(panel->mShadowLayer, IShadow(COLOR_BLACK_DROP_SHADOW, 20.0, 0.0, yDrop, 1.0, true));
  }
  g.DrawLayer(panel->mShadowLayer, &panel->mBlend);
#endif
}

void IPopupMenuControl::DrawCellBackground(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item* pItem, bool sel, IBlend* pBlend)
{
  if(sel)
    g.FillRect(COLOR_BLUE, bounds.GetHPadded(PAD), pBlend);
}

void IPopupMenuControl::DrawCellText(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item* pItem, bool sel, IBlend* pBlend)
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
  
  mText.mAlign = EAlign::Near;
  g.DrawText(mText, pItem->GetText(), textRect, pBlend);
}

void IPopupMenuControl::DrawTick(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item* pItem, bool sel, IBlend* pBlend)
{
  IRECT tickRect = IRECT(bounds.L, bounds.T, bounds.L + TICK_SIZE, bounds.B).GetCentredInside(TICK_SIZE);
  g.FillRoundRect(sel ? COLOR_WHITE : COLOR_BLACK, tickRect.GetCentredInside(TICK_SIZE/2.f), 2, pBlend);
}

void IPopupMenuControl::DrawSubMenuArrow(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item* pItem, bool sel, IBlend* pBlend)
{
  IRECT tri = IRECT(bounds.R-ARROW_SIZE, bounds.T+2, bounds.R-2, bounds.B-2); // FIXME: triangle doesn't look good at all font sizes
  g.FillTriangle(sel ? COLOR_WHITE : COLOR_BLACK, tri.L, tri.T, tri.L, tri.B, tri.R, tri.MH(), pBlend);
}

void IPopupMenuControl::DrawUpArrow(IGraphics& g, const IRECT& bounds, bool sel, IBlend* pBlend)
{
  IRECT tri = IRECT(bounds.MW()-ARROW_SIZE, bounds.T+2, bounds.MW()+ARROW_SIZE, bounds.B-2).GetPadded(-2); // FIXME: triangle doesn't look good at all font sizes
  g.FillTriangle(sel ? COLOR_WHITE : COLOR_BLACK, tri.L, tri.B, tri.MW(), tri.T, tri.R, tri.B, pBlend);
}

void IPopupMenuControl::DrawDownArrow(IGraphics& g, const IRECT& bounds, bool sel, IBlend* pBlend)
{
  IRECT tri = IRECT(bounds.MW()-ARROW_SIZE, bounds.T+2, bounds.MW()+ARROW_SIZE, bounds.B-2).GetPadded(-2); // FIXME: triangle doesn't look good at all font sizes
  g.FillTriangle(sel ? COLOR_WHITE : COLOR_BLACK, tri.L, tri.T, tri.MW(), tri.B, tri.R, tri.T, pBlend);
}

void IPopupMenuControl::DrawSeparator(IGraphics& g, const IRECT& bounds, IBlend* pBlend)
{
  if(pBlend->mWeight > 0.9)
    g.FillRect(COLOR_MID_GRAY, bounds, &BLEND_25);
}

void IPopupMenuControl::CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds)
{
  mMenu = &menu;
  
  if(mMaxBounds.W() == 0)
    mMaxBounds = GetUI()->GetBounds();
    
  if(GetState() == kCollapsed)
    Expand(bounds);
}

IRECT IPopupMenuControl::GetLargestCellRectForMenu(IPopupMenu& menu, float x, float y) const
{
  IRECT span;
  
  for (auto i = 0; i < menu.NItems(); ++i)
  {
    IPopupMenu::Item* pItem = menu.GetItem(i);
    IRECT textBounds;
    
    const IGraphics* pGraphics = GetUI();
    
    pGraphics->MeasureText(mText, pItem->GetText(), textBounds);
    span = span.Union(textBounds);
  }
  
  span.HPad(TEXT_HPAD); // add some padding because we don't want to be flush to the edges
  span.Pad(TICK_SIZE, 0, ARROW_SIZE, 0);
  
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
          pMenuPanelForThisMenu = mMenuPanels.Add(new MenuPanel(*this, *pSubMenu, pCellRect->R + PAD, pCellRect->T, mMenuPanels.Find(mActiveMenuPanel)));
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

void IPopupMenuControl::Expand(const IRECT& anchorArea)
{
  Hide(false);
  mState = kExpanding;
  GetUI()->UpdateTooltips(); //will disable
  
  mMenuPanels.Empty(true);
  
  mAnchorArea = anchorArea;
  
  float x = anchorArea.L;
  float y = anchorArea.B;
  
  if(mCallOut)
  {
    x = anchorArea.R + CALLOUT_SPACE;
    y = anchorArea.MH() - CALLOUT_SPACE - mText.mSize;
    mCalloutArrowBounds = IRECT(anchorArea.R, anchorArea.MH() - CALLOUT_SPACE, x, anchorArea.MH() + CALLOUT_SPACE);
    mCalloutArrowDir = kEast;
    
    if(y < (mMaxBounds.T+PAD)) // if we're going off the top of the max anchorArea
    {
      IRECT maxCell = GetLargestCellRectForMenu(*mMenu, 0, 0);
      
      x = anchorArea.MW() - (maxCell.W() / 2.f);
      y = anchorArea.B + CALLOUT_SPACE;
      mCalloutArrowBounds = IRECT(anchorArea.MW() - CALLOUT_SPACE, anchorArea.B, anchorArea.MW() + CALLOUT_SPACE, anchorArea.B + CALLOUT_SPACE);
      mCalloutArrowDir = kNorth;
    }
  }
  
  mActiveMenuPanel = mAppearingMenuPanel = mMenuPanels.Add(new MenuPanel(*this, *mMenu, x, y, -1));

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
  
  GetUI()->SetControlValueAfterPopupMenu(pClickedMenu);
    
  mActiveMenuPanel = nullptr;

  mState = kFlickering;
  Hide(true);
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
    mAnchorArea = IRECT();
    
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

IPopupMenuControl::MenuPanel::MenuPanel(IPopupMenuControl& control, IPopupMenu& menu, float x, float y, int parentIdx)
: mMenu(menu)
, mParentIdx(parentIdx)
{
  mSingleCellBounds = control.GetLargestCellRectForMenu(menu, x, y);
  
  float left = x + control.PAD;
  float top = y + control.PAD;
  
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
    
    if((top + toAddY + control.PAD) > control.mMaxBounds.B || newColumn) // it's gonna go off the bottom
    {
      if(control.mScrollIfTooBig)
      {
        const float maxTop = control.mMaxBounds.T + control.PAD + control.mDropShadowSize;
        const float maxBottom = control.mMaxBounds.B - control.PAD;// - control.mDropShadowSize;
        const float maxH = (maxBottom - maxTop);
        mScrollMaxRows = static_cast<int>(maxH  / (CellHeight() + control.mCellGap)); // maximum cell rows (full height, not with separators)
        
        // clear everything added so far
        mCellBounds.Empty(true);
        GetIncrements(menu.GetItem(0), toAddX, toAddY);
        
        if(menu.NItems() < mScrollMaxRows)
        {
          top = (y + control.PAD + CellHeight()) - (menu.NItems() * CellHeight());
          
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
        top = mSingleCellBounds.T + control.PAD;
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
  
  const float maxR = (control.mMaxBounds.R - control.PAD - control.mDropShadowSize);
  
  // check if it's gone off the right hand side
  if(span.R > maxR)
  {
    if(control.mCallOut)
    {
      const float newRight = control.mAnchorArea.L - control.CALLOUT_SPACE - control.PAD;
      const float shiftLeft = span.R-newRight;

      for(auto i = 0; i < mCellBounds.GetSize(); i++)
      {
        mCellBounds.Get(i)->Translate(-shiftLeft, 0.f);
      }
      
      control.mCalloutArrowDir = kWest;
      control.mCalloutArrowBounds = IRECT(control.mAnchorArea.L - control.CALLOUT_SPACE, control.mAnchorArea.MH() - control.CALLOUT_SPACE, control.mAnchorArea.L, control.mAnchorArea.MH() + control.CALLOUT_SPACE);
    }
    else
    {
      const float shiftLeft = span.R-maxR;

      // shift all cell rects left
      for(auto i = 0; i < mCellBounds.GetSize(); i++)
      {
        mCellBounds.Get(i)->Translate(-shiftLeft, 0.f);
      }
    }
    
    // recalculate span
    span = *mCellBounds.Get(0);
    
    for(auto i = 1; i < mCellBounds.GetSize(); i++)
    {
      span = span.Union(*mCellBounds.Get(i));
    }
    
    //FIXME: this shift is not quite working
  }
  
  if (control.mSpecifiedExpandedBounds.W())
  {
    mTargetRECT = control.mSpecifiedExpandedBounds.GetPadded(control.PAD); // pad the unioned cell rects)
    mRECT = control.mSpecifiedExpandedBounds.GetPadded(control.mDropShadowSize + control.PAD);
  }
  else
  {
    mTargetRECT = span.GetPadded(control.PAD); // pad the unioned cell rects)
    mRECT = span.GetPadded(control.mDropShadowSize + control.PAD);
  }
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
