#pragma once

#include "IPlug_include_in_plug_hdr.h"

#if defined VST3_API
using namespace Steinberg;
#include "reaper_vst3_interfaces.h"
#endif

#if defined CLAP_API
void *(*clap_get_reaper_context)(const clap_host *host, int sel);
#endif

const int kNumPresets = 1;

enum EParams
{
  kGain = 0,
  kNumParams
};

using namespace iplug;
using namespace igraphics;

class ReaProject;
class MediaTrack;
class MediaItem;
class MediaItem_Take;
class FxDsp;
class REAPER_FXEMBED_IBitmap;

class IReaperHostApplication;

class IPlugReaperPlugin final : public Plugin
#if defined VST3_API
, public IReaperUIEmbedInterface
#endif
{
public:
  IPlugReaperPlugin(const InstanceInfo& info);

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  
  void OnHostIdentified() override;
  
  void GetEmbeddedUIPrefferedAspectRatio(int& numerator, int& denominator) { numerator = 1; denominator = 1; };
  void GetEmbeddedUIMinimumAspectRatio(int& numerator, int& denominator) { numerator = 1; denominator = 1; };
  
  void OnCreateEmbeddedUI() { /* No Op */ };
  void OnDestroyEmbeddedUI() { /* No Op */ };
  void OnEmbeddedUIMouseOver(int mouseX, int mouseY) { /* No Op */ }
  void OnEmbeddedUIMouseLeft(int mouseX, int mouseY, bool down) { /* No Op */ }
  void OnEmbeddedUIMouseRight(int mouseX, int mouseY, bool down) { /* No Op */ }
  void OnEmbeddedUIResize(int w, int h) { /* No Op */ };
  void DrawEmbeddedUI(REAPER_FXEMBED_IBitmap* pBitmap, int mouseX, int mouseY, bool leftMouseDown, bool rightMouseDown);
  
#if defined VST3_API
  tresult PLUGIN_API initialize(FUnknown* context) override;
  
  OBJ_METHODS(IPlugReaperPlugin, Plugin)
  DEFINE_INTERFACES
      DEF_INTERFACE(IReaperUIEmbedInterface)
  END_DEFINE_INTERFACES(Plugin)
  REFCOUNT_METHODS(Plugin)
#endif
  
private:
#if defined CLAP_API
  const clap_host* mpClapHost = nullptr;
#elif defined VST3_API
  IReaperHostApplication* mpReaperHostApplication = nullptr;
  Steinberg::TPtrInt embed_message(int msg, Steinberg::TPtrInt parm2, Steinberg::TPtrInt parm3) override;
#endif

#if defined VST2_API
  VstIntPtr VSTCanDo(const char* hostString) override;
  VstIntPtr VSTVendorSpecific(VstInt32 idx, VstIntPtr value, void* ptr, float opt) override;
#endif

  template<typename T>
  T GetReaperThing(int type);
  MediaTrack* GetReaperTrack();
  MediaItem_Take* GetReaperTake();
  ReaProject* GetReaperProject();
  FxDsp* GetFxDsp();
  int GetReaperTrackChannelCount();
  int GetReaperIndexInChain();
  void LogToReaperConsole(const char* str);
  int EmbeddedUIProc(int message, void* pMsg1, void* pMsg2);
};

template<typename T>
T IPlugReaperPlugin::GetReaperThing(int sel)
{
  void* pResult = nullptr;
#if defined VST2_API
  pResult = (void*) mHostCallback(&mAEffect, 0xdeadbeef, 0xdeadf00e, sel, NULL, 0.0);
#elif defined VST3_API
  if (mpReaperHostApplication)
  {
    pResult = mpReaperHostApplication->getReaperParent(sel);
  }
#elif defined CLAP_API
  if (clap_get_reaper_context)
  {
    pResult = clap_get_reaper_context(mpClapHost, sel);
  }
#endif
  
  return reinterpret_cast<T>(pResult);
}
