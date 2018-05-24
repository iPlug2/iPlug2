#pragma once

#pragma once

#include "IControl.h"
#include "IPlugQueue.h"

template <int MAXNC = 1, int MAXBUF = 128>
class IVScopeControl : public IControl
                     , public IVectorBase
{
public:
  static constexpr int kUpdateMessage = 0;
  
  struct Data
  {
    int nchans = MAXNC;
    float vals[MAXNC][MAXBUF] = {};
  };
  
  class IVScopeBallistics
  {
  public:
    IVScopeBallistics(int controlTag)
    : mControlTag(controlTag)
    {
    }
    
    void ProcessBlock(sample** inputs, int nFrames)
    {
      for (auto s = 0; s < nFrames; s++)
      {
        if(mBufCount == MAXBUF)
        {
          mQueue.Push(mBuf);
          mBufCount = 0;
        }
        
        for (auto c = 0; c < MAXNC; c++)
        {
          mBuf.vals[c][mBufCount] = (float) inputs[c][s];
        }
        
        mBufCount++;
      }
    }
    
    // this must be called on the main thread - typically in MyPlugin::OnIdle()
    void TransmitData(IDelegate& dlg)
    {
      Data d;

      while(mQueue.ElementsAvailable())
      {
        mQueue.Pop(d);
        dlg.SendControlMessageFromDelegate(mControlTag, kUpdateMessage, sizeof(Data), (void*) &d);
      }
    }
    
  private:
    Data mBuf;
    int mControlTag;
    int mBufCount = 0;
    IPlugQueue<Data> mQueue { 1024 };
  };
  
  IVScopeControl(IDelegate& dlg, IRECT bounds, const char* trackNames = 0, ...)
  : IControl(dlg, bounds, MAXNC)
  {
  }
  
  virtual void Draw(IGraphics& g) override
  {
    g.FillRect(GetColor(kBG), mRECT);
    
    IRECT r = mRECT.GetPadded(-mPadding);
    
    const float maxY = (r.H() / 2.f); // y +/- centre
    
    float xPerData = r.W() / (float) MAXBUF;
    
    for (auto c = 0; c < mBuf.nchans; c++) {
      float xHi = 0.f;
      float yHi = mBuf.vals[c][0] * maxY;
      yHi = Clip(yHi, -maxY, maxY);
      
      for (int s = 1; s < MAXBUF; s++)
      {
        float xLo = xHi, yLo = yHi;
        xHi = ((float) s * xPerData);
        yHi = mBuf.vals[c][s] * maxY;
        yHi = Clip(yHi, -maxY, maxY);
        g.DrawLine(GetColor(kFG), r.L + xLo, r.MH() - yLo, r.L + xHi, r.MH() - yHi);
      }
    }
  }

  void OnDataFromDelegate(int messageTag, int dataSize, const void* pData) override
  {
    IByteChunk chnk;
    chnk.PutBytes(pData, dataSize); // unnessecary copy
    
    int pos = chnk.Get(&mBuf.nchans, 0);
    
    while(pos < chnk.Size())
    {
      for (auto ch = 0; ch < mBuf.nchans; ch++) {
        for (auto s = 0; s < MAXBUF; s++) {
          pos = chnk.Get(&mBuf.vals[ch][s], pos);
        }
      }
    }
    
    SetDirty(false);
  }
  
private:
  Data mBuf;
  float mPadding = 2.f;
};

