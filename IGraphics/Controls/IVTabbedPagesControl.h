/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup Controls
 * @copydoc IVKeyboardControl
 */

#include <map>
#include "IControls.h"
#include "IBubbleControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE


/** A control used a the base class for a tabbed page of subcontrols
 * @ingroup IControls */
class IVTabbedPageBase : public IContainerBase, public IVectorBase
{
public:
  /** Constructor
   * @param attachFunc A function to call when the page is attached to a tabbed control, that should add subcontrols to the page
   * Note: in the AttachFunc, the bounds you pass to the subcontrol constructor are not used (so you can just pass IRECT()), the resizeFunc will be called
   * which is what is used to position the subcontrols
   * @param resizeFunc A function to call when the control is resized
   * @param style The style. It is not actually used by the base class */
  IVTabbedPageBase(AttachFunc attachFunc = nullptr, ResizeFunc resizeFunc = nullptr, const IVStyle& style = DEFAULT_STYLE)
  : IContainerBase(IRECT(), attachFunc, resizeFunc)
  , IVectorBase(style)
  {
    AttachIControl(this, "");
  }
  
  virtual void Draw(IGraphics &g) override
  {
    /* NO-OP */
  }
};

/** A control to manage tabbed pages of sub controls
 * @ingroup IControls */
class IVTabbedPagesControl : public IContainerBase, public IVectorBase
{
public:
  IVTabbedPagesControl(const IRECT& bounds, const std::map<const char*, IVTabbedPageBase*>& pages, const char* label = "",
                       const IVStyle& style = DEFAULT_STYLE.WithDrawFrame(false), float tabBarSize = 20.0f, float tabBarFrac = 0.5, EAlign tabsAlign = EAlign::Near)
  : IContainerBase(bounds)
  , IVectorBase(style.WithDrawShadows(false))
  , mTabBarSize(tabBarSize)
  , mTabBarFrac(tabBarFrac)
  , mTabsAlign(tabsAlign)
  {
    AttachIControl(this, label);
    
    for (auto page : pages)
    {
      AddPage(page.first, page.second);
    }
  }
  
  ~IVTabbedPagesControl() override
  {
    mPages.Empty(false);
  }
  
  void Hide(bool hide) override
  {
    if (hide)
    {
      ForAllChildrenFunc([hide](int childIdx, IControl* pChild) {
        pChild->Hide(hide);
      });
    }
    else
    {
      ForAllPagesFunc([&](IVTabbedPageBase* pPage) {
        pPage->Hide(true);
      });
      
      // Show the tabs and first page
      GetTabsControl()->Hide(false);
      GetPage(GetTabsControl()->GetSelectedIdx())->Hide(false);
    }
    
    IControl::Hide(hide);
  }
  
  void Draw(IGraphics& g) override
  {
    DrawLabel(g);
    
    float rcr = GetRoundedCornerRadius(GetTabBarArea());
  
    float rcrForTabCorners = mTabBarFrac == 1.0f ? 0.0f : rcr;
    
    g.FillRoundRect(GetColor(kPR), GetPageArea(), mTabsAlign == EAlign::Near ? 0.0f : rcrForTabCorners,
                                                  mTabsAlign == EAlign::Far ? 0.0f : rcrForTabCorners,
                                                  rcr, rcr);
    
    if (mStyle.drawFrame)
      g.DrawRoundRect(GetColor(kFR), mRECT, rcr);
  }

  void SelectPage(int index)
  {
    GetTabsControl()->SetValue(index, 0);
    ShowSelectedPage();
  }

  void OnAttached() override
  {    
    // Set up tabs
    AddChildControl(new IVTabSwitchControl(GetTabBarArea(),
      [&](IControl* pCaller) {
        ShowSelectedPage();
    }, mPageNames, "", GetStyle()));
    
    GetTabsControl()->SetShape(EVShape::EndsRounded);
    
    // Add all pages, set their bounds and hide them
    ForAllPagesFunc([&](IVTabbedPageBase* pPage) {
      AddChildControl(pPage);
      pPage->SetTargetAndDrawRECTs(GetPageArea());
      pPage->Hide(true);
    });
    
    // Show the tabs and first page
    GetTabsControl()->Hide(false);
    GetPage(0)->Hide(false);
  }
  
  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    
    if (NChildren())
    {
      GetTabsControl()->SetTargetAndDrawRECTs(GetTabBarArea());
      
      ForAllPagesFunc([&](IVTabbedPageBase* pPage) {
        pPage->SetTargetAndDrawRECTs(GetPageArea());
      });
    }
  }
  
  float GetTabBarSize() const { return mTabBarSize; }

  IRECT GetPageArea() const { return mWidgetBounds.GetReducedFromTop(GetTabBarSize()); }

  IRECT GetTabBarArea() const
  {
    return mWidgetBounds.GetFromTop(GetTabBarSize()).FracRectHorizontal(mTabBarFrac, mTabsAlign == EAlign::Far);
  }
  
  int NPages() const { return mPages.GetSize(); }

private:
  void AddPage(const char* pageName, IVTabbedPageBase* pPage)
  {
    pPage->SetLabelStr(pageName);
    mPageNames.push_back(pageName);
    mPages.Add(pPage);
  }
  
  void ForAllPagesFunc(std::function<void(IVTabbedPageBase* pControl)> func)
  {
    for (int i=0; i<mPages.GetSize(); i++)
    {
      func(mPages.Get(i));
    }
  }
  
  IVTabSwitchControl* GetTabsControl() { return GetChild(0)->As<IVTabSwitchControl>(); }
  
  IVTabbedPageBase* GetPage(int pageIdx) { return mPages.Get(pageIdx); }

  void ShowSelectedPage()
  {
    const char* selected = GetTabsControl()->GetSelectedLabelStr();
    int idx = GetTabsControl()->GetSelectedIdx();

    ForAllPagesFunc([&](IVTabbedPageBase* pPage) {
      const char* pageLabel = pPage->GetLabelStr();
      bool hideTab = strcmp(selected, pageLabel) != 0;
      pPage->Hide(hideTab);
    });

    IBubbleControl* bc = GetUI()->GetBubbleControl();
    if (bc != nullptr)
    {
      bc->Hide(true);
    }
  }

private:
  WDL_PtrList<IVTabbedPageBase> mPages;
  std::vector<const char*> mPageNames;
  float mTabBarSize;
  float mTabBarFrac;
  EAlign mTabsAlign;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
