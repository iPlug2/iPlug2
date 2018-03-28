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

  void PathClear() override {} // TODO:
  void PathStart() override {} // TODO:
  void PathClose() override {} // TODO:

  void PathArc(float cx, float cy, float r, float aMin, float aMax) override {} // TODO:

  void PathMoveTo(float x, float y) override {} // TODO:
  void PathLineTo(float x, float y) override {} // TODO:
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override {} // TODO:

  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override {} // TODO:
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override {} // TODO:

  void PathStateSave() override {} // TODO:
  void PathStateRestore() override {} // TODO:

  void PathTransformTranslate(float x, float y) override {} // TODO:
  void PathTransformScale(float scaleX, float scaleY) override {} // TODO:
  void PathTransformRotate(float angle) override {} // TODO:

  IColor GetPoint(int x, int y) override {} // TODO:
  void* GetData() override {} // TODO:

  bool DrawText(const IText& text, const char* str, IRECT& bounds, bool measure) override {} // TODO:
  bool MeasureText(const IText& text, const char* str, IRECT& bounds) override {} // TODO:

  void RenderDrawBitmap() override {} // TODO:

  void SetPlatformContext(void* pContext) override {} // TODO:

  virtual void HideMouseCursor() override {} // TODO:
  virtual void ShowMouseCursor() override {} // TODO:
  virtual void MoveMouseCursor(float x, float y) override {} // TODO:
  virtual void ForceEndUserEdit() override {} // TODO:
  virtual void Resize(int w, int h, float scale) override {} // TODO:
  virtual void* OpenWindow(void* pParentWnd) override {} // TODO:
  virtual void CloseWindow() override {} // TODO:
  virtual void* GetWindow() override {} // TODO:
  virtual bool WindowIsOpen() override { return GetWindow(); } // TODO: ??
  virtual bool GetTextFromClipboard(WDL_String& str) override {} // TODO:
  virtual void UpdateTooltips() override {} // TODO:
  virtual int ShowMessageBox(const char* str, const char* caption, int type) override {} // TODO:
  IPopupMenu* CreatePopupMenu(const IPopupMenu& menu, const IRECT& bounds) override {} // TODO:
  virtual void CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str = "") override {} // TODO:
  virtual void PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action = kFileOpen, const char* extensions = 0) override {} // TODO:
  virtual bool PromptForColor(IColor& color, const char* str = "") override {} // TODO:
  virtual bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0) override {} // TODO:
  virtual const char* GetPlatformAPIStr() override { return "WEB"; }
  
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
};

