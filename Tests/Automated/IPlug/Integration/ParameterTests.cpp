/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests parameter changes travelling between host, DSP and UI, and the IPluginBase parameter range/group helpers.

#include <string>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "AudioBuffers.h"
#include "TestPlugins.h"

using namespace iplug;
using Catch::Approx;

namespace
{

using EType = IPlugTestAPI::ParamEvent::EType;

/** Eight parameters in two groups, which the constructor leaves for the test to initialise where it wants to */
class ParamsPlug : public RecordingPlug
{
public:
  enum EParams
  {
    kOsc1Freq = 0,
    kOsc1Level,
    kOsc2Freq,
    kOsc2Level,
    kCutoff,
    kResonance,
    kVolume,
    kSpare,
    kNumParams
  };

  ParamsPlug(bool initParams = true)
  : RecordingPlug(InstanceInfo{}, []() {
    TestConfig config;
    config.nParams = kNumParams;
    return config;
  }())
  {
    if (!initParams)
      return;

    GetParam(kOsc1Freq)->InitDouble("Osc1 Freq", 440., 20., 20000., 0.1, "Hz", 0, "Osc1");
    GetParam(kOsc1Level)->InitDouble("Osc1 Level", 50., 0., 100., 0.1, "%", 0, "Osc1");
    GetParam(kOsc2Freq)->InitDouble("Osc2 Freq", 880., 20., 20000., 0.1, "Hz", 0, "Osc2");
    GetParam(kOsc2Level)->InitDouble("Osc2 Level", 25., 0., 100., 0.1, "%", 0, "Osc2");
    GetParam(kCutoff)->InitDouble("Cutoff", 1000., 20., 20000., 0.1, "Hz", 0, "Filter");
    GetParam(kResonance)->InitDouble("Resonance", 10., 0., 100., 0.1, "%", 0, "Filter");
    GetParam(kVolume)->InitDouble("Volume", -6., -70., 12., 0.1, "dB");
    GetParam(kSpare)->InitInt("Spare", 3, 0, 10);
  }

  double Value(int paramIdx) { return GetParam(paramIdx)->Value(); }
};

/** A meta-parameter which drives two other parameters, telling the host about the changes it makes to them */
class MetaPlug : public RecordingPlug
{
public:
  enum EParams
  {
    kMacro = 0,
    kLinkedA,
    kLinkedB,
    kIndependent,
    kNumParams
  };

  MetaPlug()
  : RecordingPlug(InstanceInfo{}, []() {
    TestConfig config;
    config.nParams = kNumParams;
    return config;
  }())
  {
    GetParam(kMacro)->InitDouble("Macro", 0., 0., 1., 0.001, "", IParam::kFlagMeta);
    GetParam(kLinkedA)->InitDouble("Linked A", 0., 0., 100., 0.001);
    GetParam(kLinkedB)->InitDouble("Linked B", 10., 10., 20., 0.001);
    GetParam(kIndependent)->InitDouble("Independent", 5., 0., 10., 0.001);
  }

  void OnParamChange(int paramIdx, EParamSource source, int sampleOffset = -1) override
  {
    RecordingPlug::OnParamChange(paramIdx, source, sampleOffset);

    if (paramIdx == kMacro && (source == kUI || source == kHost))
    {
      const double normalizedValue = GetParam(kMacro)->GetNormalized();

      for (int linkedIdx : {kLinkedA, kLinkedB})
      {
        BeginInformHostOfParamChangeFromUI(linkedIdx);
        SendParameterValueFromUI(linkedIdx, normalizedValue);
        EndInformHostOfParamChangeFromUI(linkedIdx);
      }
    }
  }
};

} // namespace

