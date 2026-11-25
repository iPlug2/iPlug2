/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests that a plug-in reports the properties it was configured with: names, IDs, version, type, API and host.

#include <cstring>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "TestPlugins.h"

using namespace iplug;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::StartsWith;

namespace
{

class ConfigPlug : public RecordingPlug
{
public:
  ConfigPlug(const TestConfig& config, const InstanceInfo& info = InstanceInfo{})
  : RecordingPlug(info, config)
  {
  }
};

} // namespace

TEST_CASE("Config: names, IDs and version come from the config", "[integration][config]")
{
  TestConfig config;
  config.name = "MyEffect";
  config.version = 0x00020A0F; // 2.10.15
  config.uniqueID = 'Ab12';
  config.mfrID = 'Mfr9';

  ConfigPlug plug {config};

  CHECK(std::string(plug.GetPluginName()) == "MyEffect");
  CHECK(std::string(plug.GetProductName()) == "MyEffect");
  CHECK(std::string(plug.GetMfrName()) == "AcmeInc");
  CHECK(plug.GetUniqueID() == 'Ab12');
  CHECK(plug.GetMfrID() == 'Mfr9');
  CHECK(std::string(plug.GetBundleID()) == "com.AcmeInc.app.TestPlug");
  CHECK(std::string(plug.GetAppGroupID()) == "group.com.AcmeInc");

  CHECK(plug.GetPluginVersion(false) == 0x00020A0F);
  CHECK(plug.GetPluginVersion(true) == 21015);

  // "D" and/or "T" may be appended in debug/tracer builds
  WDL_String versionStr;
  plug.GetPluginVersionStr(versionStr);
  CHECK_THAT(versionStr.Get(), StartsWith("v2.10.15"));
}

TEST_CASE("Config: build info string mentions version, API and architecture", "[integration][config]")
{
  InstanceInfo info;
  info.mAPI = kAPICLAP;
  ConfigPlug plug {TestConfig{}, info};

  CHECK(std::strlen(plug.GetArchStr()) > 0);

  WDL_String str;
  plug.GetBuildInfoStr(str, "Jan  1 2030", "12:34:56");

  CHECK_THAT(str.Get(), StartsWith("v1.2.3"));
  CHECK_THAT(str.Get(), ContainsSubstring("CLAP"));
  CHECK_THAT(str.Get(), ContainsSubstring(plug.GetArchStr()));
  CHECK_THAT(str.Get(), ContainsSubstring("Jan  1 2030"));
  CHECK_THAT(str.Get(), ContainsSubstring("12:34")); // seconds are dropped
  CHECK_THAT(str.Get(), !ContainsSubstring("12:34:56"));
}

TEST_CASE("Config: the plug-in knows which API it is", "[integration][config]")
{
  struct APICase
  {
    EAPI mAPI;
    const char* mStr;
  };

  const APICase cases[] = {
    {kAPIVST2, "VST2"}, {kAPIVST3, "VST3"}, {kAPIAU, "AU"},   {kAPIAUv3, "AUv3"}, {kAPIAAX, "AAX"},
    {kAPIAPP, "APP"},   {kAPIWAM, "WAM"},   {kAPIWEB, "WEB"}, {kAPICLAP, "CLAP"},
  };

  for (const APICase& apiCase : cases)
  {
    INFO("API: " << apiCase.mStr);
    InstanceInfo info;
    info.mAPI = apiCase.mAPI;
    ConfigPlug plug {TestConfig{}, info};

    CHECK(plug.GetAPI() == apiCase.mAPI);
    CHECK(std::string(plug.GetAPIStr()) == apiCase.mStr);
  }
}

TEST_CASE("Config: plug-in type flags", "[integration][config]")
{
  SECTION("effect")
  {
    ConfigPlug plug {TestConfig{}};
    CHECK_FALSE(plug.IsInstrument());
    CHECK_FALSE(plug.IsMidiEffect());
    CHECK(plug.GetAUPluginType() == 'aufx');
  }

  SECTION("effect with MIDI input is an AU music effect")
  {
    TestConfig config;
    config.midiIn = true;
    ConfigPlug plug {config};
    CHECK_FALSE(plug.IsInstrument());
    CHECK(plug.GetAUPluginType() == 'aumf');
  }

  SECTION("instrument")
  {
    TestConfig config;
    config.type = 1;
    config.channelIO = "0-2";
    ConfigPlug plug {config};
    CHECK(plug.IsInstrument());
    CHECK_FALSE(plug.IsMidiEffect());
    CHECK(plug.GetAUPluginType() == 'aumu');
  }

  SECTION("MIDI effect")
  {
    TestConfig config;
    config.type = 2;
    ConfigPlug plug {config};
    CHECK_FALSE(plug.IsInstrument());
    CHECK(plug.IsMidiEffect());
    CHECK(plug.GetAUPluginType() == 'aumi');
  }

  SECTION("generator")
  {
    TestConfig config;
    config.type = 3;
    config.channelIO = "0-2";
    ConfigPlug plug {config};
    CHECK_FALSE(plug.IsInstrument());
    CHECK_FALSE(plug.IsMidiEffect());
    CHECK(plug.GetAUPluginType() == 'augn');
  }
}

TEST_CASE("Config: MIDI, MPE, chunks and UI flags", "[integration][config]")
{
  SECTION("defaults of the test config")
  {
    ConfigPlug plug {TestConfig{}};
    CHECK_FALSE(plug.DoesMIDIIn());
    CHECK_FALSE(plug.DoesMIDIOut());
    CHECK_FALSE(plug.DoesMPE());
    CHECK(plug.DoesStateChunks());
    CHECK_FALSE(plug.HasUI());
    CHECK_FALSE(plug.GetHostResizeEnabled());
  }

  SECTION("each flag is independent")
  {
    TestConfig config;
    config.midiIn = true;
    ConfigPlug midiIn {config};
    CHECK(midiIn.DoesMIDIIn());
    CHECK_FALSE(midiIn.DoesMIDIOut());
    CHECK_FALSE(midiIn.DoesMPE());

    config = TestConfig{};
    config.midiOut = true;
    ConfigPlug midiOut {config};
    CHECK_FALSE(midiOut.DoesMIDIIn());
    CHECK(midiOut.DoesMIDIOut());
    CHECK_FALSE(midiOut.DoesMPE());

    config = TestConfig{};
    config.mpe = true;
    ConfigPlug mpe {config};
    CHECK_FALSE(mpe.DoesMIDIIn());
    CHECK_FALSE(mpe.DoesMIDIOut());
    CHECK(mpe.DoesMPE());

    config = TestConfig{};
    config.chunks = false;
    config.hasUI = true;
    config.hostResize = true;
    ConfigPlug other {config};
    CHECK_FALSE(other.DoesStateChunks());
    CHECK(other.HasUI());
    CHECK(other.GetHostResizeEnabled());
  }
}

TEST_CASE("Config: parameter and preset counts", "[integration][config]")
{
  TestConfig config;
  config.nParams = 7;
  config.nPresets = 5;
  ConfigPlug plug {config};

  CHECK(plug.NParams() == 7);
  CHECK(plug.NPresets() == 5);

  for (int i = 0; i < 7; i++)
    CHECK(plug.GetParam(i) != nullptr);

  CHECK(plug.GetParam(7) == nullptr);
  CHECK(plug.GetParam(-1) == nullptr);

  ConfigPlug noParams {TestConfig{}};
  CHECK(noParams.NParams() == 0);
  CHECK(noParams.NPresets() == 1);
}

TEST_CASE("Config: editor size and constraints", "[integration][config]")
{
  TestConfig config;
  config.width = 640;
  config.height = 480;
  config.minWidth = 320;
  config.maxWidth = 1280;
  config.minHeight = 240;
  config.maxHeight = 960;
  ConfigPlug plug {config};

  CHECK(plug.GetEditorWidth() == 640);
  CHECK(plug.GetEditorHeight() == 480);
  CHECK(plug.GetMinWidth() == 320);
  CHECK(plug.GetMaxWidth() == 1280);
  CHECK(plug.GetMinHeight() == 240);
  CHECK(plug.GetMaxHeight() == 960);
}

TEST_CASE("Config: processor defaults before the host does anything", "[integration][config]")
{
  TestConfig config;
  config.latency = 123;
  ConfigPlug plug {config};

  CHECK(plug.GetSampleRate() == DEFAULT_SAMPLE_RATE);
  CHECK(plug.GetBlockSize() == DEFAULT_BLOCK_SIZE);
  CHECK(plug.GetLatency() == 123);
  CHECK(plug.GetTailSize() == 0);
  CHECK_FALSE(plug.GetTailIsInfinite());
  CHECK_FALSE(plug.GetBypassed());
  CHECK_FALSE(plug.GetRenderingOffline());
  CHECK(plug.GetCurrentPresetIdx() == 0);
}

