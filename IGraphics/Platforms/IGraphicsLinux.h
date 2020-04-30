/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IGraphics_select.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** IGraphics platform class for linux
*   @ingroup PlatformClasses
*/
class IGraphicsLinux final : public IGRAPHICS_DRAW_CLASS
{
public:
  IGraphicsLinux(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  virtual ~IGraphicsLinux);

  void* OpenWindow(void* pWindow) override;
  void CloseWindow() override;
  bool WindowIsOpen() override;
  void PlatformResize(bool parentHasResized) override;

  void HideMouseCursor() override;
  void ShowMouseCursor() override;

  EMsgBoxResult ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler) override;
  void ForceEndUserEdit() override;

  const char* GetPlatformAPIStr() override;

  void UpdateTooltips() override;

  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;
  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action,  const char* ext, IColorPickerHandlerFunc func) override;
  bool PromptForColor(IColor& color, const char* str) override;

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;

  static int GetUserOSVersion();
  bool GetTextFromClipboard(WDL_String& str) override;
  bool SetTextInClipboard(const char* str) override { return false; } // TODO

protected:
  IPopupMenu* CreatePlatformPopupMenu(const IPopupMenu& menu, IRECT& bounds) override;
  void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) override;
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
