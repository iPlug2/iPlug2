/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the baked-in factory preset mechanism of IPluginBase: creating, restoring, modifying and serializing presets.

#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "TestPlugins.h"
#include "wdl_base64.h"

using namespace iplug;

namespace
{

/** A plug-in with one parameter of each type and a bank of presets, which the tests fill in */
class PresetPlug : public RecordingPlug
{
public:
  enum EParams
  {
    kDrive = 0, // double
    kVoices,    // int
    kOn,        // bool
    kShape,     // enum
    kNumParams
  };

  static constexpr int kNumPresets = 4;

  PresetPlug(int nPresets = kNumPresets)
  : RecordingPlug(InstanceInfo{}, [nPresets]() {
    TestConfig config;
    config.nParams = kNumParams;
    config.nPresets = nPresets;
    return config;
  }())
  {
    GetParam(kDrive)->InitDouble("Drive", 1., 0., 10., 0.01);
    GetParam(kVoices)->InitInt("Voices", 4, 1, 8);
    GetParam(kOn)->InitBool("On", false);
    GetParam(kShape)->InitEnum("Shape", 0, {"Sine", "Saw", "Square"});
  }

  /** Three factory presets and one unused slot */
  void MakeFactoryPresets()
  {
    MakePreset("Clean", 0.5, 1, false, 0);
    MakePreset("Crunch", 5., 2, true, 1);
    MakePreset("Lead", 9.75, 8, true, 2);
  }

  void SetValues(double drive, int voices, bool on, int shape)
  {
    GetParam(kDrive)->Set(drive);
    GetParam(kVoices)->Set(voices);
    GetParam(kOn)->Set(on ? 1. : 0.);
    GetParam(kShape)->Set(shape);
  }

  bool HasValues(double drive, int voices, bool on, int shape)
  {
    return GetParam(kDrive)->Value() == drive && GetParam(kVoices)->Int() == voices && GetParam(kOn)->Bool() == on && GetParam(kShape)->Int() == shape;
  }
};

} // namespace

TEST_CASE("Presets: a new plug-in has uninitialised presets", "[integration][presets]")
{
  PresetPlug plug;

  CHECK(plug.NPresets() == 4);
  CHECK(plug.GetCurrentPresetIdx() == 0);

  for (int i = 0; i < 4; i++)
  {
    CHECK(std::string(plug.GetPresetName(i)) == UNUSED_PRESET_NAME);
    REQUIRE(plug.GetPreset(i) != nullptr);
    CHECK_FALSE(plug.GetPreset(i)->mInitialized);
    CHECK(plug.GetPreset(i)->mChunk.Size() == 0);
  }

  SECTION("names of presets which don't exist are empty")
  {
    CHECK(std::string(plug.GetPresetName(4)).empty());
    CHECK(std::string(plug.GetPresetName(-1)).empty());
    CHECK(plug.GetPreset(4) == nullptr);
  }
}

TEST_CASE("Presets: MakePreset() fills the next free preset from a list of values", "[integration][presets]")
{
  PresetPlug plug;
  plug.MakeFactoryPresets();

  CHECK(std::string(plug.GetPresetName(0)) == "Clean");
  CHECK(std::string(plug.GetPresetName(1)) == "Crunch");
  CHECK(std::string(plug.GetPresetName(2)) == "Lead");
  CHECK(std::string(plug.GetPresetName(3)) == UNUSED_PRESET_NAME);

  CHECK(plug.GetPreset(0)->mInitialized);
  CHECK(plug.GetPreset(2)->mInitialized);
  CHECK_FALSE(plug.GetPreset(3)->mInitialized);
  CHECK(plug.GetPreset(1)->mChunk.Size() == PresetPlug::kNumParams * static_cast<int>(sizeof(double)));

  SECTION("making a preset does not change the current parameter values or the current preset")
  {
    CHECK(plug.HasValues(1., 4, false, 0));
    CHECK(plug.GetCurrentPresetIdx() == 0);
    CHECK(plug.mParamChanges.empty());
  }

  SECTION("presets beyond the size of the bank are dropped")
  {
    plug.MakePreset("Fourth", 1., 1, false, 0);
    plug.MakePreset("Fifth", 2., 2, false, 0);

    CHECK(plug.NPresets() == 4);
    CHECK(std::string(plug.GetPresetName(3)) == "Fourth");
    CHECK_FALSE(plug.RestorePreset("Fifth"));
  }
}

