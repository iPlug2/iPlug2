#include "IPlugFaustDSP.h"
#include "IPlug_include_in_plug_src.h"

IPlugFaustDSP::IPlugFaustDSP(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, 1))
{
  InitParamRange(0, kNumParams-1, 1, "Param %i", 0., 0., 1., 0.01, "", IParam::kFlagsNone); // initialize kNumParams generic iplug params
  
#if IPLUG_DSP
  // Get the path to the libraries folder inside the bundle's resources
  WDL_String bundlePath;
  #if defined(__APPLE__)
    CFBundleRef bundle = CFBundleGetMainBundle();
    if (bundle) {
      CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(bundle);
      if (resourcesURL) {
        char resourcesPath[PATH_MAX];
        if (CFURLGetFileSystemRepresentation(resourcesURL, true, (UInt8*)resourcesPath, PATH_MAX)) {
          bundlePath.Set(resourcesPath);
        }
        CFRelease(resourcesURL);
      }
    }
  #endif
  
  WDL_String filePath, libraryPath;
  libraryPath = bundlePath;
  filePath = bundlePath;
  libraryPath.Append("/libraries");
  filePath.Append("/IPlugFaustDSP.dsp");

  mFaustProcessor = std::make_unique<FaustGen>("Hello", filePath.Get(), 1, 1, nullptr, nullptr, libraryPath.Get());
  mFaustProcessor->SetMaxChannelCount(MaxNChannels(ERoute::kInput), MaxNChannels(ERoute::kOutput));
  mFaustProcessor->Init();
//  mFaustProcessor->CompileCPP();
  mFaustProcessor->SetAutoRecompile(true);
  mFaustProcessor->CreateIPlugParameters(this);
  mFaustProcessor->SetCompileFunc([&](){
    GetUI()->RemoveAllControls();
    LayoutUI(GetUI());
    OnParamReset(EParamSource::kRecompile);
    mFaustProcessor->SyncFaustParams();  });
#endif
  
#if IPLUG_EDITOR
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    IRECT b = pGraphics->GetBounds();
    IRECT faustUIRect = b.GetPadded(-10);

    if (pGraphics->NControls()) {
      pGraphics->GetBackgroundControl()->SetTargetAndDrawRECTs(b);
      pGraphics->GetControl(1)->SetTargetAndDrawRECTs(faustUIRect);
      return;
    }
    
    pGraphics->SetLayoutOnResize(true);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->EnableMultiTouch(true);
    
    mUIBuilder = std::make_unique<IGraphicsFaustUI>(*mFaustProcessor, DEFAULT_STYLE
                                                                        .WithDrawShadows(false)
                                                                        .WithLabelText({15})
                                                    .WithShowValue(false));

    pGraphics->AttachControl(mUIBuilder->CreateFaustUIContainer(faustUIRect));

//    for (int i = 0; i < kNumParams; i++) {
//      pGraphics->AttachControl(new IVKnobControl(knobs.GetGridCell(i, 1, kNumParams).GetPadded(-5.f), i));
//    }
//    
//    pGraphics->AttachControl(new IVScopeControl<2>(scope, "", DEFAULT_STYLE.WithColor(kBG, COLOR_BLACK).WithColor(kFG, COLOR_GREEN)), kCtrlTagScope);
//    pGraphics->AttachControl(new IVKeyboardControl(keyb));
  };
#endif
}

#if IPLUG_DSP

void IPlugFaustDSP::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  mFaustProcessor->ProcessBlock(inputs, outputs, nFrames);
  mScopeSender.ProcessBlock(outputs, nFrames, kCtrlTagScope);
}

void IPlugFaustDSP::OnReset()
{
  mFaustProcessor->SetSampleRate(GetSampleRate());
}

void IPlugFaustDSP::ProcessMidiMsg(const IMidiMsg& msg)
{
  mFaustProcessor->ProcessMidiMsg(msg);
}

void IPlugFaustDSP::OnParamChange(int paramIdx)
{
  mFaustProcessor->SetParameterValueNormalised(paramIdx, GetParam(paramIdx)->GetNormalized());
}

void IPlugFaustDSP::OnIdle()
{
//  mScopeSender.TransmitData(*this);
}
#endif
