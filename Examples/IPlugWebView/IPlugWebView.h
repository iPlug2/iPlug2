#pragma once

#include "IPlug_include_in_plug_hdr.h"

const int kNumPrograms = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

enum EMsgTags
{
  kMsgTagButton = 0,
};

class IPlugWebView : public IPlug
{
public:
  IPlugWebView(IPlugInstanceInfo instanceInfo);
  
  bool OnMessage(int messageTag, int controlTag, int dataSize, const void* pData) override;
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
};