TEST_CASE("Config: host identification", "[integration][config][host]")
{
  SECTION("the host is unknown until the API class sets it")
  {
    ConfigPlug plug {TestConfig{}};
    CHECK(plug.GetHost() == kHostUninit);
    CHECK(plug.GetHostVersion(false) == 0);
    CHECK(plug.mNumHostIdentifiedCalls == 0);
  }

  SECTION("a known host")
  {
    ConfigPlug plug {TestConfig{}};
    plug.SetHost("REAPER", 0x00070203);

    CHECK(plug.GetHost() == kHostReaper);
    CHECK(plug.mNumHostIdentifiedCalls == 1);
    CHECK(plug.GetHostVersion(false) == 0x00070203);
    CHECK(plug.GetHostVersion(true) == 70203);

    WDL_String str;
    plug.GetHostVersionStr(str);
    CHECK(std::string(str.Get()) == "v7.2.3");

    plug.GetHostStr(str);
    CHECK(std::string(str.Get()) == "reaper");

    plug.GetRawHostStr(str);
    CHECK(std::string(str.Get()) == "REAPER");
  }

  SECTION("an unknown host")
  {
    ConfigPlug plug {TestConfig{}};
    plug.SetHost("Some DAW Nobody Has Heard Of", 0x00010000);

    CHECK(plug.GetHost() == kHostUnknown);
    CHECK(plug.mNumHostIdentifiedCalls == 1);

    WDL_String str;
    plug.GetRawHostStr(str);
    CHECK(std::string(str.Get()) == "Some DAW Nobody Has Heard Of");
  }

  SECTION("host names are matched case-insensitively as substrings")
  {
    struct HostCase
    {
      const char* mName;
      EHost mHost;
    };

    const HostCase cases[] = {
      {"Cubase 13", kHostCubase},
      {"Ableton Live 12 Suite", kHostAbletonLive},
      {"Logic Pro", kHostLogic},
      {"Bitwig Studio", kHostBitwig},
      {"Fruity Wrapper", kHostFL},
      {"PreSonus Studio One", kHostStudioOne},
      {"WaveLab Elements 11", kHostWaveLabElements},
      {"WaveLab Pro", kHostWaveLab},
      {"standalone", kHostStandalone},
      {"vst3plugintesthost", kHostVST3TestHost},
    };

    for (const HostCase& hostCase : cases)
    {
      INFO("Host: " << hostCase.mName);
      ConfigPlug plug {TestConfig{}};
      plug.SetHost(hostCase.mName, 0);
      CHECK(plug.GetHost() == hostCase.mHost);
      CHECK(plug.mNumHostIdentifiedCalls == 1);
    }
  }
}

TEST_CASE("Config: default answers to host queries", "[integration][config][host]")
{
  TestConfig config;
  config.nParams = 4;
  ConfigPlug plug {config};

  SECTION("important parameters are the first ones")
  {
    WDL_TypedBuf<int> results;
    plug.OnHostRequestingImportantParameters(2, results);
    REQUIRE(results.GetSize() == 2);
    CHECK(results.Get()[0] == 0);
    CHECK(results.Get()[1] == 1);
  }

  SECTION("only the zero view configuration is supported")
  {
    CHECK(plug.OnHostRequestingSupportedViewConfiguration(0, 0));
    CHECK_FALSE(plug.OnHostRequestingSupportedViewConfiguration(800, 600));
  }

  SECTION("no about box, help, note names or track info")
  {
    CHECK_FALSE(plug.OnHostRequestingAboutBox());
    CHECK_FALSE(plug.OnHostRequestingProductHelp());

    char noteText[64] = "x";
    CHECK_FALSE(plug.GetMidiNoteText(60, noteText));
    CHECK(noteText[0] == '\0');

    int r = 1, g = 2, b = 3;
    plug.GetTrackColor(r, g, b);
    CHECK(r == 0);
    CHECK(g == 0);
    CHECK(b == 0);
    CHECK(plug.GetTrackIndex() == 0);
    CHECK(plug.GetTrackNamespaceIndex() == 0);
  }
}
