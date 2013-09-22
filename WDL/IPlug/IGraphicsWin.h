#ifndef _IGRAPHICSWIN_
#define _IGRAPHICSWIN_

#include "IGraphics.h"

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

class IGraphicsWin : public IGraphics
{
public:
  IGraphicsWin(IPlugBase* pPlug, int w, int h, int refreshFPS);
  virtual ~IGraphicsWin();

  void SetHInstance(HINSTANCE hInstance) { mHInstance = hInstance; }

  void ForceEndUserEdit();

  void Resize(int w, int h);

  void HideMouseCursor();
  void ShowMouseCursor();
  int ShowMessageBox(const char* pText, const char* pCaption, int type);

  bool DrawScreen(IRECT* pR);

  void* OpenWindow(void* pParentWnd);
  void CloseWindow();
  bool WindowIsOpen() { return (mPlugWnd); }
  
  void UpdateTooltips() {}

  void HostPath(WDL_String* pPath);
  void PluginPath(WDL_String* pPath);
  void DesktopPath(WDL_String* pPath);
  void AppSupportPath(WDL_String* pPath);

  void PromptForFile(WDL_String* pFilename, EFileAction action = kFileOpen, WDL_String* pDir = 0, char* extensions = "");   // extensions = "txt wav" for example.
  bool PromptForColor(IColor* pColor, char* prompt = "");

  IPopupMenu* GetItemMenu(long idx, long &idxInMenu, long &offsetIdx, IPopupMenu* pMenu);
  HMENU CreateMenu(IPopupMenu* pMenu, long* offsetIdx);
  IPopupMenu* CreateIPopupMenu(IPopupMenu* pMenu, IRECT* pAreaRect);
  void CreateTextEntry(IControl* pControl, IText* pText, IRECT* pTextRect, const char* pString, IParam* pParam);

  bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0);

  // Specialty use!
  void* GetWindow() { return mPlugWnd; }
  HWND GetParentWindow() { return mParentWnd; }
  HWND GetMainWnd();
  void SetMainWndClassName(char* name) { mMainWndClassName.Set(name); }
  void GetMainWndClassName(char* name) { strcpy(name, mMainWndClassName.Get()); }
  IRECT GetWindowRECT();
  void SetWindowTitle(char* str);

protected:
  LICE_IBitmap* OSLoadBitmap(int ID, const char* name);

  void SetTooltip(const char* tooltip);
  void ShowTooltip();
  void HideTooltip();

private:
  HINSTANCE mHInstance;
  HWND mPlugWnd, mParamEditWnd, mTooltipWnd;
  // Ed = being edited manually.
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

////////////////////////////////////////

#endif