TEST_CASE("Presets: RestorePreset() by index", "[integration][presets]")
{
  PresetPlug plug;
  plug.MakeFactoryPresets();

  REQUIRE(plug.RestorePreset(1));

  CHECK(plug.HasValues(5., 2, true, 1));
  CHECK(plug.GetCurrentPresetIdx() == 1);

  SECTION("DSP and UI are told about every parameter, as a preset recall")
  {
    REQUIRE(plug.mParamChanges.size() == 4);
    REQUIRE(plug.mParamChangesUI.size() >= 4);

    for (int i = 0; i < 4; i++)
    {
      CHECK(plug.mParamChanges[static_cast<size_t>(i)].mIdx == i);
      CHECK(plug.mParamChanges[static_cast<size_t>(i)].mSource == kPresetRecall);
      CHECK(plug.mParamChangesUI[static_cast<size_t>(i)].mIdx == i);
      CHECK(plug.mParamChangesUI[static_cast<size_t>(i)].mSource == kPresetRecall);
    }

    CHECK(plug.mParamChanges[0].mValue == 5.);
  }

  SECTION("OnRestoreState() and OnPresetsModified() are called")
  {
    CHECK(plug.mNumRestoreStateCalls == 1);
    CHECK(plug.mNumPresetsModifiedCalls == 1);

    // ...which pushes the new values to the UI
    CHECK(plug.mParamValuesToUI.size() == 4);
  }

  SECTION("each preset can be restored in turn")
  {
    REQUIRE(plug.RestorePreset(2));
    CHECK(plug.HasValues(9.75, 8, true, 2));
    CHECK(plug.GetCurrentPresetIdx() == 2);

    REQUIRE(plug.RestorePreset(0));
    CHECK(plug.HasValues(0.5, 1, false, 0));
    CHECK(plug.GetCurrentPresetIdx() == 0);

    CHECK(plug.mNumRestoreStateCalls == 3);
  }

  SECTION("an invalid index fails and changes nothing")
  {
    plug.ClearAll();

    CHECK_FALSE(plug.RestorePreset(4));
    CHECK_FALSE(plug.RestorePreset(-1));
    CHECK_FALSE(plug.RestorePreset(1000));

    CHECK(plug.HasValues(5., 2, true, 1));
    CHECK(plug.GetCurrentPresetIdx() == 1);
    CHECK(plug.mNumRestoreStateCalls == 0);
    CHECK(plug.mParamChanges.empty());
  }
}

TEST_CASE("Presets: RestorePreset() by name", "[integration][presets]")
{
  PresetPlug plug;
  plug.MakeFactoryPresets();

  REQUIRE(plug.RestorePreset("Lead"));
  CHECK(plug.HasValues(9.75, 8, true, 2));
  CHECK(plug.GetCurrentPresetIdx() == 2);
  CHECK(plug.mNumRestoreStateCalls == 1);

  REQUIRE(plug.RestorePreset("Clean"));
  CHECK(plug.HasValues(0.5, 1, false, 0));
  CHECK(plug.GetCurrentPresetIdx() == 0);

  SECTION("unknown, empty and null names fail and change nothing")
  {
    plug.ClearAll();

    CHECK_FALSE(plug.RestorePreset("No Such Preset"));
    CHECK_FALSE(plug.RestorePreset("clean")); // names are case sensitive
    CHECK_FALSE(plug.RestorePreset("Clea"));
    CHECK_FALSE(plug.RestorePreset(""));
    CHECK_FALSE(plug.RestorePreset(static_cast<const char*>(nullptr)));

    CHECK(plug.HasValues(0.5, 1, false, 0));
    CHECK(plug.GetCurrentPresetIdx() == 0);
    CHECK(plug.mNumRestoreStateCalls == 0);
  }
}

