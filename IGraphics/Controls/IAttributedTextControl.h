/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IAboutBoxControl
 */

#include "IControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/* An IControl for drawing attributed text to a layer using the native text routines, e.g. CoreGraphics */

class IAttributedTextControl : public IEditableTextControl
{
public:
  IAttributedTextControl(const IRECT& bounds, const char* str, const IText& text = DEFAULT_TEXT)
  : IEditableTextControl(bounds, str, text)
  {
  }
  
  int DrawAttributedText(IGraphics& g, const char* str, const IRECT& textArea, const IText& text)
  {
    auto* vg = static_cast<NVGcontext*>(g.GetDrawContext());
    
    float w, h;
    auto imageData = g.DrawAttributedText(str, text, w, h);
    int imgID = nvgCreateImageRGBA(vg, ceil(w * g.GetTotalScale()), ceil(h * g.GetTotalScale()), 0, imageData.Get());
    
    float x, y;

    switch (text.mAlign)
    {
      case EAlign::Near:
        x = textArea.L;
        break;
      case EAlign::Center:
        x = textArea.MW() - (w / 2.0);
        break;
      case EAlign::Far:
        x = textArea.R;
        break;
    }
    
    switch (text.mVAlign)
    {
      case EVAlign::Top:
        y = textArea.T;
        break;
      case EVAlign::Middle:
        y = textArea.MH() - (h / 2.0);
        break;
      case EVAlign::Bottom:
        y = textArea.B - h;
        break;
    }
    
    nvgBeginPath(vg);
    nvgRect(vg, x, y, w, h);
    NVGpaint imgPaint = nvgImagePattern(vg, x, y, w, h, 0, imgID, 1.f);
    nvgFillPaint(vg, imgPaint);
    nvgFill(vg);

    // Clean up
    // nvgDeleteImage(vg, imgID);
    
    return imgID;
  }

  void Draw(IGraphics& g) override
  {
    int imgID = -1;
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(this, mRECT);
      imgID = DrawAttributedText(g, mStr.Get(), mRECT.GetPadded(-10.f), mText);
      mLayer = g.EndLayer();
    }

    g.DrawLayer(mLayer, &mBlend);
    
    if (imgID > -1)
    {
      auto* vg = static_cast<NVGcontext*>(g.GetDrawContext());
      nvgDeleteImage(vg, imgID);
    }
  }
  
  void OnRescale() override
  {
    if (mLayer)
      mLayer->Invalidate();
  }

  void OnResize() override
  {
    if (mLayer)
      mLayer->Invalidate();
  }

  void SetStr(const char* str) override
  {
    ITextControl::SetStr(str);
    mLayer->Invalidate();
  }
private:
  ILayerPtr mLayer;
};


END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
