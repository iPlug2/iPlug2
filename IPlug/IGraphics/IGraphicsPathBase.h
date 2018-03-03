#pragma once

#include "IGraphics.h"

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
    PathStart();
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
};