TEST_CASE("Presets: the plug-in can tell the host that it changed preset", "[integration][presets]")
{
  PresetPlug plug;
  plug.MakeFactoryPresets();

  CHECK(plug.mNumPresetChangeNotifications == 0);

  // What a preset menu in the UI does
  REQUIRE(plug.RestorePreset(2));
  plug.InformHostOfPresetChange();

  CHECK(plug.mNumPresetChangeNotifications == 1);
  CHECK(plug.GetCurrentPresetIdx() == 2);

  // The individual parameters are not reported as automation
  CHECK(plug.mParamEvents.empty());
}

TEST_CASE("Presets: restoring an unused preset initialises it with the current state", "[integration][presets]")
{
  PresetPlug plug;
  plug.MakeFactoryPresets();
  plug.SetValues(3.25, 5, true, 2);

  REQUIRE_FALSE(plug.GetPreset(3)->mInitialized);
  REQUIRE(plug.RestorePreset(3));

  CHECK(plug.GetPreset(3)->mInitialized);
  CHECK(plug.GetCurrentPresetIdx() == 3);
  CHECK(std::string(plug.GetPresetName(3)) == std::string(DEFAULT_USER_PRESET_NAME) + " 1");

  // The parameters keep their values, which are now stored in the preset
  CHECK(plug.HasValues(3.25, 5, true, 2));

  REQUIRE(plug.RestorePreset(0));
  CHECK(plug.HasValues(0.5, 1, false, 0));
  REQUIRE(plug.RestorePreset(3));
  CHECK(plug.HasValues(3.25, 5, true, 2));
}

TEST_CASE("Presets: MakePresetFromNamedParams() sets the named parameters only", "[integration][presets]")
{
  PresetPlug plug;

  plug.MakePresetFromNamedParams("Named", 2, PresetPlug::kShape, 2, PresetPlug::kDrive, 7.5);
  plug.MakePresetFromNamedParams("All Named", 4, PresetPlug::kOn, true, PresetPlug::kVoices, 3, PresetPlug::kDrive, 0.25, PresetPlug::kShape, 1);
  plug.MakePresetFromNamedParams("None Named", 0);

  CHECK(std::string(plug.GetPresetName(0)) == "Named");
  CHECK(std::string(plug.GetPresetName(1)) == "All Named");
  CHECK(std::string(plug.GetPresetName(2)) == "None Named");

  // Change everything, so that restoring has something to do
  plug.SetValues(10., 8, true, 1);

  REQUIRE(plug.RestorePreset("Named"));
  CHECK(plug.HasValues(7.5, 4, false, 2)); // Voices and On have their defaults

  REQUIRE(plug.RestorePreset("All Named"));
  CHECK(plug.HasValues(0.25, 3, true, 1));

  REQUIRE(plug.RestorePreset("None Named"));
  CHECK(plug.HasValues(1., 4, false, 0));
}

// BUG: IPluginBase::MakePresetFromNamedParams() (IPlug/IPlugPluginBase.cpp) is documented as "Parameters that are not set
// will be defaulted", but fills them in with GetParam(i)->Value(), the *current* value, rather than the default. The
// two only coincide when it is called before any parameter has been changed (i.e. from the constructor).
TEST_CASE("Presets: MakePresetFromNamedParams() uses defaults for unnamed parameters even after values changed", "[.][bug]")
{
  PresetPlug plug;
  plug.SetValues(10., 8, true, 1);

  plug.MakePresetFromNamedParams("Named", 1, PresetPlug::kShape, 2);

  REQUIRE(plug.RestorePreset("Named"));
  CHECK(plug.HasValues(1., 4, false, 2));
}

