#pragma once

#include "IGraphics.h"
#include "IGraphicsNanoSVG.h"

class IGraphicsPathBase : public IGraphics
{
  
public:
  
  IGraphicsPathBase(IDelegate& dlg, int w, int h, int fps) : IGraphics(dlg, w, h, fps) {}
  
  // Draw methods

  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend) override
  {
    PathMoveTo(x1, y1);
    PathLineTo(x2, y2);
    PathStroke(color, 1.0, IStrokeOptions(), pBlend);
  }
  
  void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override
  {
    PathTriangle(x1, y1, x2, y2, x3, y3);
    PathStroke(color, 1.0, IStrokeOptions(), pBlend);
  }
  
  void DrawRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override
  {
    PathRect(rect);
    PathStroke(color, 1.0, IStrokeOptions(), pBlend);
  }
  
  void DrawRoundRect(const IColor& color, const IRECT& rect, float corner, const IBlend* pBlend) override
  {
    PathRoundRect(rect, corner);
    PathStroke(color, 1.0, IStrokeOptions(), pBlend);
  }
  
  void DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend) override
  {
    PathConvexPolygon(x, y, npoints);
    PathStroke(color, 1.0, IStrokeOptions(), pBlend);
  }
  
  void DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend) override
  {
    PathArc(cx, cy, r, aMin -90.f, aMax -90.f);
    PathStroke(color, 1.0, IStrokeOptions(), pBlend);
  }
  
  void DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend) override
  {
    PathCircle(cx, cy, r);
    PathStroke(color, 1.0, IStrokeOptions(), pBlend);
  }
  
  // Dotted Rect
  
  void DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override
  {
    float dashLength = 2;
    IStrokeOptions options;
    options.mDash.SetDash(&dashLength, 0.0, 1);
    PathRect(rect);
    PathStroke(color, 1.0, options, pBlend);
  }

  // Fill methods
  
  void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override
  {
    PathTriangle(x1, y1, x2, y2, x3, y3);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override
  {
    PathRect(rect);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillRoundRect(const IColor& color, const IRECT& rect, float corner, const IBlend* pBlend) override
  {
    PathRoundRect(rect, corner);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend) override
  {
    PathConvexPolygon(x, y, npoints);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend) override
  {
    PathMoveTo(cx, cy);
    PathArc(cx, cy, r, aMin - 90.f, aMax - 90.f);
    PathClose();
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend) override
  {
    PathCircle(cx, cy, r);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  // Pixel Manipulation
  
  void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend) override
  {
    FillRect(color, IRECT(x, y, 1, 1), pBlend);
  }
  
  void ForcePixel(const IColor& color, int x, int y) override
  {
    IColor preMulColor(255, (color.R * color.A) / 255, (color.G * color.A) / 255, (color.B * color.A) / 255);
    DrawPoint(preMulColor, x, y, 0);
  }
  
  // Path Methods

  bool HasPathSupport() const override { return true; }
  
  void PathTriangle(float x1, float y1, float x2, float y2, float x3, float y3) override
  {
    PathMoveTo(x1, y1);
    PathLineTo(x2, y2);
    PathLineTo(x3, y3);
    PathClose();
  }
  
  void PathRect(const IRECT& rect) override
  {
    PathMoveTo(rect.L, rect.T);
    PathLineTo(rect.R, rect.T);
    PathLineTo(rect.R, rect.B);
    PathLineTo(rect.L, rect.B);
    PathClose();
  }
  
  void PathRoundRect(const IRECT& rect, float cr) override
  {
    const double y = rect.B - rect.H();
    PathMoveTo(rect.L, y + cr);
    PathArc(rect.L + cr, y + cr, cr, 180.0, 270.0);
    PathArc(rect.L + rect.W() - cr, y + cr, cr, 270.0, 360.0);
    PathArc(rect.L + rect.W() - cr, y + rect.H() - cr, cr, 0.0, 90.0);
    PathArc(rect.L + cr, y + rect.H() - cr, cr, 90.0, 180.0);
    PathClose();
  }
  
  void PathCircle(float cx, float cy, float r) override
  {
    PathMoveTo(cx + r, cy);
    PathArc(cx, cy, r, 0.f, 360.f);
    PathClose();
  }
  
  void PathConvexPolygon(float* x, float* y, int npoints) override
  {
    PathMoveTo(x[0], y[0]);
    for(int i = 1; i < npoints; i++)
      PathLineTo(x[i], y[i]);
    PathClose();
  }
  
  virtual void PathStateSave() = 0;
  virtual void PathStateRestore() = 0;
  
  virtual void PathTransformTranslate(float x, float y) = 0;
  virtual void PathTransformScale(float scale) = 0;
  virtual void PathTransformRotate(float angle) = 0;
  
  // SVG Support
  
  void DrawSVG(ISVG& svg, const IRECT& dest, const IBlend* pBlend) override
  {
    double xScale = dest.W() / svg.W();
    double yScale = dest.H() / svg.H();
    double scale = xScale < yScale ? xScale : yScale;
    
    PathStateSave();
    PathTransformTranslate(dest.L, dest.T);
    ClipRegion(IRECT(0, 0, dest.W(), dest.H()));
    PathTransformScale(scale);
    NanoSVGRenderer::RenderNanoSVG(*this, svg.mImage);
    PathStateRestore();
  }
  
  void DrawRotatedSVG(ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend) override
  {
    PathStateSave();
    PathTransformTranslate(destCtrX, destCtrY);
    PathTransformRotate(angle);
    DrawSVG(svg, IRECT(-width * 0.5, - height * 0.5, width * 0.5, height * 0.5), pBlend);
    PathStateRestore();
  }

};
