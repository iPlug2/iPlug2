/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IGraphicsPathBase
 */

#include <algorithm>
#include <stack>

#include "IGraphics.h"

#include "nanosvg.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A base class to share implementations of IGraphics.h functionality across different path based graphics backends. */
class IGraphicsPathBase : public IGraphics
{
public:
  IGraphicsPathBase(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
  : IGraphics(dlg, w, h, fps, scale) 
  {}

  void DrawRotatedBitmap(const IBitmap& bitmap, float destCtrX, float destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override
  {
    //TODO: offset support
    
    float width = bitmap.W() / bitmap.GetDrawScale();
    float height = bitmap.H() / bitmap.GetDrawScale();
    
    PathTransformSave();
    PathTransformTranslate(destCtrX, destCtrY);
    PathTransformRotate((float) angle);
    DrawBitmap(bitmap, IRECT(-width * 0.5f, - height * 0.5f, width * 0.5f, height * 0.5f), 0, 0, pBlend);
    PathTransformRestore();
  }
  
  void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend) override
  {
    FillRect(color, IRECT(x, y, x+1.f, y+1.f), pBlend);
  }
  
  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness) override
  {
    PathClear();
    PathMoveTo(x1, y1);
    PathLineTo(x2, y2);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void DrawGrid(const IColor& color, const IRECT& bounds, float gridSizeH, float gridSizeV, const IBlend* pBlend, float thickness) override
  {
    PathClear();

    // Vertical Lines grid
    if (gridSizeH > 1.f)
    {
      for (float x = bounds.L; x < bounds.R; x += gridSizeH)
      {
        PathMoveTo(x, bounds.T);
        PathLineTo(x, bounds.B);
      }
    }
    // Horizontal Lines grid
    if (gridSizeV > 1.f)
    {
      for (float y = bounds.T; y < bounds.B; y += gridSizeV)
      {
        PathMoveTo(bounds.L, y);
        PathLineTo(bounds.R, y);
      }
    }
    
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void DrawData(const IColor& color, const IRECT& bounds, float* normYPoints, int nPoints, float* normXPoints, const IBlend* pBlend, float thickness) override
  {
    PathClear();
    
    float xPos = bounds.L;

    PathMoveTo(xPos, bounds.B - (bounds.H() * normYPoints[0]));

    for (auto i = 1; i < nPoints; i++)
    {
      if(normXPoints)
        xPos = bounds.L + (bounds.W() * normXPoints[i]);
      else
        xPos = bounds.L + ((bounds.W() / (float) (nPoints - 1) * i));
      
      PathLineTo(xPos, bounds.B - (bounds.H() * normYPoints[i]));
    }
    
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness, float dashLen) override
  {
    PathClear();
    
    IStrokeOptions options;
    options.mDash.SetDash(&dashLen, 0.0, 1);
    PathMoveTo(x1, y1);
    PathLineTo(x2, y2);
    PathStroke(color, thickness, options, pBlend);
  }
  
  void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend, float thickness) override
  {
    PathClear();
    PathTriangle(x1, y1, x2, y2, x3, y3);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness) override
  {
    PathClear();
    PathRect(bounds);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void DrawRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius, const IBlend* pBlend, float thickness) override
  {
    PathClear();
    PathRoundRect(bounds, cornerRadius);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void DrawRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend, float thickness) override
  {
    PathClear();
    PathRoundRect(bounds, cRTL, cRTR, cRBR, cRBL);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void DrawConvexPolygon(const IColor& color, float* x, float* y, int nPoints, const IBlend* pBlend, float thickness) override
  {
    PathClear();
    PathConvexPolygon(x, y, nPoints);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void DrawArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend, float thickness) override
  {
    PathClear();
    PathArc(cx, cy, r, a1, a2);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend, float thickness) override
  {
    PathClear();
    PathCircle(cx, cy, r);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness, float dashLen) override
  {
    PathClear();
    IStrokeOptions options;
    options.mDash.SetDash(&dashLen, 0., 1);
    PathRect(bounds);
    PathStroke(color, thickness, options, pBlend);
  }
  
  void DrawEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness) override
  {
    PathClear();
    PathEllipse(bounds);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }
  
  void DrawEllipse(const IColor& color, float x, float y, float r1, float r2, float angle, const IBlend* pBlend, float thickness) override
  {
    PathClear();
    PathEllipse(x, y, r1, r2, angle);
    PathStroke(color, thickness, IStrokeOptions(), pBlend);
  }

  void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override
  {
    PathClear();
    PathTriangle(x1, y1, x2, y2, x3, y3);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend) override
  {
    PathClear();
    PathRect(bounds);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius, const IBlend* pBlend) override
  {
    PathClear();
    PathRoundRect(bounds, cornerRadius);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillRoundRect(const IColor& color, const IRECT& bounds, float cRTL, float cRTR, float cRBR, float cRBL, const IBlend* pBlend) override
  {
    PathClear();
    PathRoundRect(bounds, cRTL, cRTR, cRBR, cRBL);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillConvexPolygon(const IColor& color, float* x, float* y, int nPoints, const IBlend* pBlend) override
  {
    PathClear();
    PathConvexPolygon(x, y, nPoints);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend) override
  {
    PathClear();
    PathMoveTo(cx, cy);
    PathArc(cx, cy, r, a1, a2);
    PathClose();
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend) override
  {
    PathClear();
    PathCircle(cx, cy, r);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillEllipse(const IColor& color, const IRECT& bounds, const IBlend* pBlend) override
  {
    PathClear();
    PathEllipse(bounds);
    PathFill(color, IFillOptions(), pBlend);
  }
  
  void FillEllipse(const IColor& color, float x, float y, float r1, float r2, float angle, const IBlend* pBlend) override
  {
    PathClear();
    PathEllipse(x, y, r1, r2, angle);
    PathFill(color, IFillOptions(), pBlend);
  }

  bool HasPathSupport() const override { return true; }
  
  void PathTriangle(float x1, float y1, float x2, float y2, float x3, float y3) override
  {
    PathMoveTo(x1, y1);
    PathLineTo(x2, y2);
    PathLineTo(x3, y3);
    PathClose();
  }
  
  void PathRect(const IRECT& bounds) override
  {
    PathMoveTo(bounds.L, bounds.T);
    PathLineTo(bounds.R, bounds.T);
    PathLineTo(bounds.R, bounds.B);
    PathLineTo(bounds.L, bounds.B);
    PathClose();
  }
  
  void PathRoundRect(const IRECT& bounds, float ctl, float ctr, float cbl, float cbr) override
  {
    if (ctl <= 0.f && ctr <= 0.f && cbl <= 0.f && cbr <= 0.f)
    {
      PathRect(bounds);
    }
    else
    {
      const float y = bounds.B - bounds.H();
      PathMoveTo(bounds.L, y + ctl);
      PathArc(bounds.L + ctl, y + ctl, ctl, 270.f, 360.f);
      PathArc(bounds.L + bounds.W() - ctr, y + ctr, ctr, 0.f, 90.f);
      PathArc(bounds.L + bounds.W() - cbr, y + bounds.H() - cbr, cbr, 90.f, 180.f);
      PathArc(bounds.L + cbl, y + bounds.H() - cbl, cbl, 180.f, 270.f);
      PathClose();
    }
  }
  
  void PathRoundRect(const IRECT& bounds, float cr) override
  {
    PathRoundRect(bounds, cr, cr, cr, cr);
  }
  
  void PathEllipse(float x, float y, float r1, float r2, float angle = 0.0) override
  {
    PathTransformSave();
    
    if (r1 <= 0.0 || r2 <= 0.0)
      return;
    
    PathTransformTranslate(x, y);
    PathTransformRotate(angle);
    PathTransformScale(r1, r2);
    
    PathCircle(0.0, 0.0, 1.0);
    
    PathTransformRestore();
  }
  
  void PathEllipse(const IRECT& bounds) override
  {
    PathEllipse(bounds.MW(), bounds.MH(), bounds.W() / 2.f, bounds.H() / 2.f);
  }
  
  void PathCircle(float cx, float cy, float r) override
  {
    PathMoveTo(cx, cy - r);
    PathArc(cx, cy, r, 0.f, 360.f);
    PathClose();
  }
  
  void PathConvexPolygon(float* x, float* y, int nPoints) override
  {
    PathMoveTo(x[0], y[0]);
    for(int i = 1; i < nPoints; i++)
      PathLineTo(x[i], y[i]);
    PathClose();
  }
    
  void PathTransformSave() override
  {
    mTransformStates.push(mTransform);
  }
  
  void PathTransformRestore() override
  {
    if (!mTransformStates.empty())
    {
      mTransform = mTransformStates.top();
      mTransformStates.pop();
      PathTransformSetMatrix(mTransform);
    }
  }
  
  void PathTransformReset(bool clearStates = false) override
  {
    if (clearStates)
    {
      std::stack<IMatrix> newStack;
      mTransformStates.swap(newStack);
    }
    
    mTransform = IMatrix();
    PathTransformSetMatrix(mTransform);
  }
  
  void PathTransformTranslate(float x, float y) override
  {
    mTransform.Translate(x, y);
    PathTransformSetMatrix(mTransform);
  }
  
  void PathTransformScale(float scaleX, float scaleY) override
  {
    mTransform.Scale(scaleX, scaleY);
    PathTransformSetMatrix(mTransform);
  }
  
  void PathTransformScale(float scale) override
  {
    PathTransformScale(scale, scale);
  }
  
  void PathTransformRotate(float angle) override
  {
    mTransform.Rotate(angle);
    PathTransformSetMatrix(mTransform);
  }
    
  void PathTransformSkew(float xAngle, float yAngle) override
  {
    mTransform.Skew(xAngle, yAngle);
    PathTransformSetMatrix(mTransform);
  }

  void PathTransformMatrix(const IMatrix& matrix) override
  {
    mTransform.Transform(matrix);
    PathTransformSetMatrix(mTransform);
  }

  void PathClipRegion(const IRECT r = IRECT()) override
  {
    IRECT drawArea = mLayers.empty() ? mClipRECT : mLayers.top()->Bounds();
    IRECT clip = r.Empty() ? drawArea : r.Intersect(drawArea);
    PathTransformSetMatrix(IMatrix());
    SetClipRegion(clip);
    PathTransformSetMatrix(mTransform);
  }
  
  void DrawFittedBitmap(const IBitmap& bitmap, const IRECT& bounds, const IBlend* pBlend) override
  {
    PathTransformSave();
    PathTransformTranslate(bounds.L, bounds.T);
    IRECT newBounds(0., 0., static_cast<float>(bitmap.W()), static_cast<float>(bitmap.H()));
    PathTransformScale(bounds.W() / static_cast<float>(bitmap.W()), bounds.H() / static_cast<float>(bitmap.H()));
    DrawBitmap(bitmap, newBounds, 0, 0, pBlend);
    PathTransformRestore();
  }
  
  void DrawSVG(const ISVG& svg, const IRECT& dest, const IBlend* pBlend) override
  {
    float xScale = dest.W() / svg.W();
    float yScale = dest.H() / svg.H();
    float scale = xScale < yScale ? xScale : yScale;
    
    PathTransformSave();
    PathTransformTranslate(dest.L, dest.T);
    PathTransformScale(scale);
    RenderNanoSVG(svg.mImage);
    PathTransformRestore();
  }
  
  void DrawRotatedSVG(const ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend) override
  {
    PathTransformSave();
    PathTransformTranslate(destCtrX, destCtrY);
    PathTransformRotate((float) angle);
    DrawSVG(svg, IRECT(-width * 0.5f, - height * 0.5f, width * 0.5f, height * 0.5f), pBlend);
    PathTransformRestore();
  }

private:
  IPattern GetSVGPattern(const NSVGpaint& paint, float opacity)
  {
    int alpha = std::min(255, std::max(int(roundf(opacity * 255.f)), 0));
    
    switch (paint.type)
    {
      case NSVG_PAINT_COLOR:
        return IColor(alpha, (paint.color >> 0) & 0xFF, (paint.color >> 8) & 0xFF, (paint.color >> 16) & 0xFF);
        
      case NSVG_PAINT_LINEAR_GRADIENT:
      case NSVG_PAINT_RADIAL_GRADIENT:
      {
        NSVGgradient* pGrad = paint.gradient;
        
        IPattern pattern(paint.type == NSVG_PAINT_LINEAR_GRADIENT ? EPatternType::Linear : EPatternType::Radial);
        
        // Set Extend Rule
        switch (pGrad->spread)
        {
          case NSVG_SPREAD_PAD:       pattern.mExtend = EPatternExtend::Pad;       break;
          case NSVG_SPREAD_REFLECT:   pattern.mExtend = EPatternExtend::Reflect;   break;
          case NSVG_SPREAD_REPEAT:    pattern.mExtend = EPatternExtend::Repeat;    break;
        }
        
        // Copy Stops        
        for (int i = 0; i < pGrad->nstops; i++)
        {
          unsigned int color = pGrad->stops[i].color;
          pattern.AddStop(IColor(255, (color >> 0) & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF), pGrad->stops[i].offset);
        }
        
        // Copy transform        
        pattern.SetTransform(pGrad->xform[0], pGrad->xform[1], pGrad->xform[2], pGrad->xform[3], pGrad->xform[4], pGrad->xform[5]);
        
        return pattern;
      }
      default:
        return IColor(alpha, 0, 0, 0);
    }
  }
  
  void RenderNanoSVG(NSVGimage* pImage)
  {
    assert(pImage != nullptr);

    for (NSVGshape* pShape = pImage->shapes; pShape; pShape = pShape->next)
    {
      if (!(pShape->flags & NSVG_FLAGS_VISIBLE))
        continue;
      
      PathClear();
        
      for (NSVGpath* pPath = pShape->paths; pPath; pPath = pPath->next)
      {
        PathMoveTo(pPath->pts[0], pPath->pts[1]);
        
        for (int i = 1; i < pPath->npts; i += 3)
        {
          float *p = pPath->pts + i * 2;
          PathCubicBezierTo(p[0], p[1], p[2], p[3], p[4], p[5]);
        }
        
        if (pPath->closed)
          PathClose();
      }
      
      // Fill
      if (pShape->fill.type != NSVG_PAINT_NONE)
      {
        IFillOptions options;
        
        if (pShape->fillRule == NSVG_FILLRULE_EVENODD)
          options.mFillRule = EFillRule::EvenOdd;
        else
          options.mFillRule = EFillRule::Winding;
        
        options.mPreserve = pShape->stroke.type != NSVG_PAINT_NONE;
        PathFill(GetSVGPattern(pShape->fill, pShape->opacity), options, nullptr);
      }
      
      // Stroke
      if (pShape->stroke.type != NSVG_PAINT_NONE)
      {
        IStrokeOptions options;
        
        options.mMiterLimit = pShape->miterLimit;
        
        switch (pShape->strokeLineCap)
        {
          case NSVG_CAP_BUTT:   options.mCapOption = ELineCap::Butt;    break;
          case NSVG_CAP_ROUND:  options.mCapOption = ELineCap::Round;   break;
          case NSVG_CAP_SQUARE: options.mCapOption = ELineCap::Square;  break;
        }
        
        switch (pShape->strokeLineJoin)
        {
          case NSVG_JOIN_MITER:   options.mJoinOption = ELineJoin::Miter;   break;
          case NSVG_JOIN_ROUND:   options.mJoinOption = ELineJoin::Round;   break;
          case NSVG_JOIN_BEVEL:   options.mJoinOption = ELineJoin::Bevel;   break;
        }
        
        options.mDash.SetDash(pShape->strokeDashArray, pShape->strokeDashOffset, pShape->strokeDashCount);
        
        PathStroke(GetSVGPattern(pShape->stroke, pShape->opacity), pShape->strokeWidth, options, nullptr);
      }
    }
  }
  
protected:
    
  void DoTextRotation(const IText& text, const IRECT& bounds, const IRECT& rect)
  {
    if (!text.mAngle)
      return;
    
    IRECT rotated = rect;
    double tx, ty;
    
    CalulateTextRotation(text, bounds, rotated, tx, ty);
    PathTransformTranslate(tx, ty);
    PathTransformRotate(text.mAngle);
  }
  
  float GetBackingPixelScale() const override { return GetScreenScale() * GetDrawScale(); };

  IMatrix GetTransformMatrix() const { return mTransform; }
  
private:
  void PrepareRegion(const IRECT& r) override
  {
    PathTransformReset(true);
    PathClear();
    SetClipRegion(r);
    mClipRECT = r;
  }
  
  virtual void SetClipRegion(const IRECT& r) = 0;
  virtual void PathTransformSetMatrix(const IMatrix& matrix) = 0;

  IRECT mClipRECT;
  IMatrix mTransform;
  std::stack<IMatrix> mTransformStates;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