TEST_CASE("Presets: MakeDefaultPreset() fills free presets with the current state", "[integration][presets]")
{
  PresetPlug plug;
  plug.MakePreset("First", 2., 2, true, 2);

  SECTION("one preset with the default name")
  {
    plug.MakeDefaultPreset();

    CHECK(std::string(plug.GetPresetName(1)) == "Empty");
    CHECK(plug.GetPreset(1)->mInitialized);
    CHECK_FALSE(plug.GetPreset(2)->mInitialized);
  }

  SECTION("several named presets")
  {
    plug.MakeDefaultPreset("Init", 2);

    CHECK(std::string(plug.GetPresetName(0)) == "First");
    CHECK(std::string(plug.GetPresetName(1)) == "Init");
    CHECK(std::string(plug.GetPresetName(2)) == "Init");
    CHECK_FALSE(plug.GetPreset(3)->mInitialized);

    REQUIRE(plug.RestorePreset(0));
    CHECK(plug.HasValues(2., 2, true, 2));
    REQUIRE(plug.RestorePreset(2));
    CHECK(plug.HasValues(1., 4, false, 0));
  }

  SECTION("asking for more than there is room for fills what is left")
  {
    plug.MakeDefaultPreset("Init", 100);

    CHECK(plug.NPresets() == 4);

    for (int i = 1; i < 4; i++)
    {
      CHECK(std::string(plug.GetPresetName(i)) == "Init");
      CHECK(plug.GetPreset(i)->mInitialized);
    }
  }

  SECTION("EnsureDefaultPreset() initialises every free preset")
  {
    plug.EnsureDefaultPreset();

    CHECK(std::string(plug.GetPresetName(0)) == "First");

    for (int i = 1; i < 4; i++)
    {
      CHECK(std::string(plug.GetPresetName(i)) == "Empty");
      CHECK(plug.GetPreset(i)->mInitialized);
    }
  }
}

TEST_CASE("Presets: PruneUninitializedPresets() removes the unused ones", "[integration][presets]")
{
  PresetPlug plug;
  plug.MakePreset("First", 2., 2, true, 2);
  plug.MakePreset("Second", 3., 3, false, 1);

  plug.PruneUninitializedPresets();

  CHECK(plug.NPresets() == 2);
  CHECK(std::string(plug.GetPresetName(0)) == "First");
  CHECK(std::string(plug.GetPresetName(1)) == "Second");

  REQUIRE(plug.RestorePreset(1));
  CHECK(plug.HasValues(3., 3, false, 1));

  SECTION("nothing to prune")
  {
    plug.PruneUninitializedPresets();
    CHECK(plug.NPresets() == 2);
  }
}

TEST_CASE("Presets: ModifyCurrentPreset() stores the current state in the current preset", "[integration][presets]")
{
  PresetPlug plug;
  plug.MakeFactoryPresets();
  REQUIRE(plug.RestorePreset(1));

  plug.SetValues(6.5, 7, false, 2);

  SECTION("keeping the name")
  {
    plug.ModifyCurrentPreset();
    CHECK(std::string(plug.GetPresetName(1)) == "Crunch");
  }

  SECTION("with a new name")
  {
    plug.ModifyCurrentPreset("Crunchier");
    CHECK(std::string(plug.GetPresetName(1)) == "Crunchier");
    CHECK(std::string(plug.GetPresetName(0)) == "Clean");
    CHECK(std::string(plug.GetPresetName(2)) == "Lead");
  }

  // The other presets are unaffected, and the modified one comes back with its new values
  REQUIRE(plug.RestorePreset(0));
  CHECK(plug.HasValues(0.5, 1, false, 0));
  REQUIRE(plug.RestorePreset(2));
  CHECK(plug.HasValues(9.75, 8, true, 2));
  REQUIRE(plug.RestorePreset(1));
  CHECK(plug.HasValues(6.5, 7, false, 2));
}

TEST_CASE("Presets: SetCurrentPresetIdx() selects a preset without restoring it", "[integration][presets]")
{
  PresetPlug plug;
  plug.MakeFactoryPresets();

  plug.SetCurrentPresetIdx(2);

  CHECK(plug.GetCurrentPresetIdx() == 2);
  CHECK(plug.HasValues(1., 4, false, 0));
  CHECK(plug.mNumRestoreStateCalls == 0);
}