TEST_CASE("Params: host automation reaches the DSP immediately", "[integration][params]")
{
  GainPlug plug;
  plug.HostActivate(48000., 64);

  plug.HostSetParameterNormalized(GainPlug::kGain, 0.75);

  REQUIRE(plug.mParamChanges.size() == 1);
  CHECK(plug.mParamChanges[0].mIdx == GainPlug::kGain);
  CHECK(plug.mParamChanges[0].mSource == kHost);
  CHECK(plug.mParamChanges[0].mValue == Approx(150.));
  CHECK(plug.GetParam(GainPlug::kGain)->Value() == Approx(150.));
  CHECK(plug.GetParam(GainPlug::kGain)->GetNormalized() == Approx(0.75));

  SECTION("the host is not told about a change it made itself")
  {
    CHECK(plug.mParamEvents.empty());
  }

  SECTION("the next block uses the new value")
  {
    AudioBuffers<double> in {2, 64, 0.5}, out {2, 64};
    plug.HostProcess(in.Get(), out.Get(), 64);
    CHECK(out[0][0] == Approx(0.75));
  }

  SECTION("each parameter type is converted from its normalized value")
  {
    plug.HostSetParameterNormalized(GainPlug::kMute, 1.);
    CHECK(plug.GetParam(GainPlug::kMute)->Bool());
    CHECK(plug.mParamChanges.back().mIdx == GainPlug::kMute);
    CHECK(plug.mParamChanges.back().mValue == 1.);

    plug.HostSetParameterNormalized(GainPlug::kMute, 0.);
    CHECK_FALSE(plug.GetParam(GainPlug::kMute)->Bool());

    plug.HostSetParameterNormalized(GainPlug::kMode, 0.5);
    CHECK(plug.GetParam(GainPlug::kMode)->Int() == 1);

    plug.HostSetParameterNormalized(GainPlug::kMode, 1.);
    CHECK(plug.GetParam(GainPlug::kMode)->Int() == 2);

    plug.HostSetParameterNormalized(GainPlug::kMode, 0.);
    CHECK(plug.GetParam(GainPlug::kMode)->Int() == 0);

    CHECK(plug.mParamChanges.size() == 6);

    for (const auto& change : plug.mParamChanges)
      CHECK(change.mSource == kHost);
  }

  SECTION("out of range normalized values are clamped")
  {
    plug.HostSetParameterNormalized(GainPlug::kGain, 1.5);
    CHECK(plug.GetParam(GainPlug::kGain)->Value() == 200.);

    plug.HostSetParameterNormalized(GainPlug::kGain, -0.5);
    CHECK(plug.GetParam(GainPlug::kGain)->Value() == 0.);
  }
}

TEST_CASE("Params: host automation reaches the UI on the idle call", "[integration][params]")
{
  GainPlug plug;

  plug.HostSetParameterNormalized(GainPlug::kGain, 0.75);
  plug.HostSetParameterNormalized(GainPlug::kMute, 1.);

  // Nothing UI related happens on the thread the host called on
  CHECK(plug.mParamChangesUI.empty());
  CHECK(plug.mParamValuesToUI.empty());

  plug.HostIdle();

  REQUIRE(plug.mParamValuesToUI.size() == 2);
  CHECK(plug.mParamValuesToUI[0].mIdx == GainPlug::kGain);
  CHECK(plug.mParamValuesToUI[0].mValue == Approx(150.));
  CHECK_FALSE(plug.mParamValuesToUI[0].mNormalized);
  CHECK(plug.mParamValuesToUI[1].mIdx == GainPlug::kMute);
  CHECK(plug.mParamValuesToUI[1].mValue == 1.);
  CHECK_FALSE(plug.mParamValuesToUI[1].mNormalized);

  REQUIRE(plug.mParamChangesUI.size() == 2);
  CHECK(plug.mParamChangesUI[0].mIdx == GainPlug::kGain);
  CHECK(plug.mParamChangesUI[0].mSource == kDelegate);
  CHECK(plug.mParamChangesUI[1].mIdx == GainPlug::kMute);
  CHECK(plug.mParamChangesUI[1].mSource == kDelegate);

  SECTION("each change is delivered once")
  {
    plug.HostIdle();
    CHECK(plug.mParamValuesToUI.size() == 2);
    CHECK(plug.mParamChangesUI.size() == 2);
  }

  SECTION("many changes between two idle calls all arrive, in order")
  {
    plug.ClearAll();

    for (int i = 0; i <= 100; i++)
      plug.HostSetParameterNormalized(GainPlug::kGain, i / 100.);

    plug.HostIdle();

    REQUIRE(plug.mParamValuesToUI.size() == 101);

    for (int i = 0; i <= 100; i++)
      CHECK(plug.mParamValuesToUI[static_cast<size_t>(i)].mValue == Approx(i * 2.));
  }
}

TEST_CASE("Params: SendParameterValueFromAPI() queues values for the UI", "[integration][params]")
{
  GainPlug plug;

  SECTION("non-normalized values are passed on as they are")
  {
    plug.SendParameterValueFromAPI(GainPlug::kGain, 42., false);
    CHECK(plug.mParamValuesToUI.empty());

    plug.HostIdle();
    REQUIRE(plug.mParamValuesToUI.size() == 1);
    CHECK(plug.mParamValuesToUI[0].mIdx == GainPlug::kGain);
    CHECK(plug.mParamValuesToUI[0].mValue == 42.);
    CHECK_FALSE(plug.mParamValuesToUI[0].mNormalized);
  }

  SECTION("normalized values are converted to real values")
  {
    plug.SendParameterValueFromAPI(GainPlug::kGain, 0.25, true);
    plug.SendParameterValueFromAPI(GainPlug::kMode, 1., true);

    plug.HostIdle();
    REQUIRE(plug.mParamValuesToUI.size() == 2);
    CHECK(plug.mParamValuesToUI[0].mValue == Approx(50.));
    CHECK_FALSE(plug.mParamValuesToUI[0].mNormalized);
    CHECK(plug.mParamValuesToUI[1].mIdx == GainPlug::kMode);
    CHECK(plug.mParamValuesToUI[1].mValue == Approx(2.));
  }

  SECTION("it does not change the parameter or call OnParamChange()")
  {
    plug.SendParameterValueFromAPI(GainPlug::kGain, 42., false);
    plug.HostIdle();
    CHECK(plug.GetParam(GainPlug::kGain)->Value() == 100.);
    CHECK(plug.mParamChanges.empty());
  }
}

