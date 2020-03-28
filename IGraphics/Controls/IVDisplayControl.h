/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 */

#include "IControl.h"
#include "ISender.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/**  */
class IVDisplayControl : public IControl
                       , public IVectorBase
{
public:
  static constexpr int MAX_BUFFER_SIZE = 2048;
  
  IVDisplayControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE, EDirection dir = EDirection::Vertical, float lo = 0., float hi = 1.f, float defaultVal = 0., uint32_t bufferSize = 100, float strokeThickness = 2.f)
  : IControl(bounds)
  , IVectorBase(style)
  , mLoValue(lo)
  , mHiValue(hi)
  , mBuffer(bufferSize, defaultVal)
  , mDirection(dir)
  , mStrokeThickness(strokeThickness)
  {
    assert(bufferSize > 0 && bufferSize < MAX_BUFFER_SIZE);

    AttachIControl(this, label);
  }

  void Update(float v)
  {
    mBuffer[mReadPos] = v;
    mReadPos = (mReadPos+1) % mBuffer.size();
    SetDirty(false);
  }
  
  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    
    mPlotBounds = mWidgetBounds.GetPadded(mDirection == EDirection::Horizontal ? 0.f : -mStrokeThickness,
                                mDirection == EDirection::Horizontal ? -mStrokeThickness : 0.f,
                                mDirection == EDirection::Horizontal ? 0.f : -mStrokeThickness,
                                mDirection == EDirection::Horizontal ? -mStrokeThickness : 0.f);
    
    SetDirty(false);
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackGround(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    
    if(mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  void DrawWidget(IGraphics& g) override
  {
    float x = mPlotBounds.L;
    float y = mPlotBounds.T;
    float w = mPlotBounds.W();
    float h = mPlotBounds.H();
    
    const int sz = static_cast<int>(mBuffer.size());

    auto getPlotPos = [&](int pos, float axis, float extrem) {
      float v = mBuffer[(mReadPos+pos) % sz];
      v = (v - mLoValue) / (mHiValue - mLoValue);
      return axis + extrem - (v * extrem);
    };
    
    if(mDirection == EDirection::Horizontal)
    {
      g.PathMoveTo(x, getPlotPos(0, y, h));
      
      for (int i = 0; i < sz; i++)
      {
        float vx = x + ((float)i/(sz-1)) * w;
        float vy = getPlotPos(i, y, h);
        g.PathLineTo(vx, vy);
      }
    }
    else
    {
      g.PathMoveTo(getPlotPos(0, x, w), y);
      
      for (int i = 0; i < sz; i++)
      {
        float vx = getPlotPos(i, x, w);
        float vy = y + ((float)i/(sz-1)) * h;
        g.PathLineTo(vx, vy);
      }
    }
    
    g.PathClipRegion();
    
    g.PathStroke(IPattern::CreateLinearGradient(mPlotBounds, mDirection, {{COLOR_TRANSPARENT, 0.f}, {GetColor(kX1), 1.f}}), mStrokeThickness, IStrokeOptions(), &mBlend);
  }
  
  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      IByteStream stream(pData, dataSize);

      int pos = 0;
      ISenderData<1> d;
      pos = stream.Get(&d, pos);
      Update(d.vals[0]);
      SetDirty(false);
    }
  }
  
private:
  std::vector<float> mBuffer;
  float mLoValue = 0.f;
  float mHiValue = 1.f;
  int mReadPos = 0;
  float mStrokeThickness = 2.f;
  EDirection mDirection;
  IRECT mPlotBounds;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
