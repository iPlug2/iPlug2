/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests a complete plug-in with an IGraphics UI: the test is both the host and the user

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "IPlugTestAPI.h"
#include "AudioBuffers.h"
#include "IGraphicsHeadless.h"
#include "TestControls.h"
#include "IControls.h"
#include "IVKeyboardControl.h"
#include "IVMeterControl.h"
#include "ISender.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;

namespace
{
enum EParams { kGain = 0, kMode, kNumParams };
enum ECtrlTags { kCtrlTagKnob = 0, kCtrlTagSlider, kCtrlTagMode, kCtrlTagMeter, kCtrlTagKeyboard };

const int kBlockSize = 512;
const IRECT kKnobBounds = IRECT(20, 20, 120, 120);
const IRECT kSliderBounds = IRECT(140, 20, 180, 220);
const IRECT kModeBounds = IRECT(200, 20, 380, 50);
const IRECT kMeterBounds = IRECT(200, 60, 260, 220);
const IRECT kKeyboardBounds = IRECT(20, 230, 380, 290);
const IVStyle kPlainStyle = DEFAULT_STYLE.WithShowLabel(false).WithShowValue(false);

Config MakeConfig()
{
  TestConfig config;
  config.nParams = kNumParams;
  config.midiIn = true;
  config.hasUI = true;
  config.width = 400;
  config.height = 300;
  return config.Make();
}

/** A gain plug-in with a knob and a slider for the gain, a mode switch, an output meter and an on-screen keyboard */
class UIPlug : public Plugin
{
public:
  UIPlug(const InstanceInfo& info = InstanceInfo())
  : Plugin(info, MakeConfig())
  {
    GetParam(kGain)->InitDouble("Gain", 100., 0., 100., 0.01, "%");
    GetParam(kMode)->InitEnum("Mode", 0, {"Normal", "Invert", "Mute"});

    mMakeGraphicsFunc = [this]() { return new IGraphicsHeadless(*this, 400, 300); };

    mLayoutFunc = [this](IGraphics* pGraphics) {
      mNumLayouts++;
      pGraphics->AttachPanelBackground(COLOR_GRAY);
      pGraphics->AttachControl(new IVKnobControl(kKnobBounds, kGain, "", kPlainStyle), kCtrlTagKnob);
      pGraphics->AttachControl(new IVSliderControl(kSliderBounds, kGain, "", kPlainStyle), kCtrlTagSlider);
      pGraphics->AttachControl(new IVTabSwitchControl(kModeBounds, kMode, {}, "", kPlainStyle), kCtrlTagMode);
      pGraphics->AttachControl(new IVMeterControl<2>(kMeterBounds, "", kPlainStyle), kCtrlTagMeter);
      pGraphics->AttachControl(new IVKeyboardControl(kKeyboardBounds, 48, 72), kCtrlTagKeyboard);
    };
  }

  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override
  {
    double gain = GetParam(kGain)->Value() / 100.;

    switch (GetParam(kMode)->Int())
    {
      case 1: gain = -gain; break;
      case 2: gain = 0.; break;
      default: break;
    }

    for (int c = 0; c < NOutChansConnected(); c++)
    {
      for (int s = 0; s < nFrames; s++)
        outputs[c][s] = inputs[c][s] * gain;
    }

    mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);
  }

  void ProcessMidiMsg(const IMidiMsg& msg) override { mMidiReceived.push_back(msg); }
  void OnIdle() override { mMeterSender.TransmitData(*this); }
  void OnReset() override { mMeterSender.Reset(GetSampleRate()); }
  void OnUIOpen() override { mNumUIOpens++; Plugin::OnUIOpen(); }
  void OnUIClose() override { mNumUICloses++; Plugin::OnUIClose(); }

  IGraphicsHeadless* UI() { return static_cast<IGraphicsHeadless*>(GetUI()); }

  IPeakSender<2> mMeterSender;
  std::vector<IMidiMsg> mMidiReceived;
  int mNumLayouts = 0;
  int mNumUIOpens = 0;
  int mNumUICloses = 0;
};

/** A plug-in with its UI open, and buffers with a constant input signal of 0.5 */
struct Fixture
{
  Fixture()
  : mIn(2, kBlockSize, 0.5f)
  , mOut(2, kBlockSize)
  {
    mPlug.HostActivate(48000., kBlockSize);
    mPlug.OpenWindow(nullptr);
    mUI = mPlug.UI();
    REQUIRE(mUI != nullptr);
    mUI->RenderUntilClean();
    mUI->ClearRecorded();
    mPlug.ClearRecorded();
  }

  ~Fixture()
  {
    mPlug.CloseWindow();
  }

  void Process() { mPlug.HostProcess(mIn.Get(), mOut.Get(), kBlockSize); }

  UIPlug mPlug;
  AudioBuffers<float> mIn;
  AudioBuffers<float> mOut;
  IGraphicsHeadless* mUI = nullptr;
};

using ParamEvent = UIPlug::ParamEvent;

int CountEvents(const UIPlug& plug, ParamEvent::EType type, int paramIdx)
{
  int n = 0;
  for (auto& event : plug.mParamEvents)
    n += (event.mType == type && event.mIdx == paramIdx);
  return n;
}
} // namespace

TEST_CASE("Plug-in UI: opening the editor builds the UI and shows the parameter values", "[integration][plugin]")
{
  UIPlug plug;
  plug.GetParam(kGain)->Set(30.);

  REQUIRE(plug.GetUI() == nullptr);
  REQUIRE(plug.OpenWindow(nullptr) != nullptr);

  auto* pUI = plug.UI();
  REQUIRE(pUI != nullptr);
  REQUIRE(plug.mNumLayouts == 1);
  REQUIRE(plug.mNumUIOpens == 1);
  REQUIRE(pUI->NControls() == 6);
  REQUIRE(pUI->Width() == 400);
  REQUIRE(pUI->Height() == 300);
  REQUIRE(pUI->GetControlWithTag(kCtrlTagKnob)->GetValue() == Approx(0.3));
  REQUIRE(pUI->GetControlWithTag(kCtrlTagSlider)->GetValue() == Approx(0.3));
  REQUIRE(pUI->RenderFrame());
  REQUIRE(plug.mParamEvents.empty());

  plug.CloseWindow();
  REQUIRE(plug.GetUI() == nullptr);
  REQUIRE(plug.mNumUICloses == 1);
}

TEST_CASE("Plug-in UI: host automation moves the controls on the next idle", "[integration][plugin][params]")
{
  Fixture f;
  IControl* pKnob = f.mUI->GetControlWithTag(kCtrlTagKnob);
  IControl* pSlider = f.mUI->GetControlWithTag(kCtrlTagSlider);

  f.mPlug.HostSetParameterNormalized(kGain, 0.25);
  REQUIRE(f.mPlug.GetParam(kGain)->Value() == Approx(25.));

  // The UI is updated from the main thread, not from wherever the host set the parameter
  f.mPlug.HostIdle();

  REQUIRE(pKnob->GetValue() == Approx(0.25));
  REQUIRE(pSlider->GetValue() == Approx(0.25));
  REQUIRE(pKnob->IsDirty());
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->mLastDirtyBounds.Contains(kKnobBounds));

  // The host is not told about its own change
  REQUIRE(f.mPlug.mParamEvents.empty());

  f.mPlug.HostSetParameterNormalized(kMode, 1.);
  f.mPlug.HostIdle();
  REQUIRE(f.mUI->GetControlWithTag(kCtrlTagMode)->GetValue() == Approx(1.));
  REQUIRE(f.mUI->GetControlWithTag(kCtrlTagMode)->As<IVTabSwitchControl>()->GetSelectedIdx() == 2);
}