TEST_CASE("Params: a UI gesture informs the host and updates the DSP", "[integration][params]")
{
  GainPlug plug;
  plug.HostActivate(48000., 64);

  plug.BeginInformHostOfParamChangeFromUI(GainPlug::kGain);
  plug.SendParameterValueFromUI(GainPlug::kGain, 0.1);
  plug.SendParameterValueFromUI(GainPlug::kGain, 0.2);
  plug.SendParameterValueFromUI(GainPlug::kGain, 0.3);
  plug.EndInformHostOfParamChangeFromUI(GainPlug::kGain);

  SECTION("the host sees begin, the normalized values in order, then end")
  {
    REQUIRE(plug.mParamEvents.size() == 5);

    CHECK(plug.mParamEvents[0].mType == EType::Begin);
    CHECK(plug.mParamEvents[1].mType == EType::Value);
    CHECK(plug.mParamEvents[2].mType == EType::Value);
    CHECK(plug.mParamEvents[3].mType == EType::Value);
    CHECK(plug.mParamEvents[4].mType == EType::End);

    for (const auto& event : plug.mParamEvents)
      CHECK(event.mIdx == GainPlug::kGain);

    CHECK(plug.mParamEvents[1].mNormalizedValue == 0.1);
    CHECK(plug.mParamEvents[2].mNormalizedValue == 0.2);
    CHECK(plug.mParamEvents[3].mNormalizedValue == 0.3);
  }

  SECTION("the DSP is told about each value, with the UI as the source")
  {
    REQUIRE(plug.mParamChanges.size() == 3);

    for (const auto& change : plug.mParamChanges)
    {
      CHECK(change.mIdx == GainPlug::kGain);
      CHECK(change.mSource == kUI);
    }

    CHECK(plug.mParamChanges[0].mValue == Approx(20.));
    CHECK(plug.mParamChanges[1].mValue == Approx(40.));
    CHECK(plug.mParamChanges[2].mValue == Approx(60.));
    CHECK(plug.GetParam(GainPlug::kGain)->Value() == Approx(60.));

    AudioBuffers<double> in {2, 64, 1.}, out {2, 64};
    plug.HostProcess(in.Get(), out.Get(), 64);
    CHECK(out[1][63] == Approx(0.6));
  }

  SECTION("OnParamChangeUI() is called for each value, with the UI as the source")
  {
    REQUIRE(plug.mParamChangesUI.size() == 3);

    for (const auto& change : plug.mParamChangesUI)
    {
      CHECK(change.mIdx == GainPlug::kGain);
      CHECK(change.mSource == kUI);
    }

    CHECK(plug.mParamChangesUI[2].mValue == Approx(60.));
  }

  SECTION("a change which came from the UI is not sent back to it on idle")
  {
    plug.HostIdle();
    CHECK(plug.mParamValuesToUI.empty());
    CHECK(plug.mParamChangesUI.size() == 3);
  }
}

TEST_CASE("Params: gestures on different parameters are reported separately", "[integration][params]")
{
  GainPlug plug;

  plug.BeginInformHostOfParamChangeFromUI(GainPlug::kMute);
  plug.SendParameterValueFromUI(GainPlug::kMute, 1.);
  plug.EndInformHostOfParamChangeFromUI(GainPlug::kMute);

  plug.BeginInformHostOfParamChangeFromUI(GainPlug::kMode);
  plug.SendParameterValueFromUI(GainPlug::kMode, 1.);
  plug.EndInformHostOfParamChangeFromUI(GainPlug::kMode);

  REQUIRE(plug.mParamEvents.size() == 6);

  const int expectedIdx[] = {GainPlug::kMute, GainPlug::kMute, GainPlug::kMute, GainPlug::kMode, GainPlug::kMode, GainPlug::kMode};
  const EType expectedType[] = {EType::Begin, EType::Value, EType::End, EType::Begin, EType::Value, EType::End};

  for (size_t i = 0; i < 6; i++)
  {
    CHECK(plug.mParamEvents[i].mIdx == expectedIdx[i]);
    CHECK(plug.mParamEvents[i].mType == expectedType[i]);
  }

  CHECK(plug.GetParam(GainPlug::kMute)->Bool());
  CHECK(plug.GetParam(GainPlug::kMode)->Int() == 2);
}

