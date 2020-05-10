/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IGraphics_select.h"

#include <xcbt.h>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** IGraphics platform class for linux
*   @ingroup PlatformClasses
*/
class IGraphicsLinux final : public IGRAPHICS_DRAW_CLASS
{
  class Font;
public:
  IGraphicsLinux(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  virtual ~IGraphicsLinux();

  void* OpenWindow(void* pWindow) override;

  void CloseWindow() override;

  void* GetWindow() override { return (void *)(intptr_t)mPlugWnd; }

  bool WindowIsOpen() override { return (mPlugWnd); }

  void PlatformResize(bool parentHasResized) override;

  void HideMouseCursor(bool hide, bool lock) override {} // TODO
  void MoveMouseCursor(float x, float y) override {} // TODO

  EMsgBoxResult ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler) override;
  void ForceEndUserEdit() override {} // TODO

  void DrawResize() override;

  const char* GetPlatformAPIStr() override { return "Linux"; }

  void UpdateTooltips() override {}; // TODO

  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override { return false; } // TODO
  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* extensions) override;
  void PromptForDirectory(WDL_String& dir) override;

  bool PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func) override { return false; } // TODO

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override { return false; } // TODO

  static int GetUserOSVersion();
  bool GetTextFromClipboard(WDL_String& str) override { return false; } // TODO
  bool SetTextInClipboard(const char* str) override { return false; } // TODO

  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) override;

  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) override { } // No reason to cache (no universal font handle)

  void SetIntegration(void *mainLoop) override;

protected:
  enum EParamEditMsg
  {
    kNone,
    kEditing,
    kUpdate,
    kCancel,
    kCommit
  };

  void SetTooltip(const char* tooltip) {} // TODO
  void ShowTooltip() {} // TODO
  void HideTooltip() {} // TODO

  IPopupMenu *CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, bool& isAsync) override { return nullptr; } // TODO
  void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) override { } // TODO

  virtual void GetMouseLocation(float& x, float&y) const
  {
    //TODO:
  }
  
private:

  xcbt         mX = NULL;
  xcbt_embed  *mEmbed = NULL;
  
  xcbt_window  mPlugWnd = NULL;
  xcbt_window_handler mBaseWindowHandler;
  void               *mBaseWindowData;

  xcb_timestamp_t     mLastLeftClickStamp; // it will be not zero in case there is a chance for double click

  void Paint();
  inline IMouseInfo GetMouseInfo(int16_t x, int16_t y, int16_t state);
  inline IMouseInfo GetMouseInfoDeltas(float& dX, float& dY, int16_t x, int16_t y, int16_t state);
  void WindowHandler(xcb_generic_event_t *evt);
  void TimerHandler(int id);

  static void WindowHandlerProxy(xcbt_window xw, xcb_generic_event_t *evt, IGraphicsLinux *_this) {
    _this->WindowHandler(evt);
  }
  static void TimerHandlerProxy(xcbt x, int timer_id, IGraphicsLinux *_this) {
    _this->TimerHandler(timer_id);
  }
  
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
