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
  {
    DrawCalloutArrow(g, mCalloutArrowBounds, &mMenuPanels.Get(0)->mBlend);
    if (mMenuHasSubmenu && mSubMenuOpened)
    {
      DrawSubMenuCalloutArrow(g, mSubMenuCalloutArrowBounds, &mMenuPanels.Get(0)->mBlend);
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
  float trisize = bounds.H();
  float halftri = trisize * 0.5f;
  float ax, ay, bx, by, cx, cy;
  
  switch (mCalloutArrowDir) {
    case kNorth:
      ax = bounds.MW() - halftri;
      ay = bounds.MH() - halftri;
      bx = ax + trisize;
      by = ay;
      cx = bounds.MW();
      cy = ay + trisize;
      break;
    case kEast:
      ax = bounds.MW() + halftri;
      ay = bounds.MH() + halftri;
      bx = ax;
      by = ay - trisize;
      cx = ax - trisize;
      cy = bounds.MH();
      break;
    case kSouth:
      ax = bounds.MW() - halftri;
      ay = bounds.MH() + halftri;
      bx = ax + trisize;
      by = ay;
      cx = bounds.MW();
      cy = ay - trisize;
      break;
    case kWest:
      ax = bounds.MW() - halftri;
      ay = bounds.MH() + halftri;
      bx = ax;
      by = ay - trisize;
      cx = ax + trisize;
      cy = bounds.MH();
      break;
    default:
      break;
  }
  g.FillTriangle(mPanelBackgroundColor, ax, ay, bx, by, cx, cy, pBlend);
}

void IPopupMenuControl::DrawSubMenuCalloutArrow(IGraphics& g, const IRECT& bounds, IBlend* pBlend)
{
  float trisize = bounds.H();
  float halftri = trisize * 0.5f;
  float ax, ay, bx, by, cx, cy;
  
  if (mSubmenuOnRight)
  {
    ax = bounds.MW() + halftri;
    ay = bounds.MH() + halftri;
    bx = ax;
    by = ay - trisize;
    cx = ax - trisize;
    cy = bounds.MH();
  }
  else
  {
    ax = bounds.MW() - halftri;
    ay = bounds.MH() + halftri;
    bx = ax;
    by = ay - trisize;
    cx = ax + trisize;
    cy = bounds.MH();
  }
  g.FillTriangle(mPanelBackgroundColor, ax, ay, bx, by, cx, cy, pBlend);
}

void IPopupMenuControl::DrawPanelBackground(IGraphics& g, MenuPanel* panel)
{
  // mTargetRECT = inner area
  g.FillRoundRect(mPanelBackgroundColor, panel->mTargetRECT, mRoundness, &panel->mBlend);
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
    g.FillRect(mCellBackGroundColor, bounds.GetHPadded(PAD), pBlend);
}

void IPopupMenuControl::DrawCellText(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item* pItem, bool sel, IBlend* pBlend)
{
  IRECT tickRect = IRECT(bounds.L, bounds.T, bounds.L + TICK_SIZE, bounds.B).GetCentredInside(TICK_SIZE);
  IRECT textRect = IRECT(tickRect.R + TEXT_HPAD, bounds.T, bounds.R - TEXT_HPAD, bounds.B);
  
  if(sel)
    mText.mFGColor = mItemMouseoverColor;
  else
  {
    if(pItem->GetEnabled())
      mText.mFGColor = mItemColor;
    else
      mText.mFGColor = mDisabledItemColor;
  }
  
  mText.mAlign = EAlign::Near;
  g.DrawText(mText, pItem->GetText(), textRect, pBlend);
}

void IPopupMenuControl::DrawTick(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item* pItem, bool sel, IBlend* pBlend)
{
  IRECT tickRect = IRECT(bounds.L, bounds.T, bounds.L + TICK_SIZE, bounds.B).GetCentredInside(TICK_SIZE);
  g.FillRoundRect(sel ? mItemMouseoverColor : mItemColor, tickRect.GetCentredInside(TICK_SIZE/2.f), 2, pBlend);
}

void IPopupMenuControl::DrawSubMenuArrow(IGraphics& g, const IRECT& bounds, const IPopupMenu::Item* pItem, bool sel, IBlend* pBlend)
{
  float trisize, halftri, ax, ay, bx, by, cx, cy;
  
  if (mSubmenuOnRight)
  {
    IRECT tri = IRECT(bounds.R + (PAD * 0.5f) - bounds.H(), bounds.T, bounds.R + (PAD * 0.5f), bounds.B);
    trisize = (tri.R - tri.L) * 0.5f;
    halftri = trisize * 0.5f;
    ax = tri.R - trisize;
    ay = tri.MH() + halftri;
    bx = ax;
    by = ay - trisize;
    cx = tri.R;
    cy = tri.MH();
  }
  else
  {
    IRECT tri = IRECT(bounds.L - (PAD * 0.5f), bounds.T, bounds.L - (PAD * 0.5f) + bounds.H(), bounds.B);
    trisize = (tri.R - tri.L) * 0.5f;
    halftri = trisize * 0.5f;
    ax = tri.L + trisize;
    ay = tri.MH() + halftri;
    bx = ax;
    by = ay - trisize;
    cx = tri.L;
    cy = tri.MH();
  }
  g.FillTriangle(sel ? mItemMouseoverColor : mItemColor, ax, ay, bx, by, cx, cy, pBlend);
}

void IPopupMenuControl::DrawUpArrow(IGraphics& g, const IRECT& bounds, bool sel, IBlend* pBlend)
{
  IRECT tri = IRECT(bounds.MW() - (bounds.H() * 0.5f), bounds.T, bounds.MW() + (bounds.H() * 0.5f), bounds.B);
  float trisize = (tri.R - tri.L) * 0.6f;
  float halftri = trisize * 0.5f;
  float ax = tri.MW() - halftri;
  float ay = tri.MH() + halftri;
  float bx = ax + trisize;
  float by = ay;
  float cx = tri.MW();
  float cy = ay - trisize;
  g.FillTriangle(sel ? mItemMouseoverColor : mItemColor, ax, ay, bx, by, cx, cy, pBlend);
}

void IPopupMenuControl::DrawDownArrow(IGraphics& g, const IRECT& bounds, bool sel, IBlend* pBlend)
{
  IRECT tri = IRECT(bounds.MW() - (bounds.H() * 0.5f), bounds.T, bounds.MW() + (bounds.H() * 0.5f), bounds.B);
  float trisize = (tri.R - tri.L) * 0.6f;
  float halftri = trisize * 0.5f;
  float ax = tri.MW() - halftri;
  float ay = tri.MH() - halftri;
  float bx = ax + trisize;
  float by = ay;
  float cx = tri.MW();
  float cy = ay + trisize;
  g.FillTriangle(sel ? mItemMouseoverColor : mItemColor, ax, ay, bx, by, cx, cy, pBlend);
}

void IPopupMenuControl::DrawSeparator(IGraphics& g, const IRECT& bounds, IBlend* pBlend)
{
  if(pBlend->mWeight > 0.9)
    g.FillRect(mSeparatorColor, bounds, &BLEND_25);
}

void IPopupMenuControl::CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds)
{
  mMenu = &menu;
  
  for (int i = 0; i< mMenu->NItems(); i++)
  {
    if (mMenu->GetItem(i)->GetSubmenu())
    {
      mMenuHasSubmenu = true;
      break;
    }
    else mMenuHasSubmenu = false;
  }
  
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

void IPopupMenuControl::GetPanelDimensions(IPopupMenu&menu, float& width, float& height) const
{
  IRECT maxCell = GetLargestCellRectForMenu(menu, 0, 0);
  
  int numItems = menu.NItems();
  int numSeparators = 0;
  float panelHeight = 0.f;
  
  for (auto i = 0; i < numItems; ++i)
  {
    IPopupMenu::Item* pItem = menu.GetItem(i);
    if (pItem->GetIsSeparator())
    {
      numSeparators += 1;
    }
  }
  float numCells = numItems - numSeparators;
  panelHeight = (numCells * maxCell.H()) + (numSeparators * mSeparatorSize) + ((numItems - 1) * mCellGap);
  
  width = maxCell.W();
  height = panelHeight;
}

void IPopupMenuControl::CalculateMenuPanels(float x, float y)
{
  float calloutSpace =0.f;
  
  if(mCallOut)
  {
    calloutSpace = CALLOUT_SPACE;
  }
  
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
          
          float panelWidth = 0.f;
          float panelHeight = 0.f;
          
          GetPanelDimensions(*pSubMenu, panelWidth, panelHeight);
          
          float minT = mMaxBounds.T + mDropShadowSize;
          float maxB = mMaxBounds.B - panelHeight - (mDropShadowSize * 2.f);
          float maxR = mMaxBounds.R - panelWidth - (mDropShadowSize * 2.f);
          float minL = mMaxBounds.L + mDropShadowSize;
          
          float x = 0.f;
          float y = 0.f;
          
          if (mCalloutArrowDir == kSouth)
          {
            y = pCellRect->T - PAD;
            if (y > maxB) y = maxB;
            if ( y <= minT) y = minT;
          }
          
          if (mCalloutArrowDir == kNorth)
          {
            y = (pCellRect->T - (PAD / 2.f) - panelHeight) + (mCellGap * 2.f) + mDropShadowSize;
            if ( y <= minT) y = minT;
            if (y > maxB) y = maxB;
          }
          
          if (mSubmenuOnRight) x = pCellRect->R + PAD + calloutSpace;
          else x = pCellRect->L - PAD - calloutSpace - panelWidth - mDropShadowSize;
          if ( x <= minL ) x = minL;
          if ( x > maxR ) x = maxR;
          
          pMenuPanelForThisMenu = mMenuPanels.Add(new MenuPanel(*this, *pSubMenu, x, y, mMenuPanels.Find(mActiveMenuPanel)));
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
              mSubMenuOpened = true;
              
              if (mSubmenuOnRight) mSubMenuCalloutArrowBounds = IRECT(pCellRect->R + PAD , pCellRect->MH() - (calloutSpace / 2.f) , pCellRect->R + PAD + calloutSpace, pCellRect->MH() + (calloutSpace / 2.f));
              else mSubMenuCalloutArrowBounds = IRECT(pCellRect->L - PAD - calloutSpace, pCellRect->MH() - (calloutSpace / 2.f), pCellRect->L - PAD, pCellRect->MH() + (calloutSpace / 2.f));
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
            mSubMenuOpened = false;
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
  
  float panelWidth = 0.f;
  float panelHeight = 0.f;
  
  GetPanelDimensions(*mMenu, panelWidth, panelHeight);
  
  float minT = mMaxBounds.T + mDropShadowSize;
  float maxB = mMaxBounds.B - panelHeight - (mDropShadowSize * 2.f);
  float maxR = mMaxBounds.R - panelWidth - (mDropShadowSize * 2.f);
  float minL = mMaxBounds.L + mDropShadowSize;
  
  float x = 0.f;
  float y = 0.f;
  
  float calloutSpace =0.f;
  if(mCallOut)
  {
    calloutSpace = CALLOUT_SPACE;
  }
  
  if ( anchorArea.MH() <= mMaxBounds.MH())
  {
    y = anchorArea.MH() - (calloutSpace + mText.mSize);
  }
  else y = (anchorArea.MH() - panelHeight) + (calloutSpace + mText.mSize);
  
  if ( anchorArea.MW() <= mMaxBounds.MW() )
  {
    x = anchorArea.R + calloutSpace;
    mCalloutArrowBounds = IRECT( anchorArea.R, anchorArea.MH() - (calloutSpace / 2.f), x, anchorArea.MH() + (calloutSpace / 2.f) );
    mCalloutArrowDir = kEast;
  }
  else
  {
    x = anchorArea.L - calloutSpace - panelWidth - mDropShadowSize;
    mCalloutArrowBounds = IRECT( anchorArea.L - calloutSpace, anchorArea.MH() - (calloutSpace / 2.f), anchorArea.L, anchorArea.MH() + (calloutSpace / 2.f) );
    mCalloutArrowDir = kWest;
  }
  
  if( y <= minT || y > maxB || x <= minL || x > maxR ) // if we're going off the top, right, left, or bottom
  {
    if ( (y <= minT || x <= minL || x > maxR) && anchorArea.MH() <= mMaxBounds.MH() )
    {
      x = anchorArea.MW() - (panelWidth / 2.f);
      y = anchorArea.B + calloutSpace;
      mCalloutArrowBounds = IRECT(anchorArea.MW() - (calloutSpace/2.f), anchorArea.B, anchorArea.MW() + (calloutSpace/2.f), anchorArea.B + calloutSpace);
      mCalloutArrowDir = kSouth;
      
      if ( y > maxB )
      {
        if ( anchorArea.MW() <= mMaxBounds.MW() )
        {
          x = anchorArea.R + calloutSpace;
          mCalloutArrowBounds = IRECT( anchorArea.R, anchorArea.MH() - (calloutSpace / 2.f), x, anchorArea.MH() + (calloutSpace / 2.f) );
          mCalloutArrowDir = kEast;
        }
        else
        {
          x = anchorArea.L - calloutSpace - panelWidth - mDropShadowSize;
          mCalloutArrowBounds = IRECT( anchorArea.L - calloutSpace, anchorArea.MH() - (calloutSpace / 2.f), anchorArea.L, anchorArea.MH() + (calloutSpace / 2.f) );
          mCalloutArrowDir = kWest;
        }
        y = maxB;
      }
    }
    
    if ( (y > maxB || x <= minL || x > maxR) && anchorArea.MH() > mMaxBounds.MH() )
    {
      x = anchorArea.MW() - (panelWidth / 2.f);
      y = anchorArea.T - calloutSpace - panelHeight - mDropShadowSize;
      mCalloutArrowBounds = IRECT(anchorArea.MW() - (calloutSpace/2.f), anchorArea.T - calloutSpace, anchorArea.MW() + (calloutSpace/2.f), anchorArea.T);
      mCalloutArrowDir = kNorth;
      
      if ( y <= minT )
      {
        if ( anchorArea.MW() <= mMaxBounds.MW() )
        {
          x = anchorArea.R + calloutSpace;
          mCalloutArrowBounds = IRECT( anchorArea.R, anchorArea.MH() - (calloutSpace / 2.f), x, anchorArea.MH() + (calloutSpace / 2.f) );
          mCalloutArrowDir = kEast;
        }
        else
        {
          x = anchorArea.L - calloutSpace - panelWidth - mDropShadowSize;
          mCalloutArrowBounds = IRECT( anchorArea.L - calloutSpace, anchorArea.MH() - (calloutSpace / 2.f), anchorArea.L, anchorArea.MH() + (calloutSpace / 2.f) );
          mCalloutArrowDir = kWest;
        }
        y = minT;
      }
    }
    
    if ( x <= minL ) x = minL;
    if ( x > maxR ) x = maxR;
    if ( y <= minT ) y = minT;
    if ( y > maxB ) y = maxB;
  }
  
  if (mForcedSouth)
  {
    if (anchorArea.B + calloutSpace <= maxB)
    {
      x = anchorArea.MW() - (panelWidth / 2.f);
      y = anchorArea.B + calloutSpace;
      mCalloutArrowBounds = IRECT(anchorArea.MW() - (calloutSpace/2.f), anchorArea.B, anchorArea.MW() + (calloutSpace/2.f), anchorArea.B + calloutSpace);
      mCalloutArrowDir = kSouth;
    }
    if ( x <= minL ) x = minL;
    if ( x > maxR ) x = maxR;
  }
  
  if (mMenuHasSubmenu)
  {
    float shiftfactor;
    if ( anchorArea.MW() <= mMaxBounds.MW() )
    {
      mSubmenuOnRight = true;
      shiftfactor = -1.f;
    }
    else
    {
      mSubmenuOnRight = false;
      shiftfactor = 1.f;
    }
    x = (anchorArea.MW() - (panelWidth / 2.f)) + (mMenuShift * shiftfactor);
    if ( x <= minL ) x = minL;
    if ( x > maxR ) x = maxR;
    
    if (anchorArea.T - mMaxBounds.T <= mMaxBounds.B - anchorArea.B)
    {
      y = anchorArea.B + calloutSpace;
      if ( y > maxB ) y = maxB;
      if ( y <= minT ) y = minT;
      mCalloutArrowBounds = IRECT(anchorArea.MW() - (calloutSpace/2.f), anchorArea.B, anchorArea.MW() + (calloutSpace/2.f), anchorArea.B + calloutSpace);
      mCalloutArrowDir = kSouth;
    }
    else
    {
      y = anchorArea.T - calloutSpace - panelHeight - mDropShadowSize;
      if ( y <= minT ) y = minT;
      if ( y > maxB ) y = maxB;
      mCalloutArrowBounds = IRECT(anchorArea.MW() - (calloutSpace/2.f), anchorArea.T - calloutSpace, anchorArea.MW() + (calloutSpace/2.f), anchorArea.T);
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
  
  mSubMenuOpened = false;
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
