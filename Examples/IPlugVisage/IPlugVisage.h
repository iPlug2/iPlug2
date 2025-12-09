#pragma once

#include "IPlug_include_in_plug_hdr.h"

using namespace iplug;

const int kNumPresets = 1;
enum EParams { kGain = 0, kNumParams };

class IPlugVisage final : public Plugin
{
public:
  IPlugVisage(const InstanceInfo& info);
  ~IPlugVisage() override { CloseWindow(); }

#if IPLUG_DSP
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
#endif
#if IPLUG_EDITOR
  void OnParamChangeUI(int paramIdx, EParamSource source = kUnknown) override;
  void OnUIClose() override;

protected:
  void OnDraw(visage::Canvas& canvas) override;
  void OnMouseDown(const visage::MouseEvent& e) override;
  void OnMouseDrag(const visage::MouseEvent& e) override;
  void OnMouseUp(const visage::MouseEvent& e) override;

private:
  void SetGainFromMouse(float x);
  bool mDragging = false;
#endif
};