TEST_CASE("Presets: CopyPreset() copies a preset's data into another slot", "[integration][presets]")
{
  PresetPlug plug;
  plug.MakeFactoryPresets();

  plug.CopyPreset(plug.GetPreset(2), 3);

  CHECK(plug.GetPreset(3)->mInitialized);
  CHECK(plug.GetPreset(3)->mChunk.IsEqual(plug.GetPreset(2)->mChunk));

  REQUIRE(plug.RestorePreset(3));
  CHECK(plug.HasValues(9.75, 8, true, 2));

  SECTION("over an existing preset")
  {
    plug.CopyPreset(plug.GetPreset(0), 1);
    REQUIRE(plug.RestorePreset(1));
    CHECK(plug.HasValues(0.5, 1, false, 0));
  }
}

TEST_CASE("Presets: presets made from a chunk of state", "[integration][presets]")
{
  // The state of a plug-in with custom data, to be used as a preset in another instance
  CustomStatePlug source;
  source.GetParam(CustomStatePlug::kParamA)->Set(8.);
  source.mText = "from a chunk";
  source.mCounter = 77;
  source.mTable[2] = 0.25;

  IByteChunk state;
  REQUIRE(source.SerializeState(state));

  CustomStatePlug plug;
  REQUIRE(plug.NPresets() == 1);

  SECTION("MakePresetFromChunk()")
  {
    plug.MakePresetFromChunk("Chunky", state);

    CHECK(std::string(plug.GetPresetName(0)) == "Chunky");
    CHECK(plug.GetPreset(0)->mChunk.IsEqual(state));
  }

  SECTION("MakePresetFromBlob() decodes base64")
  {
    std::vector<char> blob(static_cast<size_t>(state.Size()) * 2 + 8, '\0');
    wdl_base64encode(state.GetData(), blob.data(), state.Size());

    plug.MakePresetFromBlob("Blobby", blob.data(), state.Size());

    CHECK(std::string(plug.GetPresetName(0)) == "Blobby");
    CHECK(plug.GetPreset(0)->mChunk.IsEqual(state));
  }

  // Restoring the preset goes through the plug-in's own UnserializeState()
  REQUIRE(plug.RestorePreset(0));
  CHECK(plug.GetParam(CustomStatePlug::kParamA)->Value() == 8.);
  CHECK(plug.mText == "from a chunk");
  CHECK(plug.mCounter == 77);
  CHECK(plug.mTable[2] == 0.25);
}

TEST_CASE("Presets: custom state is part of presets", "[integration][presets]")
{
  CustomStatePlug plug;

  // The only preset is unused: restoring it captures the current state, custom data included
  plug.mText = "first";
  plug.mCounter = 1;
  REQUIRE(plug.RestorePreset(0));

  plug.mText = "changed";
  plug.mCounter = 2;
  plug.GetParam(CustomStatePlug::kParamB)->Set(9.);

  REQUIRE(plug.RestorePreset(0));
  CHECK(plug.mText == "first");
  CHECK(plug.mCounter == 1);
  CHECK(plug.GetParam(CustomStatePlug::kParamB)->Value() == 2.);

  plug.mText = "modified";
  plug.ModifyCurrentPreset();
  plug.mText = "scratch";
  REQUIRE(plug.RestorePreset(0));
  CHECK(plug.mText == "modified");
}

TEST_CASE("Presets: the bank round trips through SerializePresets()/UnserializePresets()", "[integration][presets]")
{
  PresetPlug source;
  source.MakeFactoryPresets(); // three presets and one unused slot
  REQUIRE(source.RestorePreset(1));

  IByteChunk bank;
  REQUIRE(source.SerializePresets(bank));
  CHECK(bank.Size() > 0);

  PresetPlug dest;
  dest.MakePreset("Will Be Replaced", 1., 1, false, 0);
  dest.SetCurrentPresetIdx(2);

  CHECK(dest.UnserializePresets(bank, 0) == bank.Size());

  SECTION("names and initialised flags")
  {
    CHECK(dest.NPresets() == 4);
    CHECK(std::string(dest.GetPresetName(0)) == "Clean");
    CHECK(std::string(dest.GetPresetName(1)) == "Crunch");
    CHECK(std::string(dest.GetPresetName(2)) == "Lead");
    CHECK(dest.GetPreset(0)->mInitialized);
    CHECK(dest.GetPreset(1)->mInitialized);
    CHECK(dest.GetPreset(2)->mInitialized);
  }

  SECTION("the preset which was current in the destination is restored")
  {
    CHECK(dest.GetCurrentPresetIdx() == 2);
    CHECK(dest.HasValues(9.75, 8, true, 2));
  }

  SECTION("every preset has the source's values")
  {
    REQUIRE(dest.RestorePreset(0));
    CHECK(dest.HasValues(0.5, 1, false, 0));
    REQUIRE(dest.RestorePreset(1));
    CHECK(dest.HasValues(5., 2, true, 1));
    REQUIRE(dest.RestorePreset(2));
    CHECK(dest.HasValues(9.75, 8, true, 2));
  }

  SECTION("serializing the destination's bank gives the same data")
  {
    IByteChunk resaved;
    REQUIRE(dest.SerializePresets(resaved));
    CHECK(resaved.IsEqual(bank));
  }

  SECTION("a bank within a larger chunk")
  {
    IByteChunk container;
    const int header = 42;
    container.Put(&header);
    const int startPos = container.Size();
    container.PutChunk(&bank);
    const int endPos = container.Size();
    container.Put(&header);

    PresetPlug other;
    CHECK(other.UnserializePresets(container, startPos) == endPos);
    CHECK(std::string(other.GetPresetName(2)) == "Lead");
  }
}

