#pragma once

#include "ReaperExt_include_in_plug_hdr.h"

enum EControlTags
{
  kCtrlTagText = 0,
  kNumControlTags
};

class ReaperExtension : public ReaperExtBase
{
public:
  ReaperExtension(reaper_plugin_info_t* pRec);
  void OnIdle() override;
  void OnUIClose() override { mGUIToggle = 0; }
  
private:
  int mPrevTrackCount = 0;
  int mGUIToggle = 0;
};

