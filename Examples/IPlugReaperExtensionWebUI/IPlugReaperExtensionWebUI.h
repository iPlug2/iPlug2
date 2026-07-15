#pragma once

#include "ReaperExt_include_in_plug_hdr.h"

enum EMsgTags
{
  kMsgTagAddTrack = 0,
  kMsgTagProcessItem = 1,
  kMsgTagToggleDock = 2,
  kMsgTagSetGain = 3, // gain (0..100) carried in ctrlTag; keeps mGain current as the slider moves
  kNumMsgTags
};

using namespace iplug;

class IPlugReaperExtensionWebUI : public ReaperExtBase
{
public:
  IPlugReaperExtensionWebUI(reaper_plugin_info_t* pRec);
  void OnIdle() override;

  // Messages sent from the WebView UI arrive here
  void SendArbitraryMsgFromUI(int msgTag, int ctrlTag, int dataSize, const void* pData) override;

  // Called once the WebView content has loaded - push current state to the fresh UI
  void OnUIOpen() override;

  // hookpostcommand: react to project edits immediately (no polling lag)
  void OnActionRun(int commandId, int flag) override;

  // projectconfig: persist the gain in the .RPP
  void SaveProjectState(ProjectStateContext* ctx) override;
  bool LoadProjectStateLine(const char* line) override;
  void OnBeginLoadProjectState(bool isUndo) override;

private:
  // Offline-process the first selected media item, applying gain (0..1), writing a
  // new file next to the original and inserting it into the project.
  void ProcessSelectedItem(double gain);

  void PushTrackCount(bool force); // send track count to the WebView (if changed or forced)
  void PushGain();                 // send the current gain to the WebView slider
  void PushDockState();            // tell the WebView whether the window is docked

  int mPrevTrackCount = -1;
  double mGain = 1.0; // last gain set from the UI; persisted per-project; used by the right-click action
};
