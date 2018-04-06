#pragma once

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

//TODO: would be nice not to put this here
#ifndef NO_IGRAPHICS
  #ifdef IGRAPHICS_AGG
    #define IGRAPHICS_DRAW_CLASS IGraphicsAGG
    #include "IGraphicsAGG.h"
  #elif defined IGRAPHICS_CAIRO
    #define IGRAPHICS_DRAW_CLASS IGraphicsCairo
    #include "IGraphicsCairo.h"
  #elif defined IGRAPHICS_NANOVG
    #define IGRAPHICS_DRAW_CLASS IGraphicsNanoVG
    #include "IGraphicsNanoVG.h"
  #elif !defined DOXYGEN_SHOULD_SKIP_THIS
    #define IGRAPHICS_DRAW_CLASS IGraphicsCairo
    #include "IGraphicsCairo.h"
  #endif
#endif

/** IGraphics platform class for Windows
*   @ingroup PlatformClasses
*/
class IGraphicsWin final : public IGRAPHICS_DRAW_CLASS
{
public:
  IGraphicsWin(IDelegate& dlg, int w, int h, int fps);
  ~IGraphicsWin();

  void SetPlatformInstance(void* instance) override { mHInstance = (HINSTANCE) instance; }
  void* GetPlatformInstance() override { return mHInstance; }

  void ForceEndUserEdit() override;

  void Resize(int w, int h, float scale) override;

  void HideMouseCursor(bool hide) override;
  void MoveMouseCursor(float x, float y) override { /* TODO - Oli - I have code for this - Alex */ };

  int ShowMessageBox(const char* str, const char* caption, int type) override;

  void* OpenWindow(void* pParentWnd) override;
  void CloseWindow() override;
  bool WindowIsOpen() override { return (mDelegateWnd); }

  void UpdateTooltips() override {}

  void HostPath(WDL_String& path) override;
  void PluginPath(WDL_String& path) override;
  void DesktopPath(WDL_String& path) override;
  void UserHomePath(WDL_String& path) override;
  void AppSupportPath(WDL_String& path, bool isSystem) override;
  void SandboxSafeAppSupportPath(WDL_String& path) override { AppSupportPath(path, false); }
  void VST3PresetsPath(WDL_String& path, const char* mfrName, const char* pluginName, bool isSystem) override;
  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override;

  void PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action, const char* ext) override;
  void PromptForDirectory(WDL_String& dir) override;
  bool PromptForColor(IColor& color, const char* str) override;

  IPopupMenu* GetItemMenu(long idx, long& idxInMenu, long& offsetIdx, const IPopupMenu& baseMenu);
  HMENU CreateMenu(const IPopupMenu& menu, long* pOffsetIdx);

  IPopupMenu* CreatePopupMenu(const IPopupMenu& menu, const IRECT& bounds) override;
  void CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str) override;

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure);

  void* GetWindow() override { return mDelegateWnd; }
  HWND GetParentWindow() const { return mParentWnd; }
  HWND GetMainWnd();
  void SetMainWndClassName(const char* name) { mMainWndClassName.Set(name); }
//  void GetMainWndClassName(char* name) { strcpy(name, mMainWndClassName.Get()); }
  IRECT GetWindowRECT();
  void SetWindowTitle(const char* str);

  const char* GetPlatformAPIStr() override { return "win32"; };

  bool GetTextFromClipboard(WDL_String& str) override;

  bool OSFindResource(const char* name, const char* type, WDL_String& result) override;
protected:
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

  HINSTANCE mHInstance = nullptr;
  HWND mDelegateWnd = nullptr;
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
  int mTooltipIdx = -1;

  WDL_String mMainWndClassName;
public:
  static BOOL EnumResNameProc(HANDLE module, LPCTSTR type, LPTSTR name, LONG param);
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK ParamEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static BOOL CALLBACK FindMainWindow(HWND hWnd, LPARAM lParam);
};
