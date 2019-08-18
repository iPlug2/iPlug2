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

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** IGraphics platform class for Windows
* @ingroup PlatformClasses */
class IGraphicsWin final : public IGRAPHICS_DRAW_CLASS
{
  class Font;
  class InstalledFont;
  struct HFontHolder;
public:
  IGraphicsWin(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsWin();

  void SetWinModuleHandle(void* pInstance) override { mHInstance = (HINSTANCE) pInstance; }
  void* GetWinModuleHandle() override { return mHInstance; }

  void ForceEndUserEdit() override;
  int GetPlatformWindowScale() const override { return GetScreenScale(); }

  void PlatformResize(bool parentHasResized) override;

#ifdef IGRAPHICS_GL
  void DrawResize() override; // overriden here to deal with GL graphics context capture
#endif

  void CheckTabletInput(UINT msg);
  void DestroyEditWindow();
    
  void HideMouseCursor(bool hide, bool lock) override;
  void MoveMouseCursor(float x, float y) override;
  ECursor SetMouseCursor(ECursor cursorType) override;

  EMsgBoxResult ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler) override;

  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  bool WindowIsOpen() override { return (mPlugWnd); }

  void UpdateTooltips() override {}

  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;
  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext) override;
  void PromptForDirectory(WDL_String& dir) override;
  bool PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func) override;

  IPopupMenu* GetItemMenu(long idx, long& idxInMenu, long& offsetIdx, IPopupMenu& baseMenu);
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
  bool SetTextInClipboard(const WDL_String& str) override;
  
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK ParamEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static BOOL CALLBACK FindMainWindow(HWND hWnd, LPARAM lParam);
    
protected:
  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds) override;
  void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) override;

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

  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) override;
  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) override;

  inline IMouseInfo GetMouseInfo(LPARAM lParam, WPARAM wParam);
  inline IMouseInfo GetMouseInfoDeltas(float&dX, float& dY, LPARAM lParam, WPARAM wParam);
  bool MouseCursorIsLocked();

#ifdef IGRAPHICS_GL
  //OpenGL context management - TODO: RAII instead?
  void CreateGLContext();
  void DestroyGLContext();

  // Captures previously active GLContext and HDC for restoring, Gets DC
  void ActivateGLContext();
  // Restores previous GL context and Releases DC
  void DeactivateGLContext();
  HGLRC mHGLRC = nullptr;
  HGLRC mStartHGLRC = nullptr;
  HDC mStartHDC = nullptr;
#endif

  HINSTANCE mHInstance = nullptr;
  HWND mPlugWnd = nullptr;
  HWND mParamEditWnd = nullptr;
  HWND mTooltipWnd = nullptr;
  HWND mParentWnd = nullptr;
  HWND mMainWnd = nullptr;
  WNDPROC mDefEditProc = nullptr;
  HFONT mEditFont = nullptr;
  DWORD mPID = 0;

  const IParam* mEditParam = nullptr;
  IText mEditText;
  IRECT mEditRECT;

  EParamEditMsg mParamEditMsg = kNone;
  bool mShowingTooltip = false;
  float mHiddenCursorX;
  float mHiddenCursorY;
  int mTooltipIdx = -1;

  WDL_String mMainWndClassName;
    
  static StaticStorage<InstalledFont> sPlatformFontCache;
  static StaticStorage<HFontHolder> sHFontCache;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE


