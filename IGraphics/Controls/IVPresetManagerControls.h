/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup IControls
 * Includes meta controls for basic preset managers
 */

#include "IControl.h"

#include "IPlugPluginBase.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A "meta control" for a "preset manager" for "baked in" factory presets
 * It adds several child buttons
 * @ingroup IControls */
class IVBakedPresetManagerControl : public IContainerBase
                                  , public IVectorBase
{
public:
  enum class ESubControl
  {
    LeftButton = 0,
    RightButton,
    PresetMenu
  };
  
  IVBakedPresetManagerControl(const IRECT& bounds, const char* label = "", 
                              const IVStyle& style = DEFAULT_STYLE.WithDrawShadows(false).WithLabelText(DEFAULT_LABEL_TEXT.WithVAlign(EVAlign::Middle)))
  : IContainerBase(bounds)
  , IVectorBase(style)
  {
    AttachIControl(this, label);
    mIgnoreMouse = true;
  }
  
  void Draw(IGraphics& g) override 
  {
    DrawLabel(g);
  }
  
  void RestorePreset(IPluginBase* pluginBase, int presetIdx)
  {
    pluginBase->RestorePreset(presetIdx);

    WDL_String str;
    str.SetFormatted(32, "Preset %i: %s", presetIdx + 1, pluginBase->GetPresetName(presetIdx));
    mPresetNameButton->SetLabelStr(str.Get());
  }
  
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (pSelectedMenu)
    {
      IPopupMenu::Item* pItem = pSelectedMenu->GetChosenItem();

      if (pItem)
      {
        IPluginBase* pluginBase = dynamic_cast<IPluginBase*>(GetDelegate());
        RestorePreset(pluginBase, pSelectedMenu->GetChosenItemIdx());
      }
    }
  }

  void OnAttached() override
  {
    auto prevPresetFunc = [&](IControl* pCaller) {
      IPluginBase* pluginBase = dynamic_cast<IPluginBase*>(pCaller->GetDelegate());

      int presetIdx = pluginBase->GetCurrentPresetIdx();
      int nPresets = pluginBase->NPresets();

      presetIdx--;
      
      if (presetIdx < 0)
        presetIdx = nPresets - 1;
      
      RestorePreset(pluginBase, presetIdx);
    };

    auto nextPresetFunc = [&](IControl* pCaller) {
      IPluginBase* pluginBase = dynamic_cast<IPluginBase*>(pCaller->GetDelegate());

      int presetIdx = pluginBase->GetCurrentPresetIdx();
      int nPresets = pluginBase->NPresets();

      presetIdx++;

      if (presetIdx >= nPresets)
        presetIdx = 0;

      RestorePreset(pluginBase, presetIdx);
    };

    auto choosePresetFunc = [&](IControl* pCaller) {
      mMenu.Clear();
      
      IPluginBase* pluginBase = dynamic_cast<IPluginBase*>(pCaller->GetDelegate());

      int currentPresetIdx = pluginBase->GetCurrentPresetIdx();
      int nPresets = pluginBase->NPresets();
      
      for (int i = 0; i < nPresets; i++) 
      {
        const char* str = pluginBase->GetPresetName(i);
        if (i == currentPresetIdx)
          mMenu.AddItem(str, -1, IPopupMenu::Item::kChecked);
        else
          mMenu.AddItem(str);
      }
      
      pCaller->GetUI()->CreatePopupMenu(*this, mMenu, pCaller->GetRECT());
    };

    AddChildControl(new IVButtonControl(IRECT(), SplashClickActionFunc, "<", mStyle))
    ->SetAnimationEndActionFunction(prevPresetFunc);
    AddChildControl(new IVButtonControl(IRECT(), SplashClickActionFunc, ">", mStyle))
    ->SetAnimationEndActionFunction(nextPresetFunc);
    AddChildControl(mPresetNameButton = new IVButtonControl(IRECT(), SplashClickActionFunc, "Choose Preset...", mStyle))->SetAnimationEndActionFunction(choosePresetFunc);
    
    OnResize();
  }
  
  void OnResize() override
  {
    MakeRects(mRECT);
    
    ForAllChildrenFunc([&](int childIdx, IControl* pChild){
      pChild->SetTargetAndDrawRECTs(GetSubControlBounds((ESubControl) childIdx));
    });
  }
  
private:
  IRECT GetSubControlBounds(ESubControl control)
  {
    auto sections = mWidgetBounds;
    
    std::array<IRECT, 3> rects = {
      sections.ReduceFromLeft(50),
      sections.ReduceFromLeft(50),
      sections
    };
    
    return rects[(int) control];
  }
  
  
  IPopupMenu mMenu;
  IVButtonControl* mPresetNameButton = nullptr;
};

