#include "IPlugReaperPlugin.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

#define REAPERAPI_IMPLEMENT
#include "reaper_plugin_functions.h"
#include "reaper_plugin_fx_embed.h"

#ifdef FillRect
#undef FillRect
#endif
#ifdef DrawText
#undef DrawText
#endif
#ifdef Polygon
#undef Polygon
#endif

#ifdef CLAP_API
#define IMPAPI(x) if (!((*((void **)&(x)) = (void*) pRec->GetFunc(#x)))) errorCount++;
#endif

extern int (*GetPlayState)();

void GetReaperTrackName(MediaTrack* pTrack, WDL_String& str)
{
  char buf[2048];
  if (GetSetMediaTrackInfo_String(pTrack, "P_NAME", buf, false) == true)
  {
    str.Set(buf);
  }
}

void SetReaperTrackName(MediaTrack* pTrack, const char* name)
{
  GetSetMediaTrackInfo_String(pTrack, "P_NAME", const_cast<char*>(name), true);
  UpdateArrange();
}

void SetTrackVolume(MediaTrack* pTrack, double gain)
{
  SetMediaTrackInfo_Value(pTrack, "D_VOL", gain);
}

IPlugReaperPlugin::IPlugReaperPlugin(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
#if defined CLAP_API
, mpClapHost(info.mHost)
#endif
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
      
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  auto embeddedLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachPanelBackground(COLOR_ORANGE);
    pGraphics->AttachControl(new IVKnobControl(IRECT(10, 10, 120, 120), kNoParameter));
  };
  
  mEmbeddedGraphicsDelegate.SetLayoutFunc(embeddedLayoutFunc);
  
  mEmbeddedGraphicsDelegate.SetPreferredAspect(1, 1);
  mEmbeddedGraphicsDelegate.SetMinimumAspect(1, 4);
  mEmbeddedGraphicsDelegate.SetMinSize(40, 40);
  mEmbeddedGraphicsDelegate.SetMaxSize(180, 300);
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();

    pGraphics->AttachControl(new ILambdaControl(b,
      [&](ILambdaControl* pCaller, IGraphics& g, IRECT& rect) {
      
      if (GetHost() == kHostReaper)
      {
        if (GetPlayState && GetPlayState()) {
          g.FillRect(COLOR_GREEN, rect);
          g.DrawText({30}, "PLAYING", rect);
        }
        else {
          g.FillRect(COLOR_RED, rect);
          g.DrawText({30}, "STOPPED", rect);
        }
        
        if (auto pTrack = GetReaperTrack())
        {
          WDL_String trackName;
          GetReaperTrackName(pTrack, trackName);
          g.DrawText({30}, trackName.Get(), rect.GetFromTop(100));
          WDL_String channelCount;
          channelCount.SetFormatted(32, "Channel count %i", GetReaperTrackChannelCount());
          g.DrawText({30}, channelCount.Get(), rect.GetFromTop(100).GetVShifted(20));
        }
      }
      else
      {
        g.DrawText({30}, "This example is designed for REAPER", rect);
      }
      
    }, DEFAULT_ANIMATION_DURATION, true /*loop*/, true /*start immediately*/));

  };
  
#if defined CLAP_API
  OnHostIdentified();
#endif
}

void IPlugReaperPlugin::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}

void IPlugReaperPlugin::OnHostIdentified()
{
  if (GetHost() == kHostReaper)
  {
    int errorCount = REAPERAPI_LoadAPI([this](const char* str) {
#if defined VST2_API
      return (void*) mHostCallback(NULL, 0xdeadbeef, 0xdeadf00d, 0, (void*) str, 0.0);
#elif defined VST3_API
      return (void*) mpReaperHostApplication->getReaperApi(str);
#elif defined CLAP_API
      return (void*) reinterpret_cast<const reaper_plugin_info_t*>(mpClapHost->get_extension(mpClapHost, "cockos.reaper_extension"))->GetFunc(str);
#else
      return nullptr;
#endif
    });
    
#if defined CLAP_API
    auto pRec = reinterpret_cast<const reaper_plugin_info_t*>(mpClapHost->get_extension(mpClapHost, "cockos.reaper_extension"));
    IMPAPI(clap_get_reaper_context);
#endif
    
    if (errorCount > 0)
    {
      LogToReaperConsole("some errors when loading reaper api functions\n");
    }
  }
}

#if defined VST3_API
DEF_CLASS_IID (IReaperHostApplication);
DEF_CLASS_IID (IReaperUIEmbedInterface);

tresult PLUGIN_API IPlugReaperPlugin::initialize(FUnknown* pContext)
{
  void* pIHostApplication = nullptr;

  if (pContext->queryInterface(IReaperHostApplication::iid, &pIHostApplication) == Steinberg::kResultOk)
  {
    mpReaperHostApplication = static_cast<IReaperHostApplication*>(pIHostApplication);
  }
  
  return Plugin::initialize(pContext);
}
#endif

MediaTrack* IPlugReaperPlugin::GetReaperTrack()
{
  return GetReaperThing<MediaTrack*>(1);
}

MediaItem_Take* IPlugReaperPlugin::GetReaperTake()
{
  return GetReaperThing<MediaItem_Take*>(2);
}

ReaProject* IPlugReaperPlugin::GetReaperProject()
{
  return GetReaperThing<ReaProject*>(3);
}

FxDsp* IPlugReaperPlugin::GetFxDsp()
{
  return GetReaperThing<FxDsp*>(4);
}

int IPlugReaperPlugin::GetReaperTrackChannelCount()
{
  return (int) GetReaperThing<INT_PTR>(5);
}

int IPlugReaperPlugin::GetReaperIndexInChain()
{
  return (int) GetReaperThing<INT_PTR>(6);
}

void IPlugReaperPlugin::LogToReaperConsole(const char* str)
{
  if (ShowConsoleMsg)
  {
    ShowConsoleMsg(str);
  }
}

#if defined VST2_API
VstIntPtr IPlugReaperPlugin::VSTVendorSpecific(VstInt32 idx, VstIntPtr value, void* ptr, float opt)
{
  if (idx == AEffectOpcodes::__effEditDrawDeprecated)
  {
    return mEmbeddedGraphicsDelegate.EmbeddedUIProc((int) opt, FromVstPtr<void*>(value), ptr);
  }
  return 0;
}

VstIntPtr IPlugReaperPlugin::VSTCanDo(const char* hostString)
{
  if (!strcmp(hostString, "hasCockosEmbeddedUI"))
  {
    return 0xbeef0000;
  }

  return 0;
}
#endif

#if defined VST3_API
template <class T> inline T* FromVstPtr (Steinberg::TPtrInt& arg)
{
  T** address = (T**)&arg;
  return *address;
}

Steinberg::TPtrInt IPlugReaperPlugin::embed_message(int msg, Steinberg::TPtrInt parm2, Steinberg::TPtrInt parm3)
{
  return mEmbeddedGraphicsDelegate.EmbeddedUIProc(msg, FromVstPtr<void*>(parm2), FromVstPtr<void*>(parm3));
}
#endif
