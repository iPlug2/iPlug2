/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IVPresetManagerControl
 */

#include "IControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A "meta control" for a "preset manager" for disk-based preset files
 * It adds several child buttons 
 * @ingroup IControls */
class IVPresetManagerControl : public IDirBrowseControlBase
{
public:
  IVPresetManagerControl(const IRECT& bounds, const char* presetPath, const char* fileExtension, const IVStyle& style = DEFAULT_STYLE)
  : IDirBrowseControlBase(bounds, fileExtension)
  , mStyle(style)
  {
    mIgnoreMouse = true;
    AddPath(presetPath, "");
    SetupMenu();
  }
  
  void Draw(IGraphics& g) override
  {
  }

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
    };

    auto choosePresetFunc = [&](IControl* pCaller) { pCaller->GetUI()->CreatePopupMenu(*this, mMainMenu, pCaller->GetRECT()); };

    GetUI()->AttachControl(new IVButtonControl(sections.ReduceFromLeft(50), SplashClickActionFunc, "<", mStyle))->SetAnimationEndActionFunction(prevPresetFunc);
    GetUI()->AttachControl(new IVButtonControl(sections.ReduceFromLeft(50), SplashClickActionFunc, ">", mStyle))->SetAnimationEndActionFunction(nextPresetFunc);
    GetUI()->AttachControl(new IVButtonControl(sections.ReduceFromRight(100), SplashClickActionFunc, "Load", mStyle))->SetAnimationEndActionFunction(loadPresetFunc);
    GetUI()->AttachControl(mPresetNameButton = new IVButtonControl(sections, SplashClickActionFunc, "Preset...", mStyle))->SetAnimationEndActionFunction(choosePresetFunc);
  }

  void LoadPresetAtCurrentIndex()
  {
    if (mSelectedIndex > -1 && mSelectedIndex < mItems.GetSize())
    {
      IPopupMenu::Item* pItem = mItems.Get(mSelectedIndex);

      //if (PLUG()->LoadProgramFromVSTPreset(mFiles.Get(pItem->GetTag())->Get()))
      //{
      //  PLUG()->ModifyCurrentPreset(PLUG()->GetPatchName());
      //  PLUG()->InformHostOfProgramChange();
        mPresetNameButton->SetLabelStr(pItem->GetText());
      //}
    }
  }

private:
  IVButtonControl* mPresetNameButton = nullptr;
  IVStyle mStyle;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE