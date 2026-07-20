#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include <visage/app.h>
#include <visage_graphics/post_effects.h>
#include "showcase.h"

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;

class IPlugVisage final : public Plugin
{
public:
  IPlugVisage(const InstanceInfo& info);
  
  void OnIdle() override;

  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  void OnParentWindowResize(int width, int height) override;

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  
private:
  std::unique_ptr<visage::ApplicationEditor> mEditor;
  std::unique_ptr<visage::Window> mWindow;
  std::unique_ptr<Showcase> mShowcase;
};