TEST_CASE("Params: SetParameterValue() updates the parameter, the host and the DSP", "[integration][params]")
{
  GainPlug plug;

  plug.SetParameterValue(GainPlug::kGain, 0.4);

  CHECK(plug.GetParam(GainPlug::kGain)->Value() == Approx(80.));

  REQUIRE(plug.mParamEvents.size() == 1);
  CHECK(plug.mParamEvents[0].mType == EType::Value);
  CHECK(plug.mParamEvents[0].mIdx == GainPlug::kGain);
  CHECK(plug.mParamEvents[0].mNormalizedValue == 0.4);

  REQUIRE(plug.mParamChanges.size() == 1);
  CHECK(plug.mParamChanges[0].mIdx == GainPlug::kGain);
  CHECK(plug.mParamChanges[0].mSource == kUI);
  CHECK(plug.mParamChanges[0].mValue == Approx(80.));

  // It is the DSP-side half of a UI change: the UI is not notified
  CHECK(plug.mParamChangesUI.empty());
}

TEST_CASE("Params: DirtyParametersFromUI() sends every normalized value to the host", "[integration][params]")
{
  GainPlug plug;

  plug.GetParam(GainPlug::kGain)->Set(50.);
  plug.GetParam(GainPlug::kMute)->Set(1.);
  plug.GetParam(GainPlug::kMode)->Set(1.);

  plug.DirtyParametersFromUI();

  REQUIRE(plug.mParamEvents.size() == 3);

  for (int i = 0; i < 3; i++)
  {
    CHECK(plug.mParamEvents[static_cast<size_t>(i)].mType == EType::Value);
    CHECK(plug.mParamEvents[static_cast<size_t>(i)].mIdx == i);
  }

  CHECK(plug.mParamEvents[0].mNormalizedValue == Approx(0.25));
  CHECK(plug.mParamEvents[1].mNormalizedValue == Approx(1.));
  CHECK(plug.mParamEvents[2].mNormalizedValue == Approx(0.5));

  // The values themselves have not changed, so the DSP has nothing to do
  CHECK(plug.mParamChanges.empty());
}

TEST_CASE("Params: OnParamReset() notifies DSP and UI about every parameter", "[integration][params]")
{
  GainPlug plug;

  plug.OnParamReset(kReset);

  REQUIRE(plug.mParamChanges.size() == 3);
  REQUIRE(plug.mParamChangesUI.size() == 3);

  for (int i = 0; i < 3; i++)
  {
    CHECK(plug.mParamChanges[static_cast<size_t>(i)].mIdx == i);
    CHECK(plug.mParamChanges[static_cast<size_t>(i)].mSource == kReset);
    CHECK(plug.mParamChangesUI[static_cast<size_t>(i)].mIdx == i);
    CHECK(plug.mParamChangesUI[static_cast<size_t>(i)].mSource == kReset);
  }

  CHECK(plug.mParamChanges[0].mValue == 100.);
}

TEST_CASE("Params: the legacy single-argument OnParamChange() is still called", "[integration][params]")
{
  class LegacyPlug : public Plugin
  {
  public:
    LegacyPlug()
    : Plugin(InstanceInfo{}, GainPlug::MakeConfig().Make())
    {
      GetParam(0)->InitDouble("Gain", 100., 0., 200., 0.01);
    }

    void OnParamChange(int paramIdx) override { mChanged.push_back(paramIdx); }

    std::vector<int> mChanged;
  };

  LegacyPlug plug;
  plug.HostSetParameterNormalized(0, 0.5);
  plug.SetParameterValue(0, 0.25);

  CHECK(plug.mChanged == std::vector<int>{0, 0});
}

