/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup SpecialControls
 * @copydoc IFPSDisplayControl
 */

#include "IControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Performance display meter, based on code from NanoVG
 *  This is a special control that lives outside the main IGraphics control stack.
 * @ingroup SpecialControls */
class IFPSDisplayControl : public IControl
                         , public IVectorBase
{
private:
  static constexpr int MAXBUF = 100;
public:
  enum EStyle
  {
    kFPS,
    kMS,
    kPercentage,
    kNumStyles
  };

  IFPSDisplayControl(const IRECT& bounds, EStyle style = EStyle::kFPS, const char* label = "Frame Time")
  : IControl(bounds)
  , IVectorBase(DEFAULT_STYLE)
  , mStyle(style)
  , mNameLabel(label)
  {
    AttachIControl(this, label);

    SetColor(kBG, COLOR_WHITE);

    mNameLabelText = IText(14, GetColor(kFR), DEFAULT_FONT, EAlign::Near, EVAlign::Bottom);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mStyle++;

    if(mStyle == kNumStyles)
      mStyle = kFPS;
  }

  bool IsDirty() override
  {
    return true;
  }

  void Update(float frameTime)
  {
    mReadPos = (mReadPos+1) % MAXBUF;
    mBuffer[mReadPos] = frameTime;
  }

  void Draw(IGraphics& g) override
  {
    float avg = 0.f;
    for (int i = 0; i < MAXBUF; i++)
      avg += mBuffer[i];

    avg = avg / (float)MAXBUF;

    g.FillRect(GetColor(kBG), mRECT);
    g.DrawRect(COLOR_BLACK, mRECT);

    IRECT padded = mRECT.GetPadded(-2);

    float x = padded.L;
    float y = padded.T;
    float w = padded.W();
    float h = padded.H();

    // TODO: replace with IGraphics::DrawData, make it work with lice

    g.PathMoveTo(x, y+h);

    if (mStyle == kFPS)
    {
      for (int i = 0; i < MAXBUF; i++) {
        float v = 1.0f / (0.00001f + mBuffer[(mReadPos+i) % MAXBUF]);
        float vx, vy;
        if (v > 80.0f) v = 80.0f;
        vx = x + ((float)i/(MAXBUF-1)) * w;
        vy = y + h - ((v / 80.0f) * h);
        g.PathLineTo(vx, vy);
      }
    }
    else if (mStyle == kPercentage)
    {
      for (int i = 0; i < MAXBUF; i++) {
        float v = mBuffer[(mReadPos+i) % MAXBUF] * 1.0f;
        float vx, vy;
        if (v > 100.0f) v = 100.0f;
        vx = x + ((float)i/(MAXBUF-1)) * w;
        vy = y + h - ((v / 100.0f) * h);
        g.PathLineTo(vx, vy);
      }
    }
    else
    {
      for (int i = 0; i < MAXBUF; i++) {
        float v = mBuffer[(mReadPos+i) % MAXBUF] * 1000.0f;
        float vx, vy;
        if (v > 20.0f) v = 20.0f;
        vx = x + ((float)i/(MAXBUF-1)) * w;
        vy = y + h - ((v / 20.0f) * h);
        g.PathLineTo(vx, vy);
      }
    }

    g.PathLineTo(mRECT.R, mRECT.B);
    g.PathFill(GetColor(kFG));

    g.DrawText(mAPILabelText, g.GetDrawingAPIStr(), padded);

    if (mNameLabel.GetLength())
      g.DrawText(mNameLabelText, mNameLabel.Get(), padded);

    WDL_String str;

    if (mStyle == kFPS)
    {
      str.SetFormatted(32, "%.2f FPS", 1.0f / avg);
      g.DrawText(mTopLabelText, str.Get(), padded);

      str.SetFormatted(32, "%.2f ms", avg * 1000.0f);
      g.DrawText(mBottomLabelText, str.Get(), padded);
    }
    else if (mStyle == kPercentage)
    {
      str.SetFormatted(32, "%.1f %%", avg * 1.0f);
      g.DrawText(mTopLabelText, str.Get(), padded);
    }
    else
    {
      str.SetFormatted(32, "%.2f ms", avg * 1000.0f);
      g.DrawText(mTopLabelText, str.Get(), padded);
    }
  }
private:
  int mStyle;
  WDL_String mNameLabel;
  float mBuffer[MAXBUF] = {};
  int mReadPos = 0;

  float mPadding = 1.f;
  IText& mNameLabelText = mText;
  IText mAPILabelText = IText(14, GetColor(kFR), DEFAULT_FONT, EAlign::Near, EVAlign::Top);
  IText mTopLabelText = IText(18, GetColor(kFR), DEFAULT_FONT, EAlign::Far, EVAlign::Top);
  IText mBottomLabelText = IText(15, GetColor(kFR), DEFAULT_FONT, EAlign::Far, EVAlign::Bottom);
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
