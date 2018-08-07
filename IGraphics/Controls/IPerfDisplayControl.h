#pragma once

#pragma once

#include "IControl.h"

#define MAXBUF 100

class IPerfDisplayControl : public IControl, public IVectorBase
{
public:
  
  enum EGraphStyle {
    GRAPH_RENDER_FPS,
    GRAPH_RENDER_MS,
    GRAPH_RENDER_PERCENT,
  };
  
  struct Data
  {
    EGraphStyle style = GRAPH_RENDER_FPS;
    char name[32];
    float vals[MAXBUF] = {};
    int head = 0;
  };

  
  IPerfDisplayControl(IGEditorDelegate& dlg, IRECT bounds)
  : IControl(dlg, bounds)
  {
    AttachIControl(this);
  }
  
  void Update(float frameTime)
  {
    mBuf.head = (mBuf.head+1) % MAXBUF;
    mBuf.vals[mBuf.head] = frameTime;
  }
  
  float GetAverage()
  {
    float avg = 0.f;
    for (int i = 0; i < MAXBUF; i++) {
      avg += mBuf.vals[i];
    }
    return avg / (float)MAXBUF;
  }
  
  void Draw(IGraphics& g) override
  {
    float avg = GetAverage();
  
    g.FillRect(GetColor(kBG), mRECT);
    
    IRECT r = mRECT.GetPadded(-mPadding);
    
    const float maxY = (r.H() / 2.f); // y +/- centre
    
    float xPerData = r.W() / (float) MAXBUF;
    
    float xHi = 0.f;
    float yHi = mBuf.vals[0] * maxY;
    yHi = Clip(yHi, -maxY, maxY);
    
    for (int s = 1; s < MAXBUF; s++)
    {
      float xLo = xHi, yLo = yHi;
      xHi = ((float) s * xPerData);
      yHi = mBuf.vals[s] * maxY;
      yHi = Clip(yHi, -maxY, maxY);
      g.DrawLine(GetColor(kFG), r.L + xLo, r.MH() - yLo, r.L + xHi, r.MH() - yHi);
    }
  }
private:
  Data mBuf;
  float mPadding = 1.f;
};