/** A "meta control" for a "preset manager" for disk-based preset files
 * It adds several child buttons 
 * @ingroup IControls */
class IVDiskPresetManagerControl : public IDirBrowseControlBase
                                 , public IVectorBase
{
public:
  enum class ESubControl
  {
    LeftButton = 0,
    RightButton,
    PresetMenu,
    LoadButton
  };

  IVDiskPresetManagerControl(const IRECT& bounds, const char* presetPath, const char* fileExtension, bool showFileExtensions = true, const char* label = "", const IVStyle& style = DEFAULT_STYLE.WithDrawShadows(false).WithLabelText(DEFAULT_LABEL_TEXT.WithVAlign(EVAlign::Middle)))
  : IDirBrowseControlBase(bounds, fileExtension, showFileExtensions)
  , IVectorBase(style)
  {
    AttachIControl(this, label);
    mIgnoreMouse = true;
    AddPath(presetPath, "");
    SetupMenu();
  }
  
  void Draw(IGraphics& g) override 
  {
    DrawLabel(g);
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (pSelectedMenu)
    {
      IPopupMenu::Item* pItem = pSelectedMenu->GetChosenItem();

      if (pItem)
      {
        mSelectedItemIndex = mItems.Find(pItem);
        LoadPresetAtCurrentIndex();
      }
    }
  }

  void OnAttached() override
  {
    auto prevPresetFunc = [&](IControl* pCaller) {
      if (NItems())
      {
        mSelectedItemIndex--;
        
        if (mSelectedItemIndex < 0)
          mSelectedItemIndex = NItems() - 1;
        
        LoadPresetAtCurrentIndex();
      }
    };

    auto nextPresetFunc = [&](IControl* pCaller) {
      if (NItems())
      {
        mSelectedItemIndex++;
        
        if (mSelectedItemIndex >= NItems())
          mSelectedItemIndex = 0;
        
        LoadPresetAtCurrentIndex();
      }
    };

    auto loadPresetFunc = [&](IControl* pCaller) {
      WDL_String fileName, path;
      pCaller->GetUI()->PromptForFile(fileName, path, EFileAction::Open, mExtension.Get());

      if (path.GetLength())
      {
        ClearPathList();
        AddPath(path.Get(), "");
        SetupMenu();
      }

      if (fileName.GetLength())
      {
        SetSelectedFile(fileName.Get());
        LoadPresetAtCurrentIndex();
      }
    };

    auto choosePresetFunc = [&](IControl* pCaller) {
      CheckSelectedItem();
      pCaller->GetUI()->CreatePopupMenu(*this, mMainMenu, pCaller->GetRECT());
    };

    AddChildControl(new IVButtonControl(IRECT(), SplashClickActionFunc, "<", mStyle))->SetAnimationEndActionFunction(prevPresetFunc);
    AddChildControl(new IVButtonControl(IRECT(), SplashClickActionFunc, ">", mStyle))->SetAnimationEndActionFunction(nextPresetFunc);
    AddChildControl(new IVButtonControl(IRECT(), SplashClickActionFunc, "Load", mStyle))->SetAnimationEndActionFunction(loadPresetFunc);
    AddChildControl(mPresetNameButton = new IVButtonControl(IRECT(), SplashClickActionFunc, "Choose Preset...", mStyle))->SetAnimationEndActionFunction(choosePresetFunc);

    OnResize();
  }
  
  void OnResize() override
  {
    MakeRects(mRECT);

    ForAllChildrenFunc([&](int childIdx, IControl* pChild){
      pChild->SetTargetAndDrawRECTs(GetSubControlBounds((ESubControl) childIdx));
    });
  }

  void LoadPresetAtCurrentIndex()
  {
    if (mSelectedItemIndex > -1 && 
        mSelectedItemIndex < mItems.GetSize())
    {
      WDL_String fileName;
      GetSelectedFile(fileName);
      if (!mShowFileExtensions)
      {
        fileName.remove_fileext();
      }
      mPresetNameButton->SetLabelStr(fileName.get_filepart());

      // If it's just a single folder, we can set the
      // chosen item index to mSelectedItemIndex
      // in order to pop up the menu at the
      // correct location
      if (!mMainMenu.HasSubMenus())
      {
        mMainMenu.SetChosenItemIdx(mSelectedItemIndex);
      }
    }
  }

private:
  IRECT GetSubControlBounds(ESubControl control)
  {
    auto sections = mWidgetBounds;
    
    std::array<IRECT, 4> rects = {
      sections.ReduceFromLeft(50),
      sections.ReduceFromLeft(50),
      sections.ReduceFromRight(50),
      sections,
    };
    
    return rects[(int) control];
  }

  IVButtonControl* mPresetNameButton = nullptr;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

