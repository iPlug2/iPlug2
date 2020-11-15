/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestFlexBoxControl
 */

#include "IControl.h"
#include "IGraphicsFlexBox.h"

static const std::initializer_list<const char*> alignStrs = { "YGAlignAuto", "YGAlignFlexStart", "YGAlignCenter", "YGAlignFlexEnd", "YGAlignStretch", "YGAlignBaseline", "YGAlignSpaceBetween", "YGAlignSpaceAround" };
static const std::initializer_list<const char*> dirStrs = { "YGFlexDirectionColumn", "YGFlexDirectionColumnReverse", "YGFlexDirectionRow", "YGFlexDirectionRowReverse" };
static const std::initializer_list<const char*> justifyStrs = { "YGJustifyFlexStart", "YGJustifyCenter", "YGJustifyFlexEnd", "YGJustifySpaceBetween", "YGJustifySpaceAround", "YGJustifySpaceEvenly" };
static const std::initializer_list<const char*> wrapStrs = { "YGWrapNoWrap", "YGWrapWrap", "YGWrapWrapReverse" };

/** Control to test IGraphicsFlexBox
 *   @ingroup TestControls */
class TestFlexBoxControl : public IControl
{
public:
  TestFlexBoxControl(const IRECT& rect)
  : IControl(rect)
  {
    mMenu.GetItem(0)->SetSubmenu(new IPopupMenu("Align", alignStrs));
    mMenu.GetItem(1)->SetSubmenu(new IPopupMenu("Direction", dirStrs));
    mMenu.GetItem(2)->SetSubmenu(new IPopupMenu("Justify", justifyStrs));
    mMenu.GetItem(3)->SetSubmenu(new IPopupMenu("Wrap", wrapStrs));

    mText = IText(70.f, EVAlign::Middle);
    SetTooltip("TestFlexboxControl");
  }
  
  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_WHITE, mRootRect);

    WDL_String str;
    for (int i=0; i<7; i++)
    {
      g.FillRect(GetRainbow(i), mItemRects[i]);
      g.DrawRect(COLOR_BLACK, mItemRects[i]);
      str.SetFormatted(2, "%i", i);
      g.DrawText(mText, str.Get(), mItemRects[i]);
    }

    IRECT textRect;
    g.MeasureText(mText.WithSize(14.f), mSettingsStr.Get(), textRect);
    IRECT adjusted = mRECT.GetCentredInside(textRect);
    g.FillRect(COLOR_WHITE.WithOpacity(0.5f), adjusted);
    g.DrawRect(COLOR_BLACK, adjusted);
    g.DrawText(mText.WithSize(14.f), mSettingsStr.Get(), adjusted);
  }
  
  void OnResize() override
  {
    DoLayout();
  }
  
  void DoLayout()
  {
    mItemRects.clear();
    
    for (int i=0; i<7; i++)
    {
      mItemRects.push_back(IRECT());
    }
    
    IFlexBox f;
    f.Init(mRECT, YGFlexDirection(mDirection), YGJustify(mJustify), YGWrap(mWrap));

    YGNodeRef r;
    
    for (int i=0; i<7; i++)
    {
      r = f.AddItem(100.f, 100.f, YGAlign(mAlign));
    }

    f.CalcLayout();

    for (int i=0; i<7; i++)
    {
      mItemRects[i] = mRECT.Inset(f.GetItemBounds(i));
    }

    mRootRect = mRECT.Inset(f.GetRootBounds());
    mSettingsStr.SetFormatted(256, "%s, %s, %s, %s", alignStrs.begin()[mAlign], dirStrs.begin()[mDirection], justifyStrs.begin()[mJustify], wrapStrs.begin()[mWrap]);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mMenu.GetItem(0)->GetSubmenu()->CheckItemAlone(mAlign);
    mMenu.GetItem(1)->GetSubmenu()->CheckItemAlone(mDirection);
    mMenu.GetItem(2)->GetSubmenu()->CheckItemAlone(mJustify);
    mMenu.GetItem(3)->GetSubmenu()->CheckItemAlone(mWrap);
    
    GetUI()->CreatePopupMenu(*this, mMenu, x, y);
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int) override
  {
    if(pSelectedMenu) 
    {
      int idx = pSelectedMenu->GetChosenItemIdx();
      if     (strcmp(pSelectedMenu->GetRootTitle(), "Align") == 0) { mAlign = idx; }
      else if(strcmp(pSelectedMenu->GetRootTitle(), "Direction") == 0) { mDirection = idx; }
      else if(strcmp(pSelectedMenu->GetRootTitle(), "Justify") == 0) { mJustify = idx; }
      else if(strcmp(pSelectedMenu->GetRootTitle(), "Wrap") == 0) { mWrap = idx; }
    }

    DoLayout();
    SetDirty(false);
  }
  
private:
  WDL_String mSettingsStr;
  IPopupMenu mMenu {"FlexBox", {"Align", "Direction", "Justify", "Wrap"}};
  IRECT mRootRect;
  int mAlign = YGAlignAuto;
  int mDirection = YGFlexDirectionColumn;
  int mJustify = YGJustifyFlexStart;
  int mWrap = YGWrapNoWrap;
  std::vector<IRECT> mItemRects;
};
