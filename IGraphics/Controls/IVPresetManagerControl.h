/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IVDiskPresetManagerControl
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
    PresetMenu,
    LoadButton
  };
  
  IVBakedPresetManagerControl(const IRECT& bounds, const IVStyle& style = DEFAULT_STYLE)
  : IContainerBase(bounds)
  , IVectorBase(style)
  {
    AttachIControl(this, "");
    mIgnoreMouse = true;
  }
  
  void Draw(IGraphics& g) override { /* NO-OP */ }
  
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
      
      for (int i = 0; i < nPresets; i++) {
        const char* str = pluginBase->GetPresetName(i);
        if (i == currentPresetIdx)
          mMenu.AddItem(str, -1, IPopupMenu::Item::kChecked);
        else
          mMenu.AddItem(str);
      }
      
      pCaller->GetUI()->CreatePopupMenu(*this, mMenu, pCaller->GetRECT());
    };

    AddChildControl(new IVButtonControl(GetSubControlBounds(ESubControl::LeftButton), SplashClickActionFunc, "<", mStyle))
    ->SetAnimationEndActionFunction(prevPresetFunc);
    AddChildControl(new IVButtonControl(GetSubControlBounds(ESubControl::RightButton), SplashClickActionFunc, ">", mStyle))
    ->SetAnimationEndActionFunction(nextPresetFunc);
//   AddChildControl(new IVButtonControl(IRECT(), SplashClickActionFunc, "Load", mStyle))->SetAnimationEndActionFunction(loadPresetFunc);
    AddChildControl(mPresetNameButton = new IVButtonControl(GetSubControlBounds(ESubControl::PresetMenu), SplashClickActionFunc, "Choose Preset...", mStyle))->SetAnimationEndActionFunction(choosePresetFunc);
  }
  
  void OnResize() override
  {
    ForAllChildrenFunc([&](int childIdx, IControl* pChild){
      pChild->SetTargetAndDrawRECTs(GetSubControlBounds((ESubControl) childIdx));
    });
  }
  
private:
  IRECT GetSubControlBounds(ESubControl control)
  {
    auto sections = mRECT;
    
    std::array<IRECT, 4> rects = {
      sections.ReduceFromLeft(50),
      sections.ReduceFromLeft(50),
//      sections.ReduceFromRight(50),
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
{
public:
  IVDiskPresetManagerControl(const IRECT& bounds, const char* presetPath, const char* fileExtension, bool showFileExtensions = true, const IVStyle& style = DEFAULT_STYLE)
  : IDirBrowseControlBase(bounds, fileExtension, showFileExtensions)
  , mStyle(style)
  {
    mIgnoreMouse = true;
    AddPath(presetPath, "");
    SetupMenu();
  }
  
  void Draw(IGraphics& g) override { /* NO-OP */ }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (pSelectedMenu)
    {
      IPopupMenu::Item* pItem = pSelectedMenu->GetChosenItem();

      if (pItem)
      {
        mSelectedIndex = mItems.Find(pItem);
        LoadPresetAtCurrentIndex();
      }
    }
  }

  void OnAttached() override
  {
    IRECT sections = mRECT.GetPadded(-5.f);

    auto prevPresetFunc = [&](IControl* pCaller) {
      mSelectedIndex--;

      if (mSelectedIndex < 0)
        mSelectedIndex = NItems() - 1;

      LoadPresetAtCurrentIndex();
    };

    auto nextPresetFunc = [&](IControl* pCaller) {
      mSelectedIndex++;

      if (mSelectedIndex >= NItems())
        mSelectedIndex = 0;

      LoadPresetAtCurrentIndex();
    };

    auto loadPresetFunc = [&](IControl* pCaller) {
      WDL_String fileName;
      WDL_String path;
      pCaller->GetUI()->PromptForFile(fileName, path, EFileAction::Open, mExtension.Get());

      if (fileName.GetLength())
        mPresetNameButton->SetLabelStr(fileName.Get());
      
      SetSelectedFile(fileName.Get());
      LoadPresetAtCurrentIndex();
    };

    auto choosePresetFunc = [&](IControl* pCaller) {
      CheckSelectedItem();
      mMainMenu.SetChosenItemIdx(mSelectedIndex);
      pCaller->GetUI()->CreatePopupMenu(*this, mMainMenu, pCaller->GetRECT());
    };

    AddChildControl(new IVButtonControl(sections.ReduceFromLeft(50), SplashClickActionFunc, "<", mStyle))->SetAnimationEndActionFunction(prevPresetFunc);
    AddChildControl(new IVButtonControl(sections.ReduceFromLeft(50), SplashClickActionFunc, ">", mStyle))->SetAnimationEndActionFunction(nextPresetFunc);
    AddChildControl(new IVButtonControl(sections.ReduceFromRight(100), SplashClickActionFunc, "Load", mStyle))->SetAnimationEndActionFunction(loadPresetFunc);
    AddChildControl(mPresetNameButton = new IVButtonControl(sections, SplashClickActionFunc, "Choose Preset...", mStyle))->SetAnimationEndActionFunction(choosePresetFunc);  
  }

  void LoadPresetAtCurrentIndex()
  {
    if (mSelectedIndex > -1 && mSelectedIndex < mItems.GetSize())
    {
      WDL_String fileName, path;
      GetSelectedFile(fileName);
      mPresetNameButton->SetLabelStr(fileName.Get());
    }
  }

private:
  IVButtonControl* mPresetNameButton = nullptr;
  IVStyle mStyle;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE


