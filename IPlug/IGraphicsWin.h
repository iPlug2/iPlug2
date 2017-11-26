#pragma once

#include "IGraphics.h"

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

class IGraphicsWin : public IGraphics
{
public:
  IGraphicsWin(IPlugBaseGraphics* pPlug, int w, int h, int refreshFPS);
  virtual ~IGraphicsWin();

  void SetHInstance(HINSTANCE hInstance) { mHInstance = hInstance; }

  void ForceEndUserEdit();

  void Resize(int w, int h);

  void HideMouseCursor();
  void ShowMouseCursor();
  int ShowMessageBox(const char* pStr, const char* pCaption, int type);

  bool DrawScreen(IRECT* pR);

  void* OpenWindow(void* pParentWnd);
  void CloseWindow();
  bool WindowIsOpen() { return (mPlugWnd); }
  
  void UpdateTooltips() {}

  void HostPath(WDL_String& path);
  void PluginPath(WDL_String& path);
  void DesktopPath(WDL_String& path);
  //void VST3PresetsPath(WDL_String* pPath, bool isSystem = true);
  void AppSupportPath(WDL_String& path, bool isSystem = false);
  void SandboxSafeAppSupportPath(WDL_String& pPath) { AppSupportPath(path, false); }

  void PromptForFile(WDL_String& fileName, EFileAction action = kFileOpen, WDL_String* pDir = 0, char* pExtensions = "");   // extensions = "txt wav" for example.
  bool PromptForColor(IColor& colour, const char* pStr);

  IPopupMenu* GetItemMenu(long idx, long &idxInMenu, long &offsetIdx, IPopupMenu* pMenu);
  HMENU CreateMenu(IPopupMenu& menu, long* offsetIdx);
  IPopupMenu* CreateIPopupMenu(IPopupMenu& menu, IRECT& areaRect);
  void CreateTextEntry(IControl* pControl, const IText& text, const IRECT& textRect, const char* pStr, IParam* pParam);

  bool OpenURL(const char* pUrl, const char* pMsgWindowTitle = 0, const char* pConfirmMsg = 0, const char* pErrMsgOnFailure = 0);

  // Specialty use!
  void* GetWindow() { return mPlugWnd; }
  HWND GetParentWindow() { return mParentWnd; }
  HWND GetMainWnd();
  void SetMainWndClassName(char* name) { mMainWndClassName.Set(name); }
  void GetMainWndClassName(char* name) { strcpy(name, mMainWndClassName.Get()); }
  IRECT GetWindowRECT();
  void SetWindowTitle(char* str);

  const char* GetGUIAPI() { return "Windows GDI"; };
  
  bool GetTextFromClipboard(WDL_String* pStr);
protected:
  LICE_IBitmap* OSLoadBitmap(int ID, const char* name);

  void SetTooltip(const char* tooltip);
  void ShowTooltip();
  void HideTooltip();

private:
  HINSTANCE mHInstance;
  HWND mPlugWnd, mParamEditWnd, mTooltipWnd;
  IControl* mEdControl;
  IParam* mEdParam;
  WNDPROC mDefEditProc;
  int mParamEditMsg;
  bool mShowingTooltip;
  int mTooltipIdx;
  COLORREF* mCustomColorStorage;

  DWORD mPID;
  HWND mParentWnd, mMainWnd;
  WDL_String mMainWndClassName;

public:
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK ParamEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static BOOL CALLBACK FindMainWindow(HWND hWnd, LPARAM lParam);
};
