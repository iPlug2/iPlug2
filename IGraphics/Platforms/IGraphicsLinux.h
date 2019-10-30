/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IGraphics_select.h"

#ifdef IGRAPHICS_GL
  #include <X11/Xlib.h>
#endif

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

  void* GetWindow() override { return mPlugWnd; }

  bool WindowIsOpen() override { return (mPlugWnd); }

  void PlatformResize(bool parentHasResized) override;

  void HideMouseCursor(bool hide, bool lock) override;
  void MoveMouseCursor(float x, float y) override;

  EMsgBoxResult ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler) override;
  void ForceEndUserEdit() override;

  const char* GetPlatformAPIStr() override { return "Linux"; }

  void UpdateTooltips() override {}; // TODO

  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select) override { return false; } // TODO
  void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* extensions) override;
  void PromptForDirectory(WDL_String& dir) override;

  bool PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func) override { return false; } // TODO

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override { return false; } // TODO

  static int GetUserOSVersion();
  bool GetTextFromClipboard(WDL_String& str) override;
  bool SetTextInClipboard(const WDL_String& str) override { return false; } // TODO

  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) override;

  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) override { } // No reason to cache (no universal handle)

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

  IPopupMenu *CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds) override { return nullptr; } // TODO
  void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) override { } // TODO


private:

  HWND mPlugWnd = nullptr;
  HWND mParentWnd = nullptr;
  HWND mParamEditWnd = nullptr;

  // SWELL does not define/use that
  enum {
    MK_SHIFT = 0x0004,
    MK_CONTROL = 0x0008
  };

  int  mModKeys = 0;

  EParamEditMsg mParamEditMsg = kNone;
  bool mShowingTooltip = false;
  float mHiddenCursorX;
  float mHiddenCursorY;

  static LRESULT  PlugWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

#ifdef IGRAPHICS_GL
  Display *mGLDisplay = nullptr;
  Window   mGLWnd = 0;
  Colormap mGLColormap = 0;
  void    *mGLContext = nullptr; // GLXContext
  void    *mGLOldContext = nullptr;
  HWND     mGLParent = nullptr;

  void getGLRect(RECT *r);

  bool CreateGLContext();
  void DestroyGLContext();
  void ActivateGLContext();
  void DeactivateGLContext();

  static bool GLFilter(XEvent *xevent, void *, void *me);
#endif
  bool mOpenBHDone = false;
  bool OpenWindowBH();

  PAINTSTRUCT mPS;
  bool ActivateContext();
  void DeactivateContext();

  void Paint();

  inline IMouseInfo GetMouseInfo(LPARAM lParam);
  inline IMouseInfo GetMouseInfoDeltas(float&dX, float& dY, LPARAM lParam);
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
