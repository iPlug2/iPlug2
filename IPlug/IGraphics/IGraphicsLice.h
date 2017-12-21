#pragma once

#include "IControl.h"
#include "lice.h"
#include "lice_text.h"

#ifdef OS_OSX
#include <CoreGraphics/CoreGraphics.h>
#include "swell.h"
#endif

class IGraphicsLice : public IGraphics
{
public:
  IGraphicsLice(IPlugBaseGraphics& plug, int w, int h, int fps);
  ~IGraphicsLice();

  void PrepDraw() override;
  void ReScale() override;

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IChannelBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IChannelBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IChannelBlend* pBlend) override;
  void DrawPoint(const IColor& color, float x, float y, const IChannelBlend* pBlend, bool aa) override;
  void ForcePixel(const IColor& color, int x, int y) override;
  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend, bool aa) override;
  void DrawTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend) override;
  void DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle,  const IChannelBlend* pBlend, bool aa) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r,const IChannelBlend* pBlend, bool aa) override;
  void DrawRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa) override;
  void FillCircle(const IColor& color, int cx, int cy, float r, const IChannelBlend* pBlend, bool aa) override;
  void FillIRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend) override;

  void FillRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa) override;
  void FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IChannelBlend* pBlend) override;
  void FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return mDrawBitmap->getBits(); }

  bool DrawIText(const IText& text, const char* str, IRECT& rect, bool measure) override;
  bool MeasureIText(const IText& text, const char* str, IRECT& destRect) override;
  
  IBitmap LoadIBitmap(const char* name, int nStates, bool framesAreHoriztonal, double scale) override;
  IBitmap ScaleIBitmap(const IBitmap& bitmap, const char* name, double targetScale) override;
  IBitmap CropIBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, double targetScale) override;
  void ReleaseIBitmap(IBitmap& bitmap) override;
  void RetainIBitmap(IBitmap& bitmap, const char * cacheName) override;
  
  inline LICE_SysBitmap* GetDrawBitmap() const { return mDrawBitmap; }

protected:
  void RenderAPIBitmap(void* pContext) override;
  
private:
#ifdef OS_OSX
  LICE_IBitmap* LoadAPIBitmap(const char* path);
  CGColorSpaceRef mColorSpace;
  
#endif
  LICE_SysBitmap* mDrawBitmap;
  LICE_IFont* CacheFont(IText& text);
  LICE_MemBitmap* mTmpBitmap;
};
