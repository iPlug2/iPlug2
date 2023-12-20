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
 * @copydoc IVTabbedPagesControl
 */

#include "IControls.h"
#include "IBubbleControl.h"

#include <map>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A control used as the base class for a tabbed page of subcontrols
 * @ingroup IControls */
class IVTabPage : public IContainerBase, public IVectorBase
{
public:
  using TabAttachFunc = std::function<void(IVTabPage* pParent, const IRECT& bounds)>;
  static constexpr double kDefaultPadding = 10.0;
  
  static void DefaultResizeFunc(IContainerBase* pTab, const IRECT& r) {
    if (pTab->NChildren() == 1)
    {
      auto innerBounds = r.GetPadded(- pTab->As<IVTabPage>()->GetPadding());
      pTab->GetChild(0)->SetTargetAndDrawRECTs(innerBounds);
    }
  }

  /** Constructor
   * @param attachFunc A function to call when the page is attached to a tabbed control, that should add subcontrols to the page
   * Note: in the AttachFunc, the bounds you pass to the subcontrol constructor are not used (so you can just pass IRECT()), the resizeFunc will be called
   * which is what is used to position the subcontrols
   * @param resizeFunc A function to call when the control is resized
   * @param style The style. It is not actually used by the base class, but is passed to the children
   * @param padding The padding that should be applied to the children of this tab */
  IVTabPage(TabAttachFunc attachFunc = nullptr, ResizeFunc resizeFunc = DefaultResizeFunc, const IVStyle& style = DEFAULT_STYLE, double padding = kDefaultPadding)
  : IContainerBase(IRECT(), [attachFunc](IContainerBase* pParent, const IRECT& bounds){
    attachFunc(pParent->As<IVTabPage>(), bounds);
  }, resizeFunc)
  , IVectorBase(style)
  , mPadding(padding)
  {
    AttachIControl(this, "");
  }
  
  virtual void Draw(IGraphics &g) override
  {
    /* NO-OP */
  }
  
  double GetPadding() const { return mPadding; }
  
  void SetPadding(double padding) { mPadding = padding; }

  void OnStyleChanged() override
  {
    ForAllChildrenFunc([this](int childIdx, IControl* pChild) {
      if (auto pVectorBase = pChild->As<IVectorBase>())
      {
        pVectorBase->SetStyle(GetStyle());
      }
    });
  }
  
private:
  double mPadding = 0.0;
};

using PageMap = std::map<const char*, IVTabPage*>;

/** A control to manage tabbed pages of sub controls
 * Basic usage example:
 * \code{.cpp}
 * pGraphics->AttachControl(new IVTabbedPagesControl(nextCell(),
 * {
 *   {"1", new IVTabPage([](IVTabPage* pPage, const IRECT& r) {
 *     pPage->AddChildControl(new IPanelControl(IRECT(), COLOR_RED));
 *   })},
 *   {"2", new IVTabPage([](IVTabPage* pPage, const IRECT& r) {
 *     pPage->AddChildControl(new IPanelControl(IRECT(), COLOR_GREEN));
 *   })},
 *   {"3", new IVTabPage([](IVTabPage* pPage, const IRECT& r) {
 *     pPage->AddChildControl(new IPanelControl(IRECT(), COLOR_BLUE));
 *   })}
 * }, "Tabbed Pages Demo"));
 * \endcode
 * 
 * Laying out subcontrols on each page is done by passing a ResizeFunc lambda to the IVTabPage constructor, 
 * which is called when the page is attached to the IVTabbedPagesControl. The default implementation of this
 * function will resize the single child of the IVTabPage to fit the page bounds, but you can override this
 * @ingroup IControls */
class IVTabbedPagesControl : public IContainerBase, public IVectorBase
{
public:
  IVTabbedPagesControl(const IRECT& bounds, const PageMap& pages, const char* label = "",
                       const IVStyle& style = DEFAULT_STYLE, float tabBarHeight = 20.0f,
                       float tabBarFrac = 0.5f, EAlign tabsAlign = EAlign::Near)
  : IContainerBase(bounds)
  , IVectorBase(style.WithDrawFrame(false).WithDrawShadows(false))
  , mTabBarHeight(tabBarHeight)
  , mTabBarFrac(tabBarFrac)
  , mTabsAlign(tabsAlign)
  {
    AttachIControl(this, label);
    
    for (auto& page : pages)
    {
      AddPage(page.first, page.second);
    }
  }
  