TEST_CASE("Plug-in UI: dragging a knob informs the host and changes the audio", "[integration][plugin][params][mouse]")
{
  Fixture f;
  IControl* pKnob = f.mUI->GetControlWithTag(kCtrlTagKnob);

  f.Process();
  REQUIRE(f.mOut[0][0] == Approx(0.5f));
  REQUIRE(f.mOut[1][kBlockSize - 1] == Approx(0.5f));

  const float x = kKnobBounds.MW(), y = kKnobBounds.MH();
  f.mUI->MouseDown(x, y);
  f.mUI->MouseDragTo(x, y + 10.f);
  f.mUI->MouseDragTo(x, y + 20.f);

  // Mid-gesture: the host has a begin and values, but no end yet
  REQUIRE(CountEvents(f.mPlug, ParamEvent::EType::Begin, kGain) == 1);
  REQUIRE(CountEvents(f.mPlug, ParamEvent::EType::Value, kGain) == 2);
  REQUIRE(CountEvents(f.mPlug, ParamEvent::EType::End, kGain) == 0);

  f.mUI->MouseUp(x, y + 20.f);

  REQUIRE(f.mPlug.mParamEvents.front().mType == ParamEvent::EType::Begin);
  REQUIRE(f.mPlug.mParamEvents.back().mType == ParamEvent::EType::End);
  REQUIRE(CountEvents(f.mPlug, ParamEvent::EType::End, kGain) == 1);

  const double gain = pKnob->GetValue();
  REQUIRE(gain < 1.);
  REQUIRE(gain > 0.);
  REQUIRE(f.mPlug.GetParam(kGain)->GetNormalized() == Approx(gain));

  double lastValueSent = -1.;
  for (auto& event : f.mPlug.mParamEvents)
  {
    if (event.mType == ParamEvent::EType::Value)
      lastValueSent = event.mNormalizedValue;
  }
  REQUIRE(lastValueSent == Approx(gain));

  f.Process();
  REQUIRE(f.mOut[0][0] == Approx(0.5 * gain));
  REQUIRE(f.mOut[1][kBlockSize - 1] == Approx(0.5 * gain));

  // The slider that shows the same parameter followed the knob
  REQUIRE(f.mUI->GetControlWithTag(kCtrlTagSlider)->GetValue() == Approx(gain));
}

TEST_CASE("Plug-in UI: a switch on the UI changes what the processor does", "[integration][plugin][params][mouse]")
{
  Fixture f;
  auto* pTabs = f.mUI->GetControlWithTag(kCtrlTagMode)->As<IVTabSwitchControl>();
  const IRECT widget = pTabs->GetWidgetBounds();

  const IRECT invertTab = widget.SubRect(EDirection::Horizontal, 3, 1);
  f.mUI->Click(invertTab.MW(), invertTab.MH());
  REQUIRE(f.mPlug.GetParam(kMode)->Int() == 1);
  f.Process();
  REQUIRE(f.mOut[0][10] == Approx(-0.5f));

  const IRECT muteTab = widget.SubRect(EDirection::Horizontal, 3, 2);
  f.mUI->Click(muteTab.MW(), muteTab.MH());
  f.Process();
  REQUIRE(f.mOut[0][10] == Approx(0.f));

  REQUIRE(CountEvents(f.mPlug, ParamEvent::EType::Begin, kMode) == 2);
  REQUIRE(CountEvents(f.mPlug, ParamEvent::EType::Value, kMode) == 2);
  REQUIRE(CountEvents(f.mPlug, ParamEvent::EType::End, kMode) == 2);
}