TEST_CASE("Params: a meta-parameter moves its linked parameters and tells the host", "[integration][params][meta]")
{
  MetaPlug plug;

  REQUIRE((plug.GetParam(MetaPlug::kMacro)->GetFlags() & IParam::kFlagMeta) != 0);
  REQUIRE((plug.GetParam(MetaPlug::kLinkedA)->GetFlags() & IParam::kFlagMeta) == 0);

  SECTION("moved by the user")
  {
    plug.BeginInformHostOfParamChangeFromUI(MetaPlug::kMacro);
    plug.SendParameterValueFromUI(MetaPlug::kMacro, 0.5);
    plug.EndInformHostOfParamChangeFromUI(MetaPlug::kMacro);

    CHECK(plug.GetParam(MetaPlug::kMacro)->Value() == Approx(0.5));
    CHECK(plug.GetParam(MetaPlug::kLinkedA)->Value() == Approx(50.));
    CHECK(plug.GetParam(MetaPlug::kLinkedB)->Value() == Approx(15.));
    CHECK(plug.GetParam(MetaPlug::kIndependent)->Value() == 5.);

    // Macro: begin, value, [A: begin, value, end], [B: begin, value, end], end
    REQUIRE(plug.mParamEvents.size() == 9);
    CHECK(plug.mParamEvents[0].mType == EType::Begin);
    CHECK(plug.mParamEvents[0].mIdx == MetaPlug::kMacro);
    CHECK(plug.mParamEvents[1].mType == EType::Value);
    CHECK(plug.mParamEvents[1].mIdx == MetaPlug::kMacro);
    CHECK(plug.mParamEvents[8].mType == EType::End);
    CHECK(plug.mParamEvents[8].mIdx == MetaPlug::kMacro);

    for (size_t linked = 0; linked < 2; linked++)
    {
      const int linkedIdx = linked == 0 ? MetaPlug::kLinkedA : MetaPlug::kLinkedB;
      const size_t first = 2 + linked * 3;

      CHECK(plug.mParamEvents[first].mType == EType::Begin);
      CHECK(plug.mParamEvents[first + 1].mType == EType::Value);
      CHECK(plug.mParamEvents[first + 1].mNormalizedValue == Approx(0.5));
      CHECK(plug.mParamEvents[first + 2].mType == EType::End);

      for (size_t i = first; i < first + 3; i++)
        CHECK(plug.mParamEvents[i].mIdx == linkedIdx);
    }

    // The DSP heard about all three
    CHECK(plug.CountParamChanges(MetaPlug::kMacro, kUI) == 1);
    CHECK(plug.CountParamChanges(MetaPlug::kLinkedA, kUI) == 1);
    CHECK(plug.CountParamChanges(MetaPlug::kLinkedB, kUI) == 1);
    CHECK(plug.CountParamChanges(MetaPlug::kIndependent, kUI) == 0);
  }

  SECTION("automated by the host")
  {
    plug.HostSetParameterNormalized(MetaPlug::kMacro, 1.);

    CHECK(plug.GetParam(MetaPlug::kLinkedA)->Value() == Approx(100.));
    CHECK(plug.GetParam(MetaPlug::kLinkedB)->Value() == Approx(20.));

    // The host is only told about the parameters it didn't set itself
    REQUIRE(plug.mParamEvents.size() == 6);

    for (const auto& event : plug.mParamEvents)
      CHECK(event.mIdx != MetaPlug::kMacro);

    CHECK(plug.mParamEvents[1].mIdx == MetaPlug::kLinkedA);
    CHECK(plug.mParamEvents[1].mNormalizedValue == Approx(1.));
    CHECK(plug.mParamEvents[4].mIdx == MetaPlug::kLinkedB);
    CHECK(plug.mParamEvents[4].mNormalizedValue == Approx(1.));

    CHECK(plug.CountParamChanges(MetaPlug::kMacro, kHost) == 1);
  }

  SECTION("moving a linked parameter does not move the others")
  {
    plug.SendParameterValueFromUI(MetaPlug::kLinkedA, 0.25);

    CHECK(plug.GetParam(MetaPlug::kLinkedA)->Value() == Approx(25.));
    CHECK(plug.GetParam(MetaPlug::kMacro)->Value() == 0.);
    CHECK(plug.GetParam(MetaPlug::kLinkedB)->Value() == 10.);
    CHECK(plug.mParamEvents.size() == 1);
  }
}

TEST_CASE("Params: parameter groups", "[integration][params]")
{
  ParamsPlug plug;

  SECTION("each parameter remembers its group")
  {
    CHECK(std::string(plug.GetParam(ParamsPlug::kOsc1Freq)->GetGroup()) == "Osc1");
    CHECK(std::string(plug.GetParam(ParamsPlug::kOsc2Level)->GetGroup()) == "Osc2");
    CHECK(std::string(plug.GetParam(ParamsPlug::kResonance)->GetGroup()) == "Filter");
    CHECK(std::string(plug.GetParam(ParamsPlug::kVolume)->GetGroup()).empty());
  }

  SECTION("the list of unique group names is built up by the API class")
  {
    CHECK(plug.NParamGroups() == 0);
    CHECK(plug.AddParamGroup("Osc1") == 1);
    CHECK(plug.AddParamGroup("Filter") == 2);
    CHECK(plug.NParamGroups() == 2);
    CHECK(std::string(plug.GetParamGroupName(0)) == "Osc1");
    CHECK(std::string(plug.GetParamGroupName(1)) == "Filter");
  }

  SECTION("ForParamInGroup() visits exactly the members of the group, in order")
  {
    std::vector<int> visited;
    plug.ForParamInGroup("Filter", [&](int paramIdx, IParam& param) {
      visited.push_back(paramIdx);
      CHECK(&param == plug.GetParam(paramIdx));
    });
    CHECK(visited == std::vector<int>{ParamsPlug::kCutoff, ParamsPlug::kResonance});

    visited.clear();
    plug.ForParamInGroup("Osc1", [&](int paramIdx, IParam&) { visited.push_back(paramIdx); });
    CHECK(visited == std::vector<int>{ParamsPlug::kOsc1Freq, ParamsPlug::kOsc1Level});

    visited.clear();
    plug.ForParamInGroup("NoSuchGroup", [&](int paramIdx, IParam&) { visited.push_back(paramIdx); });
    CHECK(visited.empty());

    // Group names are matched exactly
    plug.ForParamInGroup("Osc", [&](int paramIdx, IParam&) { visited.push_back(paramIdx); });
    plug.ForParamInGroup("filter", [&](int paramIdx, IParam&) { visited.push_back(paramIdx); });
    CHECK(visited.empty());
  }

  SECTION("ungrouped parameters form the empty group")
  {
    std::vector<int> visited;
    plug.ForParamInGroup("", [&](int paramIdx, IParam&) { visited.push_back(paramIdx); });
    CHECK(visited == std::vector<int>{ParamsPlug::kVolume, ParamsPlug::kSpare});
  }
}

