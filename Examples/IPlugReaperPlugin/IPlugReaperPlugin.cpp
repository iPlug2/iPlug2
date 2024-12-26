#include "IPlugReaperPlugin.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

#define REAPERAPI_IMPLEMENT
#include "reaper_plugin_functions.h"
#include "reaper_plugin_fx_embed.h"
#define LICE_PROVIDED_BY_APP
#include "lice.h"

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

int IPlugReaperPlugin::EmbeddedUIProc(int message, void* pMsg1, void* pMsg2)
{
  switch (message) {
    case 0: return 1;
    case WM_CREATE: OnCreateEmbeddedUI(); return 1;
    case WM_DESTROY: OnDestroyEmbeddedUI(); return 1;
    //case WM_SETCURSOR: return HCURSOR;
    case WM_MOUSEMOVE:
    {
      auto* pInfo = reinterpret_cast<REAPER_inline_positioninfo*>(pMsg2);
      OnEmbeddedUIMouseOver(pInfo->mouse_x, pInfo->mouse_y);
      return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    {
      auto* pInfo = reinterpret_cast<REAPER_inline_positioninfo*>(pMsg2);
      OnEmbeddedUIMouseLeft(pInfo->mouse_x, pInfo->mouse_y, message == WM_LBUTTONDOWN);
      return 0;
    }
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    {
      auto* pInfo = reinterpret_cast<REAPER_inline_positioninfo*>(pMsg2);
      OnEmbeddedUIMouseRight(pInfo->mouse_x, pInfo->mouse_y, message == WM_RBUTTONDOWN);
      return 0;
    }
    case WM_GETMINMAXINFO:
    {
      auto* pInfo = reinterpret_cast<MINMAXINFO*>(pMsg2);
      int aspectNumerator;
      int aspectDenominator;
      GetEmbeddedUIPrefferedAspectRatio(aspectNumerator, aspectDenominator);
      pInfo->ptReserved.x = (aspectNumerator<<16)/aspectDenominator;
      GetEmbeddedUIMinimumAspectRatio(aspectNumerator, aspectDenominator);
      pInfo->ptReserved.y = (aspectNumerator<<16)/aspectDenominator;
      return 1;
    }
    case WM_PAINT:
    {
      auto* pBitmap = reinterpret_cast<REAPER_FXEMBED_IBitmap*>(pMsg1);
      auto* pInfo = reinterpret_cast<REAPER_inline_positioninfo*>(pMsg2);

      static int oldWidth = 0;
      static int oldHeight = 0;

      if (pInfo->width != oldWidth || pInfo->height != oldHeight)
      {
        OnEmbeddedUIResize(pInfo->width, pInfo->height);
        oldWidth = pInfo->width;
        oldHeight = pInfo->height;
      }

      int extraFlag = 0;
      if (pInfo->extraParms[0])
        extraFlag = (int)(INT_PTR) pInfo->extraParms[0];

      DrawEmbeddedUI(pBitmap, pInfo->mouse_x, pInfo->mouse_y, extraFlag & 0x10000, extraFlag & 0x20000);

      return 1;
    }
    default:
      return 0;
  }

}

#if defined VST2_API
VstIntPtr IPlugReaperPlugin::VSTVendorSpecific(VstInt32 idx, VstIntPtr value, void* ptr, float opt)
{
  if (idx == AEffectOpcodes::__effEditDrawDeprecated)
  {
    return EmbeddedUIProc((int) opt, FromVstPtr<void*>(value), ptr);
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
  return EmbeddedUIProc(msg, FromVstPtr<void*>(parm2), FromVstPtr<void*>(parm3));
}
#endif

void IPlugReaperPlugin::DrawEmbeddedUI(REAPER_FXEMBED_IBitmap* pBitmap, int mouseX, int mouseY, bool leftMouseDown, bool rightMouseDown)
{
  LICE_FillRect((LICE_IBitmap*) pBitmap, 0, 0, pBitmap->getWidth(), pBitmap->getHeight(), LICE_RGBA(255, 255, 255, 255), 1.f, 0);
  
  if (leftMouseDown || rightMouseDown)
  {
    LICE_FillCircle((LICE_IBitmap*) pBitmap, mouseX, mouseY, 20.f, leftMouseDown ? LICE_RGBA(255, 0, 0, 255) : LICE_RGBA(0, 255, 0, 255), 1.f, 0, true);
  }
}
