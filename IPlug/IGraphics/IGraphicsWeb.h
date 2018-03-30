#pragma once

#include <emscripten/val.h>

#include "IPlugPlatform.h"

#include "IGraphicsPathBase.h"

/** IGraphics draw/platform class HTML5 canvas
*   @ingroup DrawClasses
*   @ingroup PlatformClasses */
class IGraphicsWeb : public IGraphicsPathBase
{
public:
  const char* GetDrawingAPIStr() override { return "WEB"; }

  IGraphicsWeb(IDelegate& dlg, int w, int h, int fps);
  ~IGraphicsWeb();

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override {} // TODO:

  void PathClear() override { getContext().call<void>("beginPath"); } // TODO:
  void PathStart() override { getContext().call<void>("beginPath"); } // TODO:
  void PathClose() override { getContext().call<void>("closePath"); }

  void PathArc(float cx, float cy, float r, float aMin, float aMax) override { getContext().call<void>("arc", cx, cy, r, DegToRad(aMin), DegToRad(aMax)); }

  void PathMoveTo(float x, float y) override { getContext().call<void>("moveTo", x, y); }
  void PathLineTo(float x, float y) override { getContext().call<void>("lineTo", x, y); }
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override { getContext().call<void>("bezierCurveTo", x1, y1, x2, y2, x3, y3); }

  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;

  void PathStateSave() override { getContext().call<void>("save"); }
  void PathStateRestore() override {  getContext().call<void>("restore"); }

  void PathTransformTranslate(float x, float y) override { getContext().call<void>("translate", x, y); }
  void PathTransformScale(float scaleX, float scaleY) override { getContext().call<void>("scale", scaleX, scaleY); }
  void PathTransformRotate(float angle) override { getContext().call<void>("rotate", angle); }

  IColor GetPoint(int x, int y) override {} // TODO:
  void* GetData() override {} // TODO:

  bool DrawText(const IText& text, const char* str, IRECT& bounds, bool measure) override {} // TODO:
  bool MeasureText(const IText& text, const char* str, IRECT& bounds) override {} // TODO:

  void RenderDrawBitmap() override {} // TODO:

  void SetPlatformContext(void* pContext) override {} // TODO:

  void HideMouseCursor() override {} // TODO:
  void ShowMouseCursor() override {} // TODO:
  void MoveMouseCursor(float x, float y) override {} // TODO:
  void ForceEndUserEdit() override {} // TODO:
  void Resize(int w, int h, float scale) override;
  void* OpenWindow(void* pParentWnd) override {} // TODO:
  void CloseWindow() override {} // TODO:
  void* GetWindow() override {} // TODO:
  bool WindowIsOpen() override { return GetWindow(); } // TODO: ??
  bool GetTextFromClipboard(WDL_String& str) override {} // TODO:
  void UpdateTooltips() override {} // TODO:
  int ShowMessageBox(const char* str, const char* caption, int type) override {} // TODO:
  IPopupMenu* CreatePopupMenu(const IPopupMenu& menu, const IRECT& bounds) override {} // TODO:
  void CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str = "") override {} // TODO:
  void PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action = kFileOpen, const char* extensions = 0) override {} // TODO:
  bool PromptForColor(IColor& color, const char* str = "") override {} // TODO:
  bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0) override {} // TODO:
  const char* GetPlatformAPIStr() override { return "WEB"; }
  
  void HostPath(WDL_String& path) override {} // TODO:
  void PluginPath(WDL_String& path) override {} // TODO:
  void UserHomePath(WDL_String& path) override {} // TODO:
  void DesktopPath(WDL_String& path) override  {} // TODO:
  void AppSupportPath(WDL_String& path, bool isSystem = false) override  {} // TODO:
  void SandboxSafeAppSupportPath(WDL_String& path) override {} // TODO:
  void VST3PresetsPath(WDL_String& path, bool isSystem = true) {} // TODO:
  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select = false) override {} // TODO:

protected:
  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override {} // TODO:
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override {} // TODO:
  bool OSFindResource(const char* name, const char* type, WDL_String& result) override {} // TODO:

private:
//  void ClipRegion(const IRECT& r) override {} // TODO:
//  void ResetClipRegion() override {} // TODO:
  
  emscripten::val getCanvas()
  {
    return emscripten::val::global("document").call<emscripten::val>("getElementById", std::string("canvas"));
  }

  emscripten::val getContext()
  {
    emscripten::val canvas = getCanvas();
    return canvas.call<emscripten::val>("getContext", std::string("2d"));
  }
  
  void SetWebSourcePattern(const IPattern& pattern, const IBlend* pBlend);
};

