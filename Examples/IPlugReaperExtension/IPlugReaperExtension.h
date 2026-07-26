#pragma once

#include "ReaperExt_include_in_plug_hdr.h"

enum EControlTags
{
  kCtrlTagText = 0,
  kNumCtrlTags
};

using namespace iplug;
using namespace igraphics;

class IPlugReaperExtension : public ReaperExtBase
{
public:
  IPlugReaperExtension(reaper_plugin_info_t* pRec);
  void OnIdle() override;

private:
  int mPrevTrackCount = 0;
};

