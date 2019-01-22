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

#include "IPlugPlatform.h"

#include "IGraphics_select.h"

using namespace emscripten;

static val GetCanvas()
{
  return val::global("document").call<val>("getElementById", std::string("canvas"));
}

static val GetPreloadedImages()
{
  return val::global("Module")["preloadedImages"];
}

/** IGraphics platform class for the web
* @ingroup PlatformClasses */
class IGraphicsWeb final : public IGRAPHICS_DRAW_CLASS
{
public:
  IGraphicsWeb(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsWeb();

  void DrawResize() override;

  const char* GetPlatformAPIStr() override { return "WEB"; }

  void SetPlatformContext(void* pContext) override {} // TODO:

  void HideMouseCursor(bool hide, bool lock) override;
  void MoveMouseCursor(float x, float y) override { /* NOT SUPPORTABLE*/ }
  void SetMouseCursor(ECursor cursor) override;

  void ForceEndUserEdit() override {} // TODO:
  void* OpenWindow(void* pParent) override;
  void CloseWindow() override {} // TODO:
  void* GetWindow() override { return nullptr; } // TODO:
  bool WindowIsOpen() override { return GetWindow(); } // TODO: ??
  bool GetTextFromClipboard(WDL_String& str) override;
  void UpdateTooltips() override {} // TODO:
  int ShowMessageBox(const char* str, const char* caption, EMessageBoxType type) override;
  
  void PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action, const char* ext) override;
  void PromptForDirectory(WDL_String& path) override;
  bool PromptForColor(IColor& color, const char* str) override { return false; } // TODO:
  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;
  
  //IGraphicsWeb
  static void OnMainLoopTimer();
  double mPrevX = 0.;
  double mPrevY = 0.;
  ECursor mCursorType = ECursor::ARROW;
  
protected:
  IPopupMenu* CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller) override;
  void CreatePlatformTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str) override;
  EResourceLocation OSFindResource(const char* name, const char* type, WDL_String& result) override;
};