TEST_CASE("Plug-in UI: a peak sender in the processor drives a meter on the UI", "[integration][plugin][sender][meter]")
{
  Fixture f;
  IControl* pMeter = f.mUI->GetControlWithTag(kCtrlTagMeter);

  REQUIRE(pMeter->GetValue(0) == Approx(0.));
  f.Process();

  // Nothing reaches the UI until the main thread idles
  REQUIRE(pMeter->GetValue(0) == Approx(0.));
  REQUIRE_FALSE(pMeter->IsDirty());

  f.mPlug.HostIdle();
  REQUIRE(pMeter->GetValue(0) == Approx(0.5).margin(0.01));
  REQUIRE(pMeter->GetValue(1) == Approx(0.5).margin(0.01));
  REQUIRE(pMeter->IsDirty());
  REQUIRE(f.mUI->RenderFrame());
  REQUIRE(f.mUI->mLastDirtyBounds.Contains(kMeterBounds));

  // Turn the gain down from the host: the meter follows the output
  f.mPlug.HostSetParameterNormalized(kGain, 0.5);
  f.Process();
  f.Process();
  f.mPlug.HostIdle();
  REQUIRE(pMeter->GetValue(0) == Approx(0.25).margin(0.01));

  // With nothing new from the processor an idle changes nothing
  f.mUI->RenderUntilClean();
  f.mPlug.HostIdle();
  REQUIRE_FALSE(pMeter->IsDirty());
}

TEST_CASE("Plug-in UI: restoring state updates the controls", "[integration][plugin][state]")
{
  Fixture f;
  IControl* pKnob = f.mUI->GetControlWithTag(kCtrlTagKnob);

  f.mPlug.HostSetParameterNormalized(kGain, 0.2);
  f.mPlug.HostSetParameterNormalized(kMode, 0.5);
  f.mPlug.HostIdle();

  IByteChunk state;
  REQUIRE(f.mPlug.HostSaveState(state));

  f.mPlug.HostSetParameterNormalized(kGain, 0.9);
  f.mPlug.HostSetParameterNormalized(kMode, 1.);
  f.mPlug.HostIdle();
  REQUIRE(pKnob->GetValue() == Approx(0.9));
  f.mUI->RenderUntilClean();
  f.mPlug.ClearRecorded();

  REQUIRE(f.mPlug.HostLoadState(state) > 0);
  f.mPlug.HostIdle();

  REQUIRE(f.mPlug.GetParam(kGain)->Value() == Approx(20.));
  REQUIRE(f.mPlug.GetParam(kMode)->Int() == 1);
  REQUIRE(pKnob->GetValue() == Approx(0.2));
  REQUIRE(f.mUI->GetControlWithTag(kCtrlTagSlider)->GetValue() == Approx(0.2));
  REQUIRE(f.mUI->GetControlWithTag(kCtrlTagMode)->GetValue() == Approx(0.5));
  REQUIRE(pKnob->IsDirty());
  REQUIRE(f.mUI->RenderFrame());

  // Restoring state is not the user editing parameters
  REQUIRE(f.mPlug.mParamEvents.empty());

  f.Process();
  REQUIRE(f.mOut[0][0] == Approx(-0.5 * 0.2));
}

TEST_CASE("Plug-in UI: notes played on the on-screen keyboard reach the processor on the next block", "[integration][plugin][midi]")
{
  Fixture f;
  // Two octaves from C3 across the keyboard: 15 white keys
  const float whiteKeyWidth = kKeyboardBounds.W() / 15.f;
  const float y = kKeyboardBounds.B - 5.f;
  const float xC3 = kKeyboardBounds.L + 0.5f * whiteKeyWidth;
  const float xG3 = kKeyboardBounds.L + 4.5f * whiteKeyWidth;

  f.mUI->MouseDown(xC3, y);
  // The message is queued for the audio thread
  REQUIRE(f.mPlug.mMidiReceived.empty());

  f.Process();
  REQUIRE(f.mPlug.mMidiReceived.size() == 1);
  REQUIRE(f.mPlug.mMidiReceived[0].StatusMsg() == IMidiMsg::kNoteOn);
  REQUIRE(f.mPlug.mMidiReceived[0].NoteNumber() == 48);
  REQUIRE(f.mPlug.mMidiReceived[0].Velocity() > 0);

  f.mUI->MouseDragTo(xG3, y);
  f.mUI->MouseUp(xG3, y);
  f.Process();

  REQUIRE(f.mPlug.mMidiReceived.size() == 4);
  REQUIRE(f.mPlug.mMidiReceived[1].StatusMsg() == IMidiMsg::kNoteOn);
  REQUIRE(f.mPlug.mMidiReceived[1].NoteNumber() == 55);
  REQUIRE(f.mPlug.mMidiReceived[2].StatusMsg() == IMidiMsg::kNoteOff);
  REQUIRE(f.mPlug.mMidiReceived[2].NoteNumber() == 48);
  REQUIRE(f.mPlug.mMidiReceived[3].StatusMsg() == IMidiMsg::kNoteOff);
  REQUIRE(f.mPlug.mMidiReceived[3].NoteNumber() == 55);

  // Each message is delivered once
  f.Process();
  REQUIRE(f.mPlug.mMidiReceived.size() == 4);
  REQUIRE(f.mPlug.mParamEvents.empty());
}