  ~IVTabbedPagesControl()
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
      ForAllPagesFunc([&](IVTabPage* pPage) {
        pPage->Hide(true);
      });
      
      // Show the tabs and first page
      GetTabSwitchControl()->Hide(false);
      GetPage(GetTabSwitchControl()->GetSelectedIdx())->Hide(false);
    }
    
    IControl::Hide(hide);
  }
  
  void Draw(IGraphics& g) override
  {
    DrawLabel(g);
    
    auto rcr = GetRoundedCornerRadius(GetTabBarArea());
  
    auto rcrForTabCorners = mTabBarFrac == 1.0f ? 0.0f : rcr;
    
    g.FillRoundRect(GetColor(kPR), GetPageArea(), mTabsAlign == EAlign::Near ? 0.0f : rcrForTabCorners,
                                                  mTabsAlign == EAlign::Far ? 0.0f : rcrForTabCorners,
                                                  rcr, rcr);
    
    if (mStyle.drawFrame)
      g.DrawRoundRect(GetColor(kFR), mRECT, rcr);
  }

  void OnAttached() override
  {    
    // Set up tabs
    AddChildControl(new IVTabSwitchControl(GetTabBarArea(),
    [&](IControl* pCaller) {
      ShowSelectedPage();
    }, mPageNames, "", GetStyle().WithWidgetFrac(1.0f)));
    
    GetTabSwitchControl()->SetShape(EVShape::EndsRounded);
    
    // Add all pages, set their bounds and hide them
    ForAllPagesFunc([&](IVTabPage* pPage) {
      AddChildControl(pPage);
      pPage->SetTargetAndDrawRECTs(GetPageArea());
      pPage->Hide(true);
    });
    
    // Show the tabs and first page
    GetTabSwitchControl()->Hide(false);
    GetPage(0)->Hide(false);
  }
  
  void OnStyleChanged() override
  {
    ForAllChildrenFunc([this](int childIdx, IControl* pChild) {
      if (auto pVectorBase = pChild->As<IVectorBase>())
      {
        pVectorBase->SetStyle(GetStyle());
      }
    });
    
    auto adjustedStyle = GetStyle().WithDrawFrame(false).WithDrawShadows(false);
    GetTabSwitchControl()->SetStyle(adjustedStyle.WithWidgetFrac(1.0));
    GetTabSwitchControl()->SetShape(EVShape::EndsRounded);
  }
  
  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    
    if (NChildren())
    {
      GetTabSwitchControl()->SetTargetAndDrawRECTs(GetTabBarArea());
      
      ForAllPagesFunc([&](IVTabPage* pPage) {
        pPage->SetTargetAndDrawRECTs(GetPageArea());
      });
    }
  }
  
  float GetTabHeight() const { return mTabBarHeight; }

  IRECT GetPageArea() const { return mWidgetBounds.GetReducedFromTop(GetTabHeight()); }

  IRECT GetTabBarArea() const
  {
    return mWidgetBounds.GetFromTop(GetTabHeight()).FracRectHorizontal(mTabBarFrac, mTabsAlign == EAlign::Far);
  }
  
  int NPages() const { return mPages.GetSize(); }

private:
  void AddPage(const char* pageName, IVTabPage* pPage)
  {
    pPage->SetLabelStr(pageName);
    mPageNames.push_back(pageName);
    mPages.Add(pPage);
  }
  
  void ForAllPagesFunc(std::function<void(IVTabPage* pControl)> func)
  {
    for (int i=0; i<mPages.GetSize(); i++)
    {
      func(mPages.Get(i));
    }
  }
  
  IVTabSwitchControl* GetTabSwitchControl() { return GetChild(0)->As<IVTabSwitchControl>(); }
  
  IVTabPage* GetPage(int pageIdx) { return mPages.Get(pageIdx); }

  void SelectPage(int index)
  {
    GetTabSwitchControl()->SetValue(static_cast<double>(index));
    ShowSelectedPage();
  }
  
private:
  void ShowSelectedPage()
  {
    ForAllPagesFunc([&](IVTabPage* pPage) {
      bool hide = strcmp(GetTabSwitchControl()->GetSelectedLabelStr(),
                                         pPage->GetLabelStr());
      pPage->Hide(hide);
    });
    
    if (IBubbleControl* pControl = GetUI()->GetBubbleControl())
    {
      pControl->Hide(true);
    }
  }
  
  WDL_PtrList<IVTabPage> mPages;
  std::vector<const char*> mPageNames;
  float mTabBarHeight;
  float mTabBarFrac;
  EAlign mTabsAlign;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
