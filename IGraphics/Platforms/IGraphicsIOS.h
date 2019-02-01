/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IGraphics_select.h"

/** IGraphics platform class for IOS
*   @ingroup PlatformClasses */
class IGraphicsIOS final : public IGRAPHICS_DRAW_CLASS
{
public:
  IGraphicsIOS(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  virtual ~IGraphicsIOS();
  
  void SetBundleID(const char* bundleID) { mBundleID.Set(bundleID); }

  void* OpenWindow(void* pWindow) override;
  void CloseWindow() override;
  bool WindowIsOpen() override;
  void PlatformResize() override;

  int ShowMessageBox(const char* str, const char* caption, EMessageBoxType type) override;
  void ForceEndUserEdit() override;

  const char* GetPlatformAPIStr() override;

  void UpdateTooltips() override {};

  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext) override;
  void PromptForDirectory(WDL_String& dir) override;
  bool PromptForColor(IColor& color, const char* str) override;
  
  void HideMouseCursor(bool hide, bool lock) override {}; // NOOP
  void MoveMouseCursor(float x, float y) override {}; // NOOP
  
  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;
  
  void* GetWindow() override;
  
  const char* GetBundleID() override { return mBundleID.Get(); }
  static int GetUserOSVersion();
  
  bool GetTextFromClipboard(WDL_String& str) override;

protected:
  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller) override;
  void CreatePlatformTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str) override;
  EResourceLocation OSFindResource(const char* name, const char* type, WDL_String& result) override;
  bool GetResourcePathFromBundle(const char* fileName, const char* searchExt, WDL_String& fullPath);
  bool GetResourcePathFromUsersMusicFolder(const char* fileName, const char* searchExt, WDL_String& fullPath);

private:
  void* mView = nullptr;
  WDL_String mBundleID;
};
