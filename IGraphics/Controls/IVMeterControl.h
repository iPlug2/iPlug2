#pragma once

#include "IControl.h"
#include "IPlugQueue.h"

template <int MAXNC = 1>
class IVMeterControl : public IVTrackControlBase
{
public:
  static constexpr int kUpdateMessage = 0;
  
  struct Data
  {
    int nchans = MAXNC;
    float vals[MAXNC] = {};
  };
  
  class IVMeterBallistics
  {
  public:
    IVMeterBallistics(int controlTag)
    : mControlTag(controlTag)
    {
    }
    
    void ProcessBlock(sample** inputs, int nFrames)
    {
      Data d;
      
      for (auto s = 0; s < nFrames; s++)
      {
        for (auto c = 0; c < MAXNC; c++)
        {
          d.vals[c] += std::fabs((float) inputs[c][s]);
        }
      }
      
      for (auto c = 0; c < MAXNC; c++)
      {
        d.vals[c] /= (float) nFrames;
      }

      mQueue.Push(d); // TODO: expensive?
    }
    
    // this must be called on the main thread - typically in MyPlugin::OnIdle()
    void TransmitData(IGEditorDelegate& dlg)
    {
      while(mQueue.ElementsAvailable())
      {
        Data d;
        mQueue.Pop(d);
        dlg.SendControlMsgFromDelegate(mControlTag, kUpdateMessage, sizeof(Data), (void*) &d);
      }
    }
    
  private:
    int mControlTag;
    IPlugQueue<Data> mQueue { 1024 };
  };
  
  IVMeterControl(IGEditorDelegate& dlg, IRECT bounds, const char* trackNames = 0, ...)
  : IVTrackControlBase(dlg, bounds, MAXNC, 0, 1., trackNames)
  {
  }
  
  //  void OnResize() override;
  //  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;
  //  void OnMouseDown(float x, float y, const IMouseMod& mod) override;
  
  void OnMsgFromDelegate(int messageTag, int dataSize, const void* pData) override
  {
    IByteChunk chnk;
    chnk.PutBytes(pData, dataSize); // unnessecary copy
    
    int pos = 0;
    Data data;
    pos = chnk.Get(&data.nchans, pos);

    while(pos < chnk.Size())
    {
      for (auto i = 0; i < data.nchans; i++) {
        pos = chnk.Get(&data.vals[i], pos);
        float* pVal = GetTrackData(i);
        *pVal = Clip(data.vals[i], 0.f, 1.f);
      }
    }
    
    SetDirty(false);
  }
};
