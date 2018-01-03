#pragma once

#include "IControl.h"

#include "nanovg.h"
#include "nanovg_mtl.h"

class IGraphicsNanoVG : public IGraphics
{
public:
  const char* GetDrawingAPIStr() override { return "NANOVG"; }

  IGraphicsNanoVG(IPlugBaseGraphics& plug, int w, int h, int fps);
  ~IGraphicsNanoVG();

  void BeginFrame() override;
  void EndFrame() override;
  void ViewInitialized(void* layer) override;
  
  void PrepDraw() override;
  void ReScale() override;

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend) override;
  void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend, bool aa) override;
  void ForcePixel(const IColor& color, int x, int y) override;
  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, bool aa) override;
  void DrawVerticalLine(const IColor& color, int xi, int yLo, int yHi);
  void DrawHorizontalLine(const IColor& color, int yi, int xLo, int xHi);
  void DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle,  const IBlend* pBlend, bool aa) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r,const IBlend* pBlend, bool aa) override;
  void DrawTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IBlend* pBlend = nullptr) override {}
  void DrawRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend, int cr, bool aa) override;

  void FillCircle(const IColor& color, int cx, int cy, float r, const IBlend* pBlend, bool aa) override;
  void FillIRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void FillRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend, int cr, bool aa) override;
  void FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IBlend* pBlend) override;
  void FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IBlend* pBlend) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return (void*) mVG; }

  bool DrawIText(const IText& text, const char* str, IRECT& rect, bool measure) override;
  bool MeasureIText(const IText& text, const char* str, IRECT& destRect) override;
  
  IBitmap LoadIBitmap(const char* name, int nStates, bool framesAreHoriztonal, double scale) override;
  IBitmap ScaleIBitmap(const IBitmap& bitmap, const char* name, double targetScale) override;
  IBitmap CropIBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, double targetScale) override;
  void ReleaseIBitmap(IBitmap& bitmap) override;
  void RetainIBitmap(IBitmap& bitmap, const char * cacheName) override;
  
protected:
//  IBitmap CreateIBitmap(const char * cacheName, int w, int h) override {}
  void RenderAPIBitmap(void* pContext) override;
  
  NVGcontext* mVG = nullptr;
};

inline NVGcolor NanoVGColor(const IColor& color)
{
  NVGcolor c;
  c.r = (float) color.R / 255.0f;
  c.g = (float) color.G / 255.0f;
  c.b = (float) color.B / 255.0f;
  c.a = (float) color.A / 255.0f;
  return c;
}