TEST_CASE("Params: ForParamInRange() is inclusive of both ends", "[integration][params]")
{
  ParamsPlug plug;
  std::vector<int> visited;

  plug.ForParamInRange(2, 5, [&](int paramIdx, IParam& param) {
    visited.push_back(paramIdx);
    CHECK(&param == plug.GetParam(paramIdx));
  });
  CHECK(visited == std::vector<int>{2, 3, 4, 5});

  visited.clear();
  plug.ForParamInRange(7, 7, [&](int paramIdx, IParam&) { visited.push_back(paramIdx); });
  CHECK(visited == std::vector<int>{7});

  visited.clear();
  plug.ForParamInRange(3, 2, [&](int paramIdx, IParam&) { visited.push_back(paramIdx); });
  CHECK(visited.empty());

  SECTION("it can modify the parameters")
  {
    plug.ForParamInRange(0, ParamsPlug::kNumParams - 1, [](int, IParam& param) { param.SetNormalized(1.); });

    CHECK(plug.Value(ParamsPlug::kOsc1Freq) == 20000.);
    CHECK(plug.Value(ParamsPlug::kVolume) == 12.);
    CHECK(plug.Value(ParamsPlug::kSpare) == 10.);
  }
}

TEST_CASE("Params: InitParamRange() initialises a run of similar parameters", "[integration][params]")
{
  ParamsPlug plug {false};

  plug.InitParamRange(2, 5, 1, "Band %i Gain", -3., -24., 24., 0.5, "dB", IParam::kFlagCannotAutomate, "EQ");

  const char* expectedNames[] = {"Band 1 Gain", "Band 2 Gain", "Band 3 Gain", "Band 4 Gain"};

  for (int i = 0; i < 4; i++)
  {
    const IParam* pParam = plug.GetParam(2 + i);
    INFO("param " << (2 + i));

    CHECK(std::string(pParam->GetName()) == expectedNames[i]);
    CHECK(pParam->Type() == IParam::kTypeDouble);
    CHECK(pParam->Value() == -3.);
    CHECK(pParam->GetDefault() == -3.);
    CHECK(pParam->GetMin() == -24.);
    CHECK(pParam->GetMax() == 24.);
    CHECK(pParam->GetStep() == 0.5);
    CHECK(std::string(pParam->GetLabel()) == "dB");
    CHECK(std::string(pParam->GetGroup()) == "EQ");
    CHECK_FALSE(pParam->GetCanAutomate());
  }

  // Parameters outside the range are untouched
  CHECK(plug.GetParam(1)->Type() == IParam::kTypeNone);
  CHECK(plug.GetParam(6)->Type() == IParam::kTypeNone);

  SECTION("the count can start anywhere")
  {
    plug.InitParamRange(0, 1, 0, "Slot %i", 0., 0., 1., 0.01);
    CHECK(std::string(plug.GetParam(0)->GetName()) == "Slot 0");
    CHECK(std::string(plug.GetParam(1)->GetName()) == "Slot 1");

    plug.InitParamRange(6, 7, 10, "Slot %i", 0., 0., 1., 0.01);
    CHECK(std::string(plug.GetParam(6)->GetName()) == "Slot 10");
    CHECK(std::string(plug.GetParam(7)->GetName()) == "Slot 11");
  }
}

TEST_CASE("Params: CloneParamRange() copies parameter definitions and values", "[integration][params]")
{
  ParamsPlug plug {false};

  plug.GetParam(0)->InitDouble("Osc1 Freq", 440., 20., 20000., 0.1, "Hz", IParam::kFlagCannotAutomate, "Osc1");
  plug.GetParam(1)->InitDouble("Osc1 Level", 50., 0., 100., 0.1, "%", 0, "Osc1");
  plug.GetParam(0)->Set(1234.);

  SECTION("plain clone")
  {
    plug.CloneParamRange(0, 1, 2);

    const IParam* pFreq = plug.GetParam(2);
    CHECK(std::string(pFreq->GetName()) == "Osc1 Freq");
    CHECK(std::string(pFreq->GetGroup()) == "Osc1");
    CHECK(std::string(pFreq->GetLabel()) == "Hz");
    CHECK(pFreq->GetMin() == 20.);
    CHECK(pFreq->GetMax() == 20000.);
    CHECK(pFreq->GetDefault() == 440.);
    CHECK(pFreq->GetStep() == 0.1);
    CHECK_FALSE(pFreq->GetCanAutomate());
    CHECK(pFreq->Value() == 1234.); // the current value is cloned too, not just the default

    const IParam* pLevel = plug.GetParam(3);
    CHECK(std::string(pLevel->GetName()) == "Osc1 Level");
    CHECK(pLevel->GetMax() == 100.);
    CHECK(pLevel->Value() == 50.);
    CHECK(pLevel->GetCanAutomate());

    // The originals are unchanged
    CHECK(std::string(plug.GetParam(0)->GetName()) == "Osc1 Freq");
    CHECK(plug.GetParam(0)->Value() == 1234.);
  }

  SECTION("with a name substitution at the start of the name and a new group")
  {
    plug.CloneParamRange(0, 1, 4, "Osc1", "Osc2", "Osc2");

    CHECK(std::string(plug.GetParam(4)->GetName()) == "Osc2 Freq");
    CHECK(std::string(plug.GetParam(5)->GetName()) == "Osc2 Level");
    CHECK(std::string(plug.GetParam(4)->GetGroup()) == "Osc2");
    CHECK(std::string(plug.GetParam(5)->GetGroup()) == "Osc2");
    CHECK(plug.GetParam(4)->Value() == 1234.);

    CHECK(std::string(plug.GetParam(0)->GetName()) == "Osc1 Freq");
    CHECK(std::string(plug.GetParam(0)->GetGroup()) == "Osc1");
  }

  SECTION("a search string which is not found leaves the name alone")
  {
    plug.CloneParamRange(0, 0, 6, "Nope", "Yes");
    CHECK(std::string(plug.GetParam(6)->GetName()) == "Osc1 Freq");
  }
}

