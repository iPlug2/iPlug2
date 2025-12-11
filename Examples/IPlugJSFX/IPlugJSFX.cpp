#include "IPlugJSFX.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

#include "sfxui.h"

HINSTANCE g_hInst = nullptr;
const char* g_config_slider_classname = "IPlugJSFXSlider";
const char* default_get_eel_funcdesc(const char* name) { return nullptr; }
const char* (*get_eel_funcdesc)(const char* name) = default_get_eel_funcdesc;

IPlugJSFX::IPlugJSFX(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // Initialize parameters - these will map to JSFX sliders
  for (int i = 0; i < kNumParams; i++)
  {
    char name[32];
    snprintf(name, sizeof(name), "Slider %d", i + 1);
    GetParam(i)->InitDouble(name, 0.0, 0.0, 1.0, 0.001);
  }

  // Set default JSFX root path relative to plugin location
  // Users should override this with SetJSFXRootPath()
  mJSFXRootPath.Set("JS");

#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);

    const IRECT b = pGraphics->GetBounds();

    // Display effect name
    WDL_String title;
    if (mJSFXInstance)
      title.SetFormatted(256, "JSFX: %s", GetEffectName());
    else
      title.Set("JSFX: No effect loaded");

    pGraphics->AttachControl(new ITextControl(b.GetFromTop(50), title.Get(), IText(24)));

    // Create sliders for the first 8 JSFX parameters
    const int numVisibleParams = 8;
    const float sliderHeight = 40.f;
    const float startY = 80.f;

    for (int i = 0; i < numVisibleParams && i < kNumParams; i++)
    {
      IRECT sliderBounds = b.GetFromTop(startY + sliderHeight * (i + 1))
                            .GetFromBottom(sliderHeight)
                            .GetPadded(-20.f);
      pGraphics->AttachControl(new IVSliderControl(sliderBounds, i));
    }

    pGraphics->AttachControl(new ITextControl(b.GetFromBottom(30),
      "Set JSFX root path and load an effect", IText(14)));
  };
#endif
}

IPlugJSFX::~IPlugJSFX()
{
  CloseEffect();
}

void IPlugJSFX::SetJSFXRootPath(const char* path)
{
  mJSFXRootPath.Set(path);
}

bool IPlugJSFX::LoadEffect(const char* effectPath)
{
  // Close any existing effect
  CloseEffect();

  if (!effectPath || !*effectPath)
    return false;

  mEffectName.Set(effectPath);

  // Create the JSFX instance
  mJSFXInstance = sx_createInstance(mJSFXRootPath.Get(), effectPath, nullptr);

  if (mJSFXInstance)
  {
    // Set up the instance
    sx_updateHostNch(mJSFXInstance, -1);

    // Set sample rate
    sx_extended(mJSFXInstance, JSFX_EXT_SET_SRATE, (void*)(intptr_t)GetSampleRate(), nullptr);

    // Sync parameters from JSFX to our parameters
    SyncParametersFromJSFX();

    mNeedsReset = true;
    return true;
  }

  return false;
}

void IPlugJSFX::CloseEffect()
{
  if (mJSFXInstance)
  {
    sx_destroyInstance(mJSFXInstance);
    mJSFXInstance = nullptr;
  }
  mEffectName.Set("");
}

const char* IPlugJSFX::GetEffectName() const
{
  if (mJSFXInstance)
    return sx_getEffectName(mJSFXInstance);
  return mEffectName.Get();
}

void IPlugJSFX::SyncParametersToJSFX()
{
  if (!mJSFXInstance)
    return;

  int numParms = sx_getNumParms(mJSFXInstance);
  for (int i = 0; i < numParms && i < kNumParams; i++)
  {
    double minVal = 0.0, maxVal = 1.0, step = 0.0;
    sx_getParmVal(mJSFXInstance, i, &minVal, &maxVal, &step);

    // Convert from normalized [0,1] to JSFX range
    double normVal = GetParam(i)->Value();
    double jsfxVal = minVal + normVal * (maxVal - minVal);

    sx_setParmVal(mJSFXInstance, i, jsfxVal, 0);
  }
}

void IPlugJSFX::SyncParametersFromJSFX()
{
  if (!mJSFXInstance)
    return;

  int numParms = sx_getNumParms(mJSFXInstance);
  for (int i = 0; i < numParms && i < kNumParams; i++)
  {
    double minVal = 0.0, maxVal = 1.0, step = 0.0;
    double val = sx_getParmVal(mJSFXInstance, i, &minVal, &maxVal, &step);

    // Get parameter name from JSFX
    char name[256] = "";
    sx_getParmName(mJSFXInstance, i, name, sizeof(name));
    if (name[0])
      GetParam(i)->SetDisplayText(0, name);

    // Normalize to [0,1]
    double normVal = 0.0;
    if (maxVal > minVal)
      normVal = (val - minVal) / (maxVal - minVal);

    GetParam(i)->Set(normVal);
  }
}

#if IPLUG_DSP

void IPlugJSFX::OnReset()
{
  mSampleRate = GetSampleRate();

  if (mJSFXInstance)
  {
    // Set sample rate for JSFX
    sx_extended(mJSFXInstance, JSFX_EXT_SET_SRATE, (void*)(intptr_t)(int)mSampleRate, nullptr);

    // Reset JSFX state - passing -1 for playstate triggers reset
    sx_processSamples(mJSFXInstance, nullptr, 0, 0, 0, 0.0, 0, 0, -1, 0, 0, 1.0, 1.0, 0);
  }

  mNeedsReset = false;
}

