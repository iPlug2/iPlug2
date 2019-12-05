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
  HANDLE mTimer = nullptr;
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
public:
  //these two RECTs are accessed by concurrent threads
  RECT mInvalidRECT;
  RECT mValidRECT;
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static LRESULT CALLBACK ParamEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  static BOOL CALLBACK FindMainWindow(HWND hWnd, LPARAM lParam);
  static void CALLBACK TimerProc(void* param, BOOLEAN timerCalled);


  // VBlank support.  Uses gdi.dll methods to sit in a high priority
  // thread waiting for VBlank to occur, then posting a message to the
  // main UI thread.  From there we can decide if we need to redraw
public:
  DWORD OnVBlankRun();
  void StartVBlankThread(HWND hWnd);
  void StopVBlankThread();
protected:
  HWND mVBlankWindow = 0;
  bool mVBlankShutdown = false;
  HANDLE mVBlankThread = INVALID_HANDLE_VALUE;

};

// TODO: move elsewhere


// Defining a class to frame times to report once per x seconds.
// This class uses the high precision timer internally.
class RedrawProfiler
{
public:
  class Report
  {
  public:
    int periodSeconds = 0;
    int startSeconds = 0;
    int framesDuringPeriod = 0;
    double fps = 0;

    // amount of time for a particular redraw (inside the drawing code).
    // Times are in ms.
    double minRedrawTime = 0;
    double maxRedrawTime = 0;
    double avgRedrawTime = 0;

    // Times are in ms.
    double minRedrawPeriod = 0;
    double maxRedrawPeriod = 0;
    double avgRedrawPeriod = 0;

    // Creates one line report
    WDL_String ToString() const;
  };

  // constructs the profiler object
  RedrawProfiler(int periodSeconds = 1);

  // starts profiling (starts first period immediately)
  void StartProfiling();

  // stops profiling
  void StopProfiling();

  void StartDrawingOperation();
  void StopDrawingOperation();

  const Report& GetLastReport() const { return _lastReport; }

private:

  // Updates the report and resets the accumulators for the next one.
  // Get the report in last report.
  //
  // 
  void MakeReport();

  double GetProfileTimestamp();

  void ClearReportVariables();

  // samples the time when calling StartProfiling so we start at the
  // beginning of a peroid and more readable numbers.
  double _epochTime = 0;

  int _periodSeconds = 0;

  // accumulated data need to make report periodically
  int _startSeconds = 0;
  int _framesPerPeriodAcc = 0;
  bool _inDrawingOperation = false;
  double _drawingOperationStart = 0;
  double _minRedrawTime = 0;
  double _maxRedrawTime = 0;
  double _accRedrawTime = 0;
  double _minRedrawPeriod = 0;
  double _maxRedrawPeriod = 0;
  double _avgRedrawPeriod = 0;

  Report _lastReport;
};


END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE


