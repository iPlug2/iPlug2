#pragma once

#include <emscripten/val.h>

#include "IPlugPlatform.h"

#include "IGraphicsPathBase.h"

/** IGraphics draw/platform class using Web (HTML5 canvas)
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
  void PathClose() override {}

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

//  virtual void HideMouseCursor() {};
//  virtual void ShowMouseCursor() {};
//  virtual void MoveMouseCursor(float x, float y) = 0;
//  void SetTabletInput(bool tablet) { mTabletInput = tablet; }
//  virtual void ForceEndUserEdit() = 0;
//  virtual void Resize(int w, int h, float scale);
//  virtual void* OpenWindow(void* pParentWnd) = 0;
//  virtual void CloseWindow() = 0;
//  virtual void* GetWindow() = 0;
//  virtual bool WindowIsOpen() { return GetWindow(); }
//  virtual bool GetTextFromClipboard(WDL_String& str) = 0;
//  virtual void UpdateTooltips() = 0;
//  virtual int ShowMessageBox(const char* str, const char* caption, int type) = 0;
//  virtual void CreateTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str = "") = 0;
//  virtual void PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action = kFileOpen, const char* extensions = 0) = 0;
//  virtual bool PromptForColor(IColor& color, const char* str = "") = 0;
//  virtual bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0) = 0;
//  virtual const char* GetPlatformAPIStr() { return ""; }
  
//   void HostPath(WDL_String& path) override;
//   void PluginPath(WDL_String& path) override;
//   void DesktopPath(WDL_String& path) override;
//   void AppSupportPath(WDL_String& path, bool isSystem = false) override;
//   void SandboxSafeAppSupportPath(WDL_String& path) override;
//   void VST3PresetsPath(WDL_String& path, bool isSystem = true);
//   bool RevealPathInExplorerOrFinder(WDL_String& path, bool select = false) override;

protected:
  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override {} // TODO:
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override {} // TODO:

private:
//  void ClipRegion(const IRECT& r) override {} // TODO:
//  void ResetClipRegion() override {} // TODO:
};

