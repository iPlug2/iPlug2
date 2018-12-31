/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

#include "IGraphics_select.h"

/** IGraphics platform class for Windows
* @ingroup PlatformClasses */
class IGraphicsWin final : public IGRAPHICS_DRAW_CLASS
{
public:
  IGraphicsWin(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsWin();

  void SetWinModuleHandle(void* pInstance) override { mHInstance = (HINSTANCE) pInstance; }
  void* GetWinModuleHandle() override { return mHInstance; }

  void ForceEndUserEdit() override;

  void PlatformResize() override;

  void CheckTabletInput(UINT msg);
    
  void HideMouseCursor(bool hide, bool lock) override;
  void MoveMouseCursor(float x, float y) override;
  void SetMouseCursor(ECursor cursor) override;

  int ShowMessageBox(const char* str, const char* caption, EMessageBoxType type) override;

  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  bool WindowIsOpen() override { return (mPlugWnd); }

  void UpdateTooltips() override {}

  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;
  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext) override;
  void PromptForDirectory(WDL_String& dir) override;
  bool PromptForColor(IColor& color, const char* str) override;

  IPopupMenu* GetItemMenu(long idx, long& idxInMenu, long& offsetIdx, const IPopupMenu& baseMenu);
  HMENU CreateMenu(IPopupMenu& menu, long* pOffsetIdx);

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure);

  void* GetWindow() override { return mPlugWnd; }
  HWND GetParentWindow() const { return mParentWnd; }
  HWND GetMainWnd();
  void SetMainWndClassName(const char* name) { mMainWndClassName.Set(name); }
//  void GetMainWndClassName(char* name) { strcpy(name, mMainWndClassName.Get()); }
  IRECT GetWindowRECT();
  void SetWindowTitle(const char* str);

  const char* GetPlatformAPIStr() override { return "win32"; };

  bool GetTextFromClipboard(WDL_String& str) override;

  EResourceLocation OSFindResource(const char* name, const char* type, WDL_String& result) override;

  const void* LoadWinResource(const char* resid, const char* resType, int& sizeInBytes) override;

protected:
  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller) override;
  void CreatePlatformTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str) override;

  void SetTooltip(const char* tooltip);
  void ShowTooltip();
  void HideTooltip();

private:
  enum EParamEditMsg
  {
    kNone,
    kEditing,
    kUpdate,
    kCancel,
    kCommit
  };

  inline IMouseInfo IGraphicsWin::GetMouseInfo(LPARAM lParam, WPARAM wParam);
  inline IMouseInfo IGraphicsWin::GetMouseInfoDeltas(float&dX, float& dY, LPARAM lParam, WPARAM wParam);
  bool MouseCursorIsLocked();

  HINSTANCE mHInstance = nullptr;
  HWND mPlugWnd = nullptr;
  HWND mParamEditWnd = nullptr;
  HWND mTooltipWnd = nullptr;
  HWND mParentWnd = nullptr;
  HWND mMainWnd = nullptr;
  COLORREF* mCustomColorStorage = nullptr;
  WNDPROC mDefEditProc = nullptr;
  DWORD mPID = 0;

  IControl* mEdControl = nullptr;
  EParamEditMsg mParamEditMsg = kNone;
  bool mShowingTooltip = false;
  float mHiddenCursorX;
  float mHiddenCursorY;
  int mTooltipIdx = -1;

  WDL_String mMainWndClassName;
public:
  static BOOL EnumResNameProc(HANDLE module, LPCTSTR type, LPTSTR name, LONG_PTR param);
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK ParamEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static BOOL CALLBACK FindMainWindow(HWND hWnd, LPARAM lParam);
};
