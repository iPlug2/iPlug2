#pragma once

#include "IControl.h"
#include "cairo/cairo.h"

class IGraphicsCairo : public IGraphics
{
public:
  IGraphicsCairo(IPlugBaseGraphics& plug, int w, int h, int fps);
  ~IGraphicsCairo();

  void PrepDraw() override;
  void ReScale() override;

  bool DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IChannelBlend* pBlend) override;
  bool DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IChannelBlend* pBlend) override;
  bool DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IChannelBlend* pBlend) override;
  bool DrawPoint(const IColor& color, float x, float y, const IChannelBlend* pBlend, bool aa) override;
  bool ForcePixel(const IColor& color, int x, int y) override;
  bool DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend, bool aa) override;
  bool DrawVerticalLine(const IColor& color, int xi, int yLo, int yHi);
  bool DrawHorizontalLine(const IColor& color, int yi, int xLo, int xHi);
  bool DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle,  const IChannelBlend* pBlend, bool aa) override;
  bool DrawCircle(const IColor& color, float cx, float cy, float r,const IChannelBlend* pBlend, bool aa) override;
  bool FillCircle(const IColor& color, int cx, int cy, float r, const IChannelBlend* pBlend, bool aa) override;
  bool FillIRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend) override;
  bool RoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa) override;
  bool FillRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa) override;
  bool FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IChannelBlend* pBlend) override;
  bool FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return nullptr; } //TODO

  bool DrawIText(const IText& text, const char* str, IRECT& rect, bool measure) override;
  bool MeasureIText(const IText& text, const char* str, IRECT& destRect) override;
  
  IBitmap LoadIBitmap(const char* name, int nStates, bool framesAreHoriztonal, double scale) override;
  IBitmap ScaleIBitmap(const IBitmap& bitmap, const char* name, double targetScale) override;
  IBitmap CropIBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, double targetScale) override;
  void ReleaseIBitmap(IBitmap& bitmap) override;
  void RetainIBitmap(IBitmap& bitmap, const char * cacheName) override;
  
protected:
  IBitmap CreateIBitmap(const char * cacheName, int w, int h) override {}
  void RenderAPIBitmap(void* pContext) override;
  cairo_t *mContext;
  cairo_surface_t *mSurface;
};
