/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>

#include <utility>

#include "IPlugPlatform.h"

#include "IGraphics_select.h"

using namespace emscripten;

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

static val GetCanvas()
{
  return val::global("document").call<val>("getElementById", std::string("canvas"));
}

static val GetPreloadedImages()
{
  return val::global("preloadedImages");
}

extern void GetScreenDimensions(int& width, int& height);

/** IGraphics platform class for the web
* @ingroup PlatformClasses */
class IGraphicsWeb final : public IGRAPHICS_DRAW_CLASS
{
  class Font;
  class FileFont;
  class MemoryFont;
public:
  IGraphicsWeb(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsWeb();

  void DrawResize() override;

  const char* GetPlatformAPIStr() override { return "WEB"; }

  void HideMouseCursor(bool hide, bool lock) override;
  void MoveMouseCursor(float x, float y) override { /* NOT SUPPORTABLE*/ }
  ECursor SetMouseCursor(ECursor cursorType) override;
  void GetMouseLocation(float& x, float&y) const override;

  void ForceEndUserEdit() override {} // TODO:
  void* OpenWindow(void* pParent) override;
  void CloseWindow() override {} // TODO:
  void* GetWindow() override { return nullptr; } // TODO:
  bool WindowIsOpen() override { return GetWindow(); } // TODO: ??
  bool GetTextFromClipboard(WDL_String& str) override { str.Set(mClipboardText.Get()); return true; }
  bool SetTextInClipboard(const char* str) override { mClipboardText.Set(str); return true; }
  void UpdateTooltips() override {} // TODO:
  EMsgBoxResult ShowMessageBox(const char* str, const char* title, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler) override;
  
  void PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler) override;
  void PromptForDirectory(WDL_String& path, IFileDialogCompletionHandlerFunc completionHandler) override;
  bool PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func) override;
  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;

  bool PlatformSupportsMultiTouch() const override { return true; }
  
  //IGraphicsWeb
  static void OnMainLoopTimer();
  double mPrevX = 0.;
  double mPrevY = 0.;
  
protected:
  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync) override;
  void CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str) override;
    
private:
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style) override;
  PlatformFontPtr LoadPlatformFont(const char* fontID, void* pData, int dataSize) override;
  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font) override {}

  WDL_String mClipboardText;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