TEST_CASE("Presets: a truncated bank fails", "[integration][presets][robustness]")
{
  PresetPlug source;
  source.MakeFactoryPresets();
  source.MakeDefaultPreset("Last");

  IByteChunk bank;
  REQUIRE(source.SerializePresets(bank));

  // Cut the bank short inside the data of the last preset: everything before it is intact
  for (int nMissing = 1; nMissing <= PresetPlug::kNumParams * static_cast<int>(sizeof(double)); nMissing++)
  {
    INFO(nMissing << " bytes missing");

    IByteChunk truncated;
    truncated.PutBytes(bank.GetData(), bank.Size() - nMissing);

    PresetPlug dest;
    CHECK(dest.UnserializePresets(truncated, 0) < 0);
    CHECK(std::string(dest.GetPresetName(0)) == "Clean");
    CHECK(std::string(dest.GetPresetName(2)) == "Lead");
  }

  SECTION("an empty bank")
  {
    IByteChunk empty;
    PresetPlug dest;
    CHECK(dest.UnserializePresets(empty, 0) < 0);
  }
}

// BUG: IPluginBase::UnserializePresets() (IPlug/IPlugPluginBase.cpp) copies each preset name it reads from the chunk into
// IPreset::mName, a char[MAX_PRESET_NAME_LEN], with strcpy(). A name longer than that in the incoming data (a corrupt
// or malicious bank) overflows the buffer. The same unchecked strcpy() is used by MakePreset(), MakePresetFromChunk(),
// MakeDefaultPreset() and ModifyCurrentPreset() for names given by the plug-in. Run this under AddressSanitizer.
TEST_CASE("Presets: an over-long preset name in a bank does not overflow the name buffer", "[.][bug]")
{
  const std::string longName(static_cast<size_t>(MAX_PRESET_NAME_LEN) * 4, 'x');

  IByteChunk bank;
  bank.PutStr(longName.c_str());
  const bool initialized = false;
  bank.Put(&initialized);

  PresetPlug dest {1};
  dest.UnserializePresets(bank, 0);

  CHECK(std::string(dest.GetPresetName(0)).size() < static_cast<size_t>(MAX_PRESET_NAME_LEN));
}

// BUG: IPluginBase::RestorePreset(int) (IPlug/IPlugPluginBase.cpp) treats `UnserializeState(...) > 0` as success. A plug-in
// with no parameters and no custom state has an empty state, for which a successful UnserializeState() returns 0, so
// restoring an initialised preset reports failure (and the current preset index is not updated).
TEST_CASE("Presets: restoring a preset of a plug-in with an empty state succeeds", "[.][bug]")
{
  RecordingPlug plug {InstanceInfo{}, []() {
    TestConfig config;
    config.nPresets = 2;
    return config;
  }()};

  plug.MakeDefaultPreset("One", 2);

  CHECK(plug.RestorePreset(1));
  CHECK(plug.GetCurrentPresetIdx() == 1);
}
