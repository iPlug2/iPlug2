/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include <emscripten/val.h>
#include <emscripten/bind.h>

#include "IPlugPlatform.h"

#include "IGraphicsPathBase.h"

using namespace emscripten;

class CanvasBitmap : public APIBitmap
{
public:
  CanvasBitmap(val imageCanvas, const char* name, int scale);
  CanvasBitmap(int width, int height, int scale, float drawScale);
  ~CanvasBitmap();
};

/** IGraphics draw class HTML5 canvas
* @ingroup DrawClasses */
class IGraphicsCanvas : public IGraphicsPathBase
{
public:
  const char* GetDrawingAPIStr() override { return "HTML5 Canvas"; }

  IGraphicsCanvas(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsCanvas();

  void DrawBitmap(IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, float destCentreX, float destCentreY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;

  void DrawResize() override {};

  void PathClear() override;
  void PathClose() override;
  void PathArc(float cx, float cy, float r, float aMin, float aMax) override;
  void PathMoveTo(float x, float y) override;
  void PathLineTo(float x, float y) override;
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override;

  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;

  IColor GetPoint(int x, int y) override { return COLOR_BLACK; } // TODO:
  void* GetDrawContext() override { return nullptr; }

protected:
  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int scale) override;
  APIBitmap* CreateAPIBitmap(int width, int height) override;

  bool DoDrawMeasureText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure) override;

private:
  
  val GetContext()
  {
    val canvas = mLayers.empty() ? val::global("document").call<val>("getElementById", std::string("canvas")) : *(mLayers.top()->GetAPIBitmap()->GetBitmap());
      
    return canvas.call<val>("getContext", std::string("2d"));
  }
    
  double XTranslate()  { return mLayers.empty() ? 0 : -mLayers.top()->Bounds().L; }
  double YTranslate()  { return mLayers.empty() ? 0 : -mLayers.top()->Bounds().T; }

  void PathTransformSetMatrix(const IMatrix& m) override;
  void SetClipRegion(const IRECT& r) override;
    
  void SetCanvasSourcePattern(const IPattern& pattern, const IBlend* pBlend = nullptr);
  void SetCanvasBlendMode(const IBlend* pBlend);
};
