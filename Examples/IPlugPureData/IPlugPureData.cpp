#include "IPlugPureData.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IPlugPaths.h"
#include "wdlstring.h"
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <sys/stat.h>

// libpd callback functions
static void pd_message_callback(const char* recv, const char* msg, int argc, t_atom* argv);
static void pd_noteon_callback(int channel, int pitch, int velocity);

static int find_pd_dir(char *buf)
{
  struct stat statbuf;
  const char *homedir = (getenv("HOME") ? getenv("HOME") : ".");

  snprintf(buf, MAXPDSTRING-1,
      "%s/Applications/Pd.app/Contents/Resources", homedir);
  buf[MAXPDSTRING-1] = 0;
  if (stat(buf, &statbuf) >= 0)
      return (1);

  snprintf(buf, MAXPDSTRING-1,
      "/Applications/Pd.app/Contents/Resources");
  buf[MAXPDSTRING-1] = 0;
  if (stat(buf, &statbuf) >= 0)
      return (1);

  return (0);
}

bool IPlugPureData::LocatePdPatch(const char* patchName, WDL_String& path)
{
#if defined(OS_MAC) || defined(OS_IOS)
  // Get the bundle ID for this plugin
  const char* bundleID = GetBundleID();
  if (!bundleID) return false;
  
  // Use iPlug's resource location functionality
  EResourceLocation location = LocateResource(patchName, "pd", path, bundleID, nullptr, nullptr);
  return (location == EResourceLocation::kAbsolutePath);
#else
  // For non-Apple platforms, just use the provided path
  path.Set(patchName);
  return true;
#endif
}

IPlugPureData::IPlugPureData(const InstanceInfo& info)
: iplug::Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

  // Create and initialize the PdWrapper
  mPdWrapper = std::make_unique<PdWrapper>(this);
  mPdWrapper->Init();
  
  // Initialize parameters
  for (int i = 0; i < kMaxParams; i++) {
    mParams[i].value = 0.0f;
    mParams[i].dirty = false;
  }
  
  // Initialize patch information
  mPatchOpen = false;
  mPatchFilename[0] = '\0';
  mPatchDir[0] = '\0';
  
  // Try to locate and open the default patch
  WDL_String patchPath;
  if (LocatePdPatch("test-patch.pd", patchPath)) {
    std::filesystem::path path(patchPath.Get());
    OpenPatch(path.filename().string().c_str(), 
              path.parent_path().string().c_str());
  }
  
  // Initialize tempo tracking
  mLastTempo = -1.0;
  mTicksSinceTempo = 1000;

#if IPLUG_EDITOR // http://bit.ly/2S64BDd
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    const IRECT b = pGraphics->GetBounds();

    if (pGraphics->NControls()) {
      pGraphics->GetControl(0)->SetTargetAndDrawRECTs(b);
      return;
    }
    
    pGraphics->SetLayoutOnResize(true);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
//    pGraphics->AttachControl(new ITextControl(b.GetMidVPadded(50), "IPlug2 with libpd", IText(50)));
//    pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100).GetVShifted(-100), kGain));
    pGraphics->AttachControl(new IVToggleControl(b.GetCentredInside(100), [&](IControl* pControl){
      
      if (pControl->GetValue() > 0.5) {
        char pddir[MAXPDSTRING];
        if (find_pd_dir(pddir))
        {
          fprintf(stderr, "start gui: %s\n", pddir);
          libpd_start_gui(pddir);
        }
        else fprintf(stderr, "couldn't find Pd gui:\n");
        pd_vis = 1;
      } else {
        libpd_stop_gui();
        pd_vis = 0;
      }
    }, "Edit the patch", DEFAULT_STYLE.WithShowLabel(false)));
  };
#endif
}

IPlugPureData::~IPlugPureData()
{
  // No need to manually delete mPdWrapper, unique_ptr will handle it
}

void IPlugPureData::OnParamChange(int paramIdx, double value)
{
  // Handle parameter changes from the UI
  if (paramIdx == kGain) {
    // Convert gain to 0-1 range for pd
    float pdValue = (float)(value / 100.0);
    SetParamValue(paramIdx, pdValue);
  }
}

void IPlugPureData::OnMidiMsg(const IMidiMsg& msg)
{
  // Handle MIDI messages from the host
  if (mPdWrapper) {
    if (msg.StatusMsg() == IMidiMsg::kNoteOn) {
      mPdWrapper->ProcessMidiNote(msg.Channel(), msg.NoteNumber(), msg.Velocity());
    } else if (msg.StatusMsg() == IMidiMsg::kNoteOff) {
      mPdWrapper->ProcessMidiNote(msg.Channel(), msg.NoteNumber(), 0);
    }
  }
}

void IPlugPureData::OpenPatch(const char* filename, const char* dir)
{
  if (mPdWrapper) {
    mPdWrapper->OpenPatch(filename, dir);
  }
}

void IPlugPureData::SetParamValue(int paramIndex, float value)
{
  if (mPdWrapper) {
    mPdWrapper->SetParamValue(paramIndex, value);
  }
}

void IPlugPureData::DefineParam(int argc, t_atom* argv)
{
  // This would be implemented to handle parameter definitions from pd
  // For now, we'll leave it as a stub
}

void IPlugPureData::ReadUiDesc(const char* filename)
{
  // This would be implemented to read UI descriptions from pd
  // For now, we'll leave it as a stub
}

void IPlugPureData::Quit()
{
  // This would be implemented to handle quit messages from pd
  // For now, we'll leave it as a stub
}

#if IPLUG_DSP
void IPlugPureData::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  if (!mPdWrapper) {
    // If pd is not initialized, just pass through
    const int nChans = NOutChansConnected();
    
    for (int s = 0; s < nFrames; s++) {
      for (int c = 0; c < nChans; c++) {
        outputs[c][s] = inputs[c][s];
      }
    }
    return;
  }
  
  // Process audio directly with pd
  mPdWrapper->ProcessAudio(inputs, outputs, NInChansConnected(), nFrames);
}
#endif

// Static callback functions
static void pd_message_callback(const char* recv, const char* msg, int argc, t_atom* argv)
{
  // This is a static callback, so we need to find the instance
  t_pdinstance* pd = libpd_this_instance();
  if (!pd) return;
  
  // We would need to find the IPlugPureData instance associated with this pd instance
  // For now, we'll leave this as a stub
}

static void pd_noteon_callback(int channel, int pitch, int velocity)
{
  // This is a static callback, so we need to find the instance
  t_pdinstance* pd = libpd_this_instance();
  if (!pd) return;
  
  // We would need to find the IPlugPureData instance associated with this pd instance
  // For now, we'll leave this as a stub
}