TEST_CASE("Params: CloneParamRange() substitutes a search string in the middle of the name", "[integration][params]")
{
  ParamsPlug plug {false};

  plug.GetParam(0)->InitDouble("Freq 1 (Hz)", 440., 20., 20000., 0.1);
  plug.CloneParamRange(0, 0, 1, "1", "2");

  CHECK(std::string(plug.GetParam(1)->GetName()) == "Freq 2 (Hz)");
}

TEST_CASE("Params: resetting parameters to their defaults", "[integration][params]")
{
  ParamsPlug plug;

  auto setAllToMax = [&]() { plug.ForParamInRange(0, ParamsPlug::kNumParams - 1, [](int, IParam& param) { param.SetNormalized(1.); }); };
  const double defaults[] = {440., 50., 880., 25., 1000., 10., -6., 3.};

  setAllToMax();
  REQUIRE(plug.Value(ParamsPlug::kOsc1Freq) == 20000.);

  SECTION("all")
  {
    plug.DefaultParamValues();

    for (int i = 0; i < ParamsPlug::kNumParams; i++)
      CHECK(plug.Value(i) == defaults[i]);
  }

  SECTION("a range, inclusive of both ends")
  {
    plug.DefaultParamValues(ParamsPlug::kOsc2Freq, ParamsPlug::kCutoff);

    CHECK(plug.Value(ParamsPlug::kOsc1Level) == 100.);
    CHECK(plug.Value(ParamsPlug::kOsc2Freq) == 880.);
    CHECK(plug.Value(ParamsPlug::kOsc2Level) == 25.);
    CHECK(plug.Value(ParamsPlug::kCutoff) == 1000.);
    CHECK(plug.Value(ParamsPlug::kResonance) == 100.);
  }

  SECTION("a group")
  {
    plug.DefaultParamValues("Filter");

    CHECK(plug.Value(ParamsPlug::kCutoff) == 1000.);
    CHECK(plug.Value(ParamsPlug::kResonance) == 10.);
    CHECK(plug.Value(ParamsPlug::kOsc1Freq) == 20000.);
    CHECK(plug.Value(ParamsPlug::kOsc2Level) == 100.);
    CHECK(plug.Value(ParamsPlug::kVolume) == 12.);
  }

  // These helpers only change the values: notifying DSP, UI and host is up to the caller
  CHECK(plug.mParamChanges.empty());
  CHECK(plug.mParamEvents.empty());
}

TEST_CASE("Params: randomising parameters keeps them in range", "[integration][params]")
{
  ParamsPlug plug;

  auto inRange = [&](int paramIdx) {
    const IParam* pParam = plug.GetParam(paramIdx);
    return pParam->Value() >= pParam->GetMin() && pParam->Value() <= pParam->GetMax();
  };

  SECTION("all")
  {
    for (int repeat = 0; repeat < 20; repeat++)
    {
      plug.RandomiseParamValues();

      for (int i = 0; i < ParamsPlug::kNumParams; i++)
        CHECK(inRange(i));

      // An integer parameter stays on integer values
      CHECK(plug.Value(ParamsPlug::kSpare) == static_cast<double>(plug.GetParam(ParamsPlug::kSpare)->Int()));
    }
  }

  SECTION("a range leaves the other parameters alone")
  {
    plug.RandomiseParamValues(ParamsPlug::kOsc2Freq, ParamsPlug::kOsc2Level);

    CHECK(inRange(ParamsPlug::kOsc2Freq));
    CHECK(inRange(ParamsPlug::kOsc2Level));
    CHECK(plug.Value(ParamsPlug::kOsc1Freq) == 440.);
    CHECK(plug.Value(ParamsPlug::kOsc1Level) == 50.);
    CHECK(plug.Value(ParamsPlug::kCutoff) == 1000.);
    CHECK(plug.Value(ParamsPlug::kSpare) == 3.);
  }

  SECTION("a group leaves the other parameters alone")
  {
    plug.RandomiseParamValues("Filter");

    CHECK(inRange(ParamsPlug::kCutoff));
    CHECK(inRange(ParamsPlug::kResonance));
    CHECK(plug.Value(ParamsPlug::kOsc1Freq) == 440.);
    CHECK(plug.Value(ParamsPlug::kOsc2Level) == 25.);
    CHECK(plug.Value(ParamsPlug::kVolume) == -6.);
  }
}

