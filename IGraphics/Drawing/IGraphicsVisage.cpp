/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IGraphicsVisage.h"
#include "visage/visage_graphics/renderer.h"

#include <iostream>

using namespace iplug;
using namespace igraphics;

IGraphicsVisage::IGraphicsVisage(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphics(dlg, w, h, fps, scale)
{
  // Canvas is default constructed
}

IGraphicsVisage::~IGraphicsVisage()
{
  // Canvas will be automatically cleaned up
}

const char* IGraphicsVisage::GetDrawingAPIStr()
{
  return "VISAGE";
}

void IGraphicsVisage::BeginFrame()
{
}

void IGraphicsVisage::EndFrame()
{
  mCanvas.submit();
}

void IGraphicsVisage::OnViewInitialized(void* pContext)
{
  visage::Renderer::instance().checkInitialization(GetWindow(), nullptr);
  mCanvas.setDimensions(Width(), Height());
  mCanvas.pairToWindow(GetWindow(), Width(), Height());
}

void IGraphicsVisage::OnViewDestroyed()
{
  visage::Renderer::instance().stop();
}

void IGraphicsVisage::DrawResize()
{
  mCanvas.setDimensions(Width(), Height());
}

void IGraphicsVisage::DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
}

void IGraphicsVisage::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness)
{
  mCanvas.setColor(FromIColor(color, pBlend));
  mCanvas.segment(x1, y1, x2, y2, thickness * 0.5f, true);
}

void IGraphicsVisage::DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness, float dashLen)
{
  // TODO: implement dashed line using multiple segments
  DrawLine(color, x1, y1, x2, y2, pBlend, thickness);
}

void IGraphicsVisage::DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness)
{
  mCanvas.setColor(FromIColor(color, pBlend));
  mCanvas.rectangleBorder(bounds.L, bounds.T, bounds.W(), bounds.H(), thickness);
}

void IGraphicsVisage::DrawRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius, const IBlend* pBlend, float thickness)
{
  mCanvas.setColor(FromIColor(color, pBlend));
  mCanvas.roundedRectangleBorder(bounds.L, bounds.T, bounds.W(), bounds.H(), cornerRadius, thickness);
}

void IGraphicsVisage::DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend, float thickness)
{
  mCanvas.setColor(FromIColor(color, pBlend));
  mCanvas.ring(cx - r, cy - r, r * 2.f, thickness);
}

void IGraphicsVisage::DrawArc(const IColor& color, float cx, float cy, float r, float a1, float a2, const IBlend* pBlend, float thickness)
{
  mCanvas.setColor(FromIColor(color, pBlend));
  mCanvas.arc(cx - r, cy - r, r * 2.f, thickness, a1, a2 - a1, true);
}

void IGraphicsVisage::FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend)
{
  mCanvas.setColor(FromIColor(color, pBlend));
  mCanvas.rectangle(bounds.L, bounds.T, bounds.W(), bounds.H());
}

void IGraphicsVisage::FillRoundRect(const IColor& color, const IRECT& bounds, float cornerRadius, const IBlend* pBlend)
{
  mCanvas.setColor(FromIColor(color, pBlend));
  mCanvas.roundedRectangle(bounds.L, bounds.T, bounds.W(), bounds.H(), cornerRadius);
}

void IGraphicsVisage::FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend)
{
  mCanvas.setColor(FromIColor(color, pBlend));
  mCanvas.circle(cx - r, cy - r, r * 2.f);
}

void IGraphicsVisage::DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness, float dashLen)
{
  // TODO: implement dashed rect using multiple segments
  DrawRect(color, bounds, pBlend, thickness);
}

void IGraphicsVisage::DrawFastDropShadow(const IRECT& innerBounds, const IRECT& outerBounds, float xyDrop, float roundness, float blur, IBlend* pBlend)
{
  mCanvas.roundedRectangleShadow(innerBounds.L, innerBounds.T, innerBounds.W(), innerBounds.H(), roundness, blur);
}

void IGraphicsVisage::DrawMultiLineText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend)
{
}

void IGraphicsVisage::PathClear()
{
}

void IGraphicsVisage::PathClose()
{
}

void IGraphicsVisage::PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding)
{
}

void IGraphicsVisage::PathMoveTo(float x, float y)
{
}

void IGraphicsVisage::PathLineTo(float x, float y)
{
}

void IGraphicsVisage::PathCubicBezierTo(float c1x, float c1y, float c2x, float c2y, float x2, float y2)
{
}

void IGraphicsVisage::PathQuadraticBezierTo(float cx, float cy, float x2, float y2)
{
}

void IGraphicsVisage::PathSetWinding(bool clockwise)
{
}

void IGraphicsVisage::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
}

void IGraphicsVisage::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
}

IColor IGraphicsVisage::GetPoint(int x, int y)
{
  return IColor();
}

IBitmap IGraphicsVisage::LoadBitmap(const char* name, int nStates, bool framesAreHorizontal, int targetScale)
{
  return IBitmap();
}

void IGraphicsVisage::ReleaseBitmap(const IBitmap& bitmap)
{
}

void IGraphicsVisage::RetainBitmap(const IBitmap& bitmap, const char* cacheName)
{
}

bool IGraphicsVisage::BitmapExtSupported(const char* ext)
{
  return false;
}

APIBitmap* IGraphicsVisage::LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext)
{
  return nullptr;
}

APIBitmap* IGraphicsVisage::LoadAPIBitmap(const char* name, const void* pData, int dataSize, int scale)
{
  return nullptr;
}

APIBitmap* IGraphicsVisage::CreateAPIBitmap(int width, int height, float scale, double drawScale, bool cacheable)
{
  return nullptr;
}

bool IGraphicsVisage::LoadAPIFont(const char* fontID, const PlatformFontPtr& font)
{
  return false;
}

void IGraphicsVisage::GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data)
{
}

void IGraphicsVisage::ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow)
{
}

float IGraphicsVisage::DoMeasureText(const IText& text, const char* str, IRECT& bounds) const
{
  return 0.f;
}

void IGraphicsVisage::DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend)
{
}

void IGraphicsVisage::PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double& y) const
{
}

void IGraphicsVisage::PathTransformSetMatrix(const IMatrix& m)
{
}

void IGraphicsVisage::SetClipRegion(const IRECT& r)
{
}

void IGraphicsVisage::UpdateLayer()
{
}
