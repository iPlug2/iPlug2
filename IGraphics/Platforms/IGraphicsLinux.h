/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IGraphics_select.h"
#include "PlatformX11.hpp"
#include "IPlugTaskThread.h"
#include <memory>
#include <mutex.h>
#include <functional>
#include <vector>

BEGIN_IPLUG_NAMESPACE
class Timer;

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
  void* GetWindow() override { return mWindow->GetHandle(); }
  bool WindowIsOpen() override { return mWindow != nullptr; }
  void PlatformResize(bool parentHasResized) override;
  void GetMouseLocation(float& x, float& y) const override;
  void HideMouseCursor(bool hide, bool lock) override;
  void MoveMouseCursor(float x, float y) override;
  EMsgBoxResult ShowMessageBox(const char* str, const char* title, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler) override;
  void ForceEndUserEdit() override { /* NO-OP */ }
  void DrawResize() override;
  const char* GetPlatformAPIStr() override { return "Linux"; }
  void UpdateTooltips() override { /* NO-OP */ };

  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;
  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler) override;
  void PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler) override;
  bool PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func) override;

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;
  static int GetUserOSVersion();
  bool GetTextFromClipboard(WDL_String& str) override;
  bool SetTextInClipboard(const char* str) override;

  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, void* pData, int dataSize) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) override;

  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) override { } // No reason to cache (no universal font handle)

  /// @see iplug::igraphics::IGraphics::UpdateUI
  void UpdateUI() override;

protected:
  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync) override;
  virtual void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) override;
  void RequestFocus();

  /// @brief Add a task to be run in the UI thread
  void AddUiTask(std::function<void()>&& task);

  friend class IGraphics;
private:
  /// @brief Window pointer
  X11Window* mWindow = NULL;
  /// @brief timestamp when we should re-render,
  uint64_t mNextDrawTime;
  /// @brief If true, then we should re-paint at the end of the call to Update().
  bool mShouldPaint = false;
  /// @brief Locked mouse position
  IVec2 mMouseLockPos;
  /// @brief List of tasks for the UI thread to run
  ThreadSafeCallList<> mUiTasks;
  /// @brief Id for IPlugTaskThread task that calls UpdateUI
  uint32_t mTaskId = 0;

  /** Loop through the events provided by the window. */
  void LoopEvents();

  void Paint();

  static uint32_t GetUserDblClickTimeout();
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
