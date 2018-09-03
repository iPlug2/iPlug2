#pragma once

#include <emscripten/val.h>
#include <emscripten/bind.h>

#include "IPlugPlatform.h"

#include "IGraphicsPathBase.h"

using namespace emscripten;

class WebBitmap : public APIBitmap
{
public:
  WebBitmap(val imageCanvas, const char* name, int scale);
};

static std::string ToCanvasColor(const IColor& color, float alpha = 1.0)
{
  WDL_String str;
  str.SetFormatted(64, "rgba(%d, %d, %d, %lf)", color.R, color.G, color.B, alpha * color.A / 255.0);
  return str.Get();
}

/** IGraphics draw class HTML5 canvas
* @ingroup DrawClasses */
class IGraphicsCanvas : public IGraphicsPathBase
{
public:
  const char* GetDrawingAPIStr() override { return "Canvas"; }

  IGraphicsCanvas(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsCanvas();

  void DrawBitmap(IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, float destCentreX, float destCentreY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override { IGraphicsPathBase::DrawRotatedBitmap(bitmap, destCentreX, destCentreY, DegToRad(angle), yOffsetZeroDeg, pBlend); }
  
  void PathClear() override { GetContext().call<void>("beginPath"); }
  void PathClose() override { GetContext().call<void>("closePath"); }

  void PathArc(float cx, float cy, float r, float aMin, float aMax) override { GetContext().call<void>("arc", cx, cy, r, DegToRad(aMin - 90.f), DegToRad(aMax - 90.f)); }

  void PathMoveTo(float x, float y) override { GetContext().call<void>("moveTo", x, y); }
  void PathLineTo(float x, float y) override { GetContext().call<void>("lineTo", x, y); }
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override { GetContext().call<void>("bezierCurveTo", x1, y1, x2, y2, x3, y3); }

  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;

  void PathStateSave() override { GetContext().call<void>("save"); }
  void PathStateRestore() override {  GetContext().call<void>("restore"); }

  void PathTransformTranslate(float x, float y) override { GetContext().call<void>("translate", x, y); }
  void PathTransformScale(float scaleX, float scaleY) override { GetContext().call<void>("scale", scaleX, scaleY); }
  void PathTransformRotate(float angle) override { GetContext().call<void>("rotate", DegToRad(angle)); }

  IColor GetPoint(int x, int y) override { return COLOR_BLACK; } // TODO:
  void* GetDrawContext() override { return nullptr; } // TODO:

  bool DrawText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure) override;
  bool MeasureText(const IText& text, const char* str, IRECT& bounds) override;

protected:
  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override;

private:
  void ClipRegion(const IRECT& r) override;
  void ResetClipRegion() override;

  val GetCanvas()
  {
    return val::global("document").call<val>("getElementById", std::string("canvas"));
  }

  val GetContext()
  {
    val canvas = GetCanvas();
    return canvas.call<val>("getContext", std::string("2d"));
  }
  
  void SetWebSourcePattern(const IPattern& pattern, const IBlend* pBlend = nullptr);
  void SetWebBlendMode(const IBlend* pBlend);
};
