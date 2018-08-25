#pragma once

#include "IControl.h"

// Performance display meter code, cribbed from NanoVG

class IPerfDisplayControl : public IControl, public IVectorBase
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
  
  IPerfDisplayControl(IGEditorDelegate& dlg, IRECT bounds, EStyle style = EStyle::kFPS, const char* label = "Frame Time")
  : IControl(dlg, bounds)
  , mStyle(style)
  , mNameLabel(label)
  {
    AttachIControl(this);
    
    mNameLabelText = IText(14, GetColor(kFR), DEFAULT_FONT, IText::kStyleNormal, IText::kAlignNear, IText::kVAlignTop);
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
    
    float x = mRECT.L;
    float y = mRECT.T;
    float w = mRECT.W();
    float h = mRECT.H();
    
    g.FillRect(GetColor(kBG), mRECT);
    
    // TODO: replace with IGraphics::DrawData, make it work with lice
    g.PathStart();
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

    if (mNameLabel.GetLength())
      g.DrawText(mNameLabelText, mNameLabel.Get(), mRECT);
    
    WDL_String str;

    if (mStyle == kFPS)
    {
      str.SetFormatted(32, "%.2f FPS\n", 1.0f / avg);
      g.DrawText(mTopLabelText, str.Get(), mRECT);

      str.SetFormatted(32, "%.2f ms", avg * 1000.0f);
      g.DrawText(mBottomLabelText, str.Get(), mRECT);
    }
    else if (mStyle == kPercentage)
    {
      str.SetFormatted(32, "%.1f %%", avg * 1.0f);
      g.DrawText(mTopLabelText, str.Get(), mRECT);
    }
    else
    {
      str.SetFormatted(32, "%.2f ms", avg * 1000.0f);
      g.DrawText(mTopLabelText, str.Get(), mRECT);
    }
  }
private:
  int mStyle;
  WDL_String mNameLabel;
  float mBuffer[MAXBUF] = {};
  int mReadPos = 0;

  float mPadding = 1.f;
  IText& mNameLabelText = mText;
  IText mTopLabelText = IText(18, GetColor(kFR), DEFAULT_FONT, IText::kStyleNormal, IText::kAlignFar, IText::kVAlignTop);
  IText mBottomLabelText = IText(15, GetColor(kFR), DEFAULT_FONT, IText::kStyleNormal, IText::kAlignFar, IText::kVAlignBottom);
};