TEST_CASE("Params: copying parameter values", "[integration][params]")
{
  ParamsPlug plug;

  SECTION("by index")
  {
    plug.GetParam(ParamsPlug::kOsc1Freq)->Set(100.);
    plug.GetParam(ParamsPlug::kOsc1Level)->Set(75.);

    plug.CopyParamValues(ParamsPlug::kOsc1Freq, ParamsPlug::kCutoff, 2);

    CHECK(plug.Value(ParamsPlug::kCutoff) == 100.);
    CHECK(plug.Value(ParamsPlug::kResonance) == 75.);

    // Source and everything else unchanged
    CHECK(plug.Value(ParamsPlug::kOsc1Freq) == 100.);
    CHECK(plug.Value(ParamsPlug::kOsc1Level) == 75.);
    CHECK(plug.Value(ParamsPlug::kOsc2Freq) == 880.);
    CHECK(plug.Value(ParamsPlug::kVolume) == -6.);
  }

  SECTION("by group")
  {
    plug.GetParam(ParamsPlug::kOsc1Freq)->Set(123.);
    plug.GetParam(ParamsPlug::kOsc1Level)->Set(99.);

    plug.CopyParamValues("Osc1", "Osc2");

    CHECK(plug.Value(ParamsPlug::kOsc2Freq) == 123.);
    CHECK(plug.Value(ParamsPlug::kOsc2Level) == 99.);
    CHECK(plug.Value(ParamsPlug::kOsc1Freq) == 123.);
    CHECK(plug.Value(ParamsPlug::kCutoff) == 1000.);
  }

  SECTION("copied values are constrained to the destination's range")
  {
    plug.GetParam(ParamsPlug::kOsc1Freq)->Set(15000.);
    plug.GetParam(ParamsPlug::kOsc1Level)->Set(0.);

    // Osc1 Freq (max 20000) -> Resonance (max 100)
    plug.CopyParamValues(ParamsPlug::kOsc1Freq, ParamsPlug::kResonance, 1);
    CHECK(plug.Value(ParamsPlug::kResonance) == 100.);
  }
}

TEST_CASE("Params: PrintParamValues() copes with every parameter type", "[integration][params]")
{
  GainPlug plug;
  plug.PrintParamValues();

  ParamsPlug uninitialised {false};
  uninitialised.PrintParamValues();

  SUCCEED();
}

TEST_CASE("Params: parameters can be added dynamically", "[integration][params]")
{
  GainPlug plug;
  REQUIRE(plug.NParams() == 3);

  IParam* pParam = plug.AddParam();
  REQUIRE(pParam != nullptr);
  pParam->InitDouble("Extra", 1., 0., 2., 0.1);

  CHECK(plug.NParams() == 4);
  CHECK(plug.GetParam(3) == pParam);

  SECTION("the new parameter takes part in automation and state")
  {
    plug.HostSetParameterNormalized(3, 1.);
    CHECK(plug.GetParam(3)->Value() == 2.);
    CHECK(plug.mParamChanges.back().mIdx == 3);

    IByteChunk chunk;
    REQUIRE(plug.SerializeParams(chunk));
    CHECK(chunk.Size() == 4 * static_cast<int>(sizeof(double)));
  }
}

// BUG: IEditorDelegate::RemoveParam() (IPlug/IPlugEditorDelegate.h) calls WDL_PtrList::Delete(idx), which removes the
// pointer from the list without deleting the IParam that AddParam()/the constructor allocated, so the parameter is
// leaked. The assertions below pass; the bug shows up as a leak report when this is run under LeakSanitizer.
TEST_CASE("Params: removing a parameter shifts the following ones down and frees it", "[.][bug]")
{
  GainPlug plug;

  plug.RemoveParam(0);
  CHECK(plug.NParams() == 2);
  CHECK(std::string(plug.GetParam(0)->GetName()) == "Mute");
  CHECK(std::string(plug.GetParam(1)->GetName()) == "Mode");
}

TEST_CASE("Params: the host can be told that parameter details changed", "[integration][params]")
{
  GainPlug plug;
  CHECK(plug.mNumParameterDetailsChanges == 0);
  plug.InformHostOfParameterDetailsChange();
  CHECK(plug.mNumParameterDetailsChanges == 1);
}