TEST_CASE("Plug-in UI: closing the editor while the plug-in is running is safe", "[integration][plugin][lifecycle]")
{
  Fixture f;

  // Mid-gesture, with meter data in flight
  f.mUI->MouseDown(kKnobBounds.MW(), kKnobBounds.MH());
  f.mUI->MouseDragTo(kKnobBounds.MW(), kKnobBounds.MH() + 10.f);
  f.Process();

  f.mPlug.CloseWindow();
  REQUIRE(f.mPlug.GetUI() == nullptr);
  REQUIRE(f.mPlug.mNumUICloses == 1);

  // The processor and the host carry on with no UI
  const double gain = f.mPlug.GetParam(kGain)->GetNormalized();
  f.Process();
  f.mPlug.HostIdle();
  f.mPlug.HostSetParameterNormalized(kGain, 0.5);
  f.mPlug.HostIdle();
  f.Process();
  f.mPlug.HostIdle();
  REQUIRE(gain < 1.);
  REQUIRE(f.mOut[0][0] == Approx(0.25f));

  IByteChunk state;
  REQUIRE(f.mPlug.HostSaveState(state));
  REQUIRE(f.mPlug.HostLoadState(state) > 0);
}

TEST_CASE("Plug-in UI: reopening the editor shows the current state", "[integration][plugin][lifecycle]")
{
  Fixture f;

  f.mPlug.CloseWindow();
  f.mPlug.HostSetParameterNormalized(kGain, 0.4);
  f.mPlug.HostSetParameterNormalized(kMode, 1.);
  f.mPlug.HostIdle();

  REQUIRE(f.mPlug.OpenWindow(nullptr) != nullptr);
  auto* pUI = f.mPlug.UI();

  REQUIRE(f.mPlug.mNumLayouts == 2);
  REQUIRE(pUI->NControls() == 6);
  REQUIRE(pUI->GetControlWithTag(kCtrlTagKnob)->GetValue() == Approx(0.4));
  REQUIRE(pUI->GetControlWithTag(kCtrlTagMode)->GetValue() == Approx(1.));
  REQUIRE(pUI->RenderFrame());

  // ... and it works: the user can still change things
  pUI->DblClick(kKnobBounds.MW(), kKnobBounds.MH());
  REQUIRE(f.mPlug.GetParam(kGain)->Value() == Approx(100.));
}

TEST_CASE("Plug-in UI: resizing the UI asks the host to resize the editor", "[integration][plugin][resize]")
{
  Fixture f;
  const int nRequests = f.mPlug.mNumEditorResizeRequests;

  f.mUI->Resize(500, 400, 1.f);

  REQUIRE(f.mPlug.mNumEditorResizeRequests == nRequests + 1);
  REQUIRE(f.mPlug.GetEditorWidth() == 500);
  REQUIRE(f.mPlug.GetEditorHeight() == 400);

  // The size is part of the editor state, and comes back when the editor is reopened
  f.mPlug.CloseWindow();
  REQUIRE(f.mPlug.OpenWindow(nullptr) != nullptr);
  REQUIRE(f.mPlug.UI()->Width() == 500);
  REQUIRE(f.mPlug.UI()->Height() == 400);
}
