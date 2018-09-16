#pragma once

#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <emscripten/html5.h>

#include "IPlugPlatform.h"

#include "IGraphics_select.h"

using namespace emscripten;

struct RetainVal
{
  RetainVal(val item) : mItem(item) {}
  val mItem;
};

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

  const char* GetPlatformAPIStr() override { return "WEB"; }

  void SetPlatformContext(void* pContext) override {} // TODO:

  void HideMouseCursor(bool hide, bool returnToStartPos) override;
  void ForceEndUserEdit() override {} // TODO:
  void* OpenWindow(void* pParent) override;
  void CloseWindow() override {} // TODO:
  void* GetWindow() override { return nullptr; } // TODO:
  bool WindowIsOpen() override { return GetWindow(); } // TODO: ??
  bool GetTextFromClipboard(WDL_String& str) override;
  void UpdateTooltips() override {} // TODO:
  int ShowMessageBox(const char* str, const char* caption, int type) override;
  
  IPopupMenu* CreatePopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller) override;
  
  void CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str) override;
  void PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action, const char* ext) override {} // TODO:
  void PromptForDirectory(WDL_String& path) override {} // TODO:
  bool PromptForColor(IColor& color, const char* str) override { return false; } // TODO:
  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) override;
  
  //IGraphicsWeb
  static void OnMainLoopTimer();
  double mPrevX = 0.;
  double mPrevY = 0.;
  
protected:
  bool OSFindResource(const char* name, const char* type, WDL_String& result) override;
};