void IPlugJSFX::OnActivate(bool active)
{
  if (active && mNeedsReset)
  {
    OnReset();
  }
}

void IPlugJSFX::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const int nInCh = NInChansConnected();
  const int nOutCh = NOutChansConnected();
  const int nCh = std::max(nInCh, nOutCh);

  if (!mJSFXInstance || nCh == 0)
  {
    // Pass-through or silence if no JSFX loaded
    for (int c = 0; c < nOutCh; c++)
    {
      if (c < nInCh)
        memcpy(outputs[c], inputs[c], nFrames * sizeof(sample));
      else
        memset(outputs[c], 0, nFrames * sizeof(sample));
    }
    return;
  }

  // Sync any parameter changes to JSFX
  SyncParametersToJSFX();

  // Resize interleave buffer if needed
  double* buf = mInterleaveBuf.Resize(nCh * nFrames, false);

  // Interleave input audio into JSFX buffer
  for (int s = 0; s < nFrames; s++)
  {
    for (int c = 0; c < nCh; c++)
    {
      if (c < nInCh)
        buf[s * nCh + c] = static_cast<double>(inputs[c][s]);
      else
        buf[s * nCh + c] = 0.0;
    }
  }

  // Get transport info
  double tempo = 120.0;
  int transportState = 1; // playing
  double posSec = 0.0;
  double posBeats = 0.0;
  int tsNum = 4, tsDenom = 4;

  ITimeInfo timeInfo;
  if (GetHost() != kHostUninit)
  {
    GetTimeInfo(timeInfo);
    tempo = timeInfo.mTempo;
    transportState = timeInfo.mTransportIsRunning ? 1 : 0;
    posSec = timeInfo.mSamplePos / mSampleRate;
    posBeats = timeInfo.mPPQPos;
    // Note: time signature would come from timeInfo if available
  }

  // Process through JSFX
  // flags: 0=normal, 1=ignore pdc, 2=delta solo, 4=zero output before wet/dry
  sx_processSamples(mJSFXInstance, buf, nFrames, nCh,
                    static_cast<int>(mSampleRate + 0.5),
                    tempo, tsNum, tsDenom,
                    transportState, posSec, posBeats,
                    1.0, 1.0, 0);

  // De-interleave output audio from JSFX buffer
  for (int s = 0; s < nFrames; s++)
  {
    for (int c = 0; c < nOutCh; c++)
    {
      if (c < nCh)
        outputs[c][s] = static_cast<sample>(buf[s * nCh + c]);
      else
        outputs[c][s] = 0.0;
    }
  }
}

bool IPlugJSFX::SerializeState(IByteChunk& chunk) const
{
  // Save effect name
  chunk.PutStr(mEffectName.Get());

  // Save JSFX state if instance exists
  if (mJSFXInstance)
  {
    int len = 0;
    const char* state = sx_saveState(const_cast<SX_Instance*>(mJSFXInstance), &len);
    if (state && len > 0)
    {
      chunk.Put(&len);
      chunk.PutBytes(state, len);
    }
    else
    {
      int zero = 0;
      chunk.Put(&zero);
    }

    // Save serialized binary state
    const char* serState = sx_saveSerState(const_cast<SX_Instance*>(mJSFXInstance), &len);
    if (serState && len > 0)
    {
      chunk.Put(&len);
      chunk.PutBytes(serState, len);
    }
    else
    {
      int zero = 0;
      chunk.Put(&zero);
    }
  }
  else
  {
    int zero = 0;
    chunk.Put(&zero);
    chunk.Put(&zero);
  }

  return true;
}

int IPlugJSFX::UnserializeState(const IByteChunk& chunk, int startPos)
{
  int pos = startPos;

  // Load effect name
  WDL_String effectName;
  pos = chunk.GetStr(effectName, pos);

  // Load the effect if we have a name
  if (effectName.GetLength() > 0)
  {
    LoadEffect(effectName.Get());
  }

  if (mJSFXInstance)
  {
    // Load text state
    int stateLen = 0;
    pos = chunk.Get(&stateLen, pos);
    if (stateLen > 0)
    {
      WDL_HeapBuf stateBuf;
      stateBuf.Resize(stateLen + 1);
      pos = chunk.GetBytes(stateBuf.Get(), stateLen, pos);
      ((char*)stateBuf.Get())[stateLen] = 0;
      sx_loadState(mJSFXInstance, (const char*)stateBuf.Get());
    }

    // Load serialized binary state
    int serLen = 0;
    pos = chunk.Get(&serLen, pos);
    if (serLen > 0)
    {
      WDL_HeapBuf serBuf;
      serBuf.Resize(serLen);
      pos = chunk.GetBytes(serBuf.Get(), serLen, pos);
      sx_loadSerState(mJSFXInstance, (const char*)serBuf.Get(), serLen);
    }

    // Sync parameters after loading state
    SyncParametersFromJSFX();
  }
  else
  {
    // Skip state data if no instance
    int stateLen = 0;
    pos = chunk.Get(&stateLen, pos);
    pos += stateLen;

    int serLen = 0;
    pos = chunk.Get(&serLen, pos);
    pos += serLen;
  }

  return pos;
}

#endif // IPLUG_DSP
