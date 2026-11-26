/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the channel I/O string parser and the bus/channel queries a plug-in answers from it.

#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "TestPlugins.h"

using namespace iplug;

namespace
{

/** The result of IPlugProcessor::ParseChannelIOStr(), with the configs flattened into vectors of bus channel counts */
struct ParsedIO
{
  struct IOConfigBuses
  {
    std::vector<int> mIn;
    std::vector<int> mOut;
  };

  int mNConfigs = 0;
  int mTotalNInChans = 0;
  int mTotalNOutChans = 0;
  int mTotalNInBuses = 0;
  int mTotalNOutBuses = 0;
  std::vector<IOConfigBuses> mConfigs;
};

ParsedIO Parse(const char* ioStr)
{
  ParsedIO result;
  WDL_PtrList<IOConfig> configs;

  result.mNConfigs = IPlugProcessor::ParseChannelIOStr(ioStr, configs, result.mTotalNInChans, result.mTotalNOutChans, result.mTotalNInBuses, result.mTotalNOutBuses);

  for (int i = 0; i < configs.GetSize(); i++)
  {
    const IOConfig* pConfig = configs.Get(i);
    ParsedIO::IOConfigBuses buses;

    for (int b = 0; b < pConfig->NBuses(ERoute::kInput); b++)
      buses.mIn.push_back(pConfig->GetBusInfo(ERoute::kInput, b)->NChans());

    for (int b = 0; b < pConfig->NBuses(ERoute::kOutput); b++)
      buses.mOut.push_back(pConfig->GetBusInfo(ERoute::kOutput, b)->NChans());

    result.mConfigs.push_back(buses);
  }

  configs.Empty(true);

  return result;
}

using Buses = std::vector<int>;

class IOPlug : public RecordingPlug
{
public:
  IOPlug(const char* channelIO, int type = 0)
  : RecordingPlug(InstanceInfo{}, [&]() {
    TestConfig config;
    config.channelIO = channelIO;
    config.type = type;
    return config;
  }())
  {
  }
};

} // namespace

TEST_CASE("ChannelIO: parsing single-bus configs", "[integration][channelio]")
{
  SECTION("mono")
  {
    const ParsedIO io = Parse("1-1");
    REQUIRE(io.mNConfigs == 1);
    REQUIRE(io.mConfigs.size() == 1);
    CHECK(io.mConfigs[0].mIn == Buses{1});
    CHECK(io.mConfigs[0].mOut == Buses{1});
    CHECK(io.mTotalNInChans == 1);
    CHECK(io.mTotalNOutChans == 1);
    CHECK(io.mTotalNInBuses == 1);
    CHECK(io.mTotalNOutBuses == 1);
  }

  SECTION("stereo")
  {
    const ParsedIO io = Parse("2-2");
    REQUIRE(io.mNConfigs == 1);
    CHECK(io.mConfigs[0].mIn == Buses{2});
    CHECK(io.mConfigs[0].mOut == Buses{2});
    CHECK(io.mTotalNInChans == 2);
    CHECK(io.mTotalNOutChans == 2);
  }

  SECTION("mono to stereo")
  {
    const ParsedIO io = Parse("1-2");
    REQUIRE(io.mNConfigs == 1);
    CHECK(io.mConfigs[0].mIn == Buses{1});
    CHECK(io.mConfigs[0].mOut == Buses{2});
    CHECK(io.mTotalNInChans == 1);
    CHECK(io.mTotalNOutChans == 2);
  }

  SECTION("multi-digit channel counts")
  {
    const ParsedIO io = Parse("16-12");
    REQUIRE(io.mNConfigs == 1);
    CHECK(io.mConfigs[0].mIn == Buses{16});
    CHECK(io.mConfigs[0].mOut == Buses{12});
    CHECK(io.mTotalNInChans == 16);
    CHECK(io.mTotalNOutChans == 12);
  }
}

TEST_CASE("ChannelIO: parsing configs with no inputs or no outputs", "[integration][channelio]")
{
  SECTION("instrument: no input bus")
  {
    const ParsedIO io = Parse("0-2");
    REQUIRE(io.mNConfigs == 1);
    CHECK(io.mConfigs[0].mIn.empty());
    CHECK(io.mConfigs[0].mOut == Buses{2});
    CHECK(io.mTotalNInChans == 0);
    CHECK(io.mTotalNOutChans == 2);
    CHECK(io.mTotalNInBuses == 0);
    CHECK(io.mTotalNOutBuses == 1);
  }

  SECTION("analyser: no output bus")
  {
    const ParsedIO io = Parse("2-0");
    REQUIRE(io.mNConfigs == 1);
    CHECK(io.mConfigs[0].mIn == Buses{2});
    CHECK(io.mConfigs[0].mOut.empty());
    CHECK(io.mTotalNOutChans == 0);
    CHECK(io.mTotalNOutBuses == 0);
  }

  SECTION("MIDI effect: no audio at all")
  {
    const ParsedIO io = Parse("0-0");
    REQUIRE(io.mNConfigs == 1);
    CHECK(io.mConfigs[0].mIn.empty());
    CHECK(io.mConfigs[0].mOut.empty());
    CHECK(io.mTotalNInChans == 0);
    CHECK(io.mTotalNOutChans == 0);
    CHECK(io.mTotalNInBuses == 0);
    CHECK(io.mTotalNOutBuses == 0);
  }
}

TEST_CASE("ChannelIO: parsing multiple configs", "[integration][channelio]")
{
  SECTION("mono and stereo")
  {
    const ParsedIO io = Parse("1-1 2-2");
    REQUIRE(io.mNConfigs == 2);
    REQUIRE(io.mConfigs.size() == 2);
    CHECK(io.mConfigs[0].mIn == Buses{1});
    CHECK(io.mConfigs[0].mOut == Buses{1});
    CHECK(io.mConfigs[1].mIn == Buses{2});
    CHECK(io.mConfigs[1].mOut == Buses{2});

    // The totals are the maximum over the configs, not the sum
    CHECK(io.mTotalNInChans == 2);
    CHECK(io.mTotalNOutChans == 2);
    CHECK(io.mTotalNInBuses == 1);
    CHECK(io.mTotalNOutBuses == 1);
  }

  SECTION("three configs, the widest one first")
  {
    const ParsedIO io = Parse("2-2 1-2 1-1");
    REQUIRE(io.mNConfigs == 3);
    CHECK(io.mConfigs[0].mIn == Buses{2});
    CHECK(io.mConfigs[1].mIn == Buses{1});
    CHECK(io.mConfigs[1].mOut == Buses{2});
    CHECK(io.mConfigs[2].mOut == Buses{1});
    CHECK(io.mTotalNInChans == 2);
    CHECK(io.mTotalNOutChans == 2);
  }

  SECTION("maxima can come from different configs")
  {
    const ParsedIO io = Parse("4-1 1-6");
    REQUIRE(io.mNConfigs == 2);
    CHECK(io.mTotalNInChans == 4);
    CHECK(io.mTotalNOutChans == 6);
  }
}

TEST_CASE("ChannelIO: parsing multiple buses", "[integration][channelio]")
{
  SECTION("stereo effect with a stereo sidechain")
  {
    const ParsedIO io = Parse("2.2-2");
    REQUIRE(io.mNConfigs == 1);
    CHECK(io.mConfigs[0].mIn == Buses{2, 2});
    CHECK(io.mConfigs[0].mOut == Buses{2});
    CHECK(io.mTotalNInChans == 4);
    CHECK(io.mTotalNOutChans == 2);
    CHECK(io.mTotalNInBuses == 2);
    CHECK(io.mTotalNOutBuses == 1);
  }

  SECTION("mono effect with a mono sidechain")
  {
    const ParsedIO io = Parse("1.1-1");
    REQUIRE(io.mNConfigs == 1);
    CHECK(io.mConfigs[0].mIn == Buses{1, 1});
    CHECK(io.mConfigs[0].mOut == Buses{1});
    CHECK(io.mTotalNInChans == 2);
    CHECK(io.mTotalNInBuses == 2);
  }

  SECTION("multi-output instrument")
  {
    const ParsedIO io = Parse("0-2.2.2");
    REQUIRE(io.mNConfigs == 1);
    CHECK(io.mConfigs[0].mIn.empty());
    CHECK(io.mConfigs[0].mOut == Buses{2, 2, 2});
    CHECK(io.mTotalNInChans == 0);
    CHECK(io.mTotalNOutChans == 6);
    CHECK(io.mTotalNInBuses == 0);
    CHECK(io.mTotalNOutBuses == 3);
  }

  SECTION("buses of different widths")
  {
    const ParsedIO io = Parse("2.1-6.2.1");
    REQUIRE(io.mNConfigs == 1);
    CHECK(io.mConfigs[0].mIn == Buses{2, 1});
    CHECK(io.mConfigs[0].mOut == Buses{6, 2, 1});
    CHECK(io.mTotalNInChans == 3);
    CHECK(io.mTotalNOutChans == 9);
  }

  SECTION("sidechain only in some configs")
  {
    const ParsedIO io = Parse("1-1 2-2 1.1-1 2.2-2");
    REQUIRE(io.mNConfigs == 4);
    CHECK(io.mConfigs[0].mIn == Buses{1});
    CHECK(io.mConfigs[1].mIn == Buses{2});
    CHECK(io.mConfigs[2].mIn == Buses{1, 1});
    CHECK(io.mConfigs[3].mIn == Buses{2, 2});
    CHECK(io.mTotalNInChans == 4);
    CHECK(io.mTotalNOutChans == 2);
    CHECK(io.mTotalNInBuses == 2);
    CHECK(io.mTotalNOutBuses == 1);
  }
}

TEST_CASE("ChannelIO: parsing wildcards", "[integration][channelio]")
{
  WDL_PtrList<IOConfig> configs;
  int totalNInChans = 0, totalNOutChans = 0, totalNInBuses = 0, totalNOutBuses = 0;

  REQUIRE(IPlugProcessor::ParseChannelIOStr("*-*", configs, totalNInChans, totalNOutChans, totalNInBuses, totalNOutBuses) == 1);
  REQUIRE(configs.GetSize() == 1);

  const IOConfig* pConfig = configs.Get(0);
  CHECK(pConfig->NBuses(ERoute::kInput) == 1);
  CHECK(pConfig->NBuses(ERoute::kOutput) == 1);
  CHECK(pConfig->ContainsWildcard(ERoute::kInput));
  CHECK(pConfig->ContainsWildcard(ERoute::kOutput));

  // A wildcard bus is flagged with a negative channel count for the API classes to pick up
  CHECK(pConfig->GetBusInfo(ERoute::kInput, 0)->NChans() < 0);
  CHECK(pConfig->GetBusInfo(ERoute::kOutput, 0)->NChans() < 0);
  CHECK(totalNInBuses == 1);
  CHECK(totalNOutBuses == 1);

  configs.Empty(true);

  SECTION("a wildcard on one side only")
  {
    REQUIRE(IPlugProcessor::ParseChannelIOStr("2-*", configs, totalNInChans, totalNOutChans, totalNInBuses, totalNOutBuses) == 1);
    CHECK_FALSE(configs.Get(0)->ContainsWildcard(ERoute::kInput));
    CHECK(configs.Get(0)->ContainsWildcard(ERoute::kOutput));
    CHECK(totalNInChans == 2);
    configs.Empty(true);
  }

  SECTION("a plug-in configured with wildcards reports them")
  {
    IOPlug plug {"*-*"};
    CHECK(plug.NIOConfigs() == 1);
    CHECK(plug.HasWildcardBus(ERoute::kInput));
    CHECK(plug.HasWildcardBus(ERoute::kOutput));
    CHECK(plug.MaxNChannelsForBus(ERoute::kInput, 0) == -1);
    CHECK(plug.MaxNChannelsForBus(ERoute::kOutput, 0) == -1);
  }
}

TEST_CASE("ChannelIO: IOConfig queries", "[integration][channelio]")
{
  IOPlug plug {"2.1-2.2.2"};

  REQUIRE(plug.NIOConfigs() == 1);
  const IOConfig* pConfig = plug.GetIOConfig(0);
  REQUIRE(pConfig != nullptr);

  CHECK(pConfig->NBuses(ERoute::kInput) == 2);
  CHECK(pConfig->NBuses(ERoute::kOutput) == 3);
  CHECK(pConfig->GetTotalNChannels(ERoute::kInput) == 3);
  CHECK(pConfig->GetTotalNChannels(ERoute::kOutput) == 6);
  CHECK_FALSE(pConfig->ContainsWildcard(ERoute::kInput));
  CHECK_FALSE(pConfig->ContainsWildcard(ERoute::kOutput));

  CHECK(pConfig->GetBusInfo(ERoute::kInput, 1)->NChans() == 1);
  CHECK(pConfig->GetBusInfo(ERoute::kInput, 1)->GetDirection() == ERoute::kInput);
  CHECK(pConfig->GetBusInfo(ERoute::kOutput, 2)->NChans() == 2);
  CHECK(pConfig->GetBusInfo(ERoute::kOutput, 2)->GetDirection() == ERoute::kOutput);

  CHECK(pConfig->NChansOnBusSAFE(ERoute::kInput, 0) == 2);
  CHECK(pConfig->NChansOnBusSAFE(ERoute::kInput, 1) == 1);
  CHECK(pConfig->NChansOnBusSAFE(ERoute::kInput, 2) == 0);
  CHECK(pConfig->NChansOnBusSAFE(ERoute::kInput, -1) == 0);
  CHECK(pConfig->NChansOnBusSAFE(ERoute::kOutput, 99) == 0);

  CHECK(plug.GetIOConfig(1) == nullptr);
  CHECK(plug.GetIOConfig(-1) == nullptr);
}

TEST_CASE("ChannelIO: stereo effect", "[integration][channelio]")
{
  IOPlug plug {"2-2"};

  CHECK(plug.NIOConfigs() == 1);
  CHECK(plug.MaxNChannels(ERoute::kInput) == 2);
  CHECK(plug.MaxNChannels(ERoute::kOutput) == 2);
  CHECK(plug.MaxNBuses(ERoute::kInput) == 1);
  CHECK(plug.MaxNBuses(ERoute::kOutput) == 1);
  CHECK(plug.MaxNChannelsForBus(ERoute::kInput, 0) == 2);
  CHECK(plug.MaxNChannelsForBus(ERoute::kOutput, 0) == 2);
  CHECK_FALSE(plug.HasSidechainInput());
  CHECK_FALSE(plug.HasWildcardBus(ERoute::kInput));
  CHECK_FALSE(plug.HasWildcardBus(ERoute::kOutput));

  CHECK(plug.LegalIO(2, 2));
  CHECK_FALSE(plug.LegalIO(1, 1));
  CHECK_FALSE(plug.LegalIO(1, 2));
  CHECK_FALSE(plug.LegalIO(2, 1));
}

TEST_CASE("ChannelIO: mono/stereo effect with optional sidechain", "[integration][channelio]")
{
  IOPlug plug {"1-1 2-2 2.2-2"};

  CHECK(plug.NIOConfigs() == 3);
  CHECK(plug.MaxNChannels(ERoute::kInput) == 4);
  CHECK(plug.MaxNChannels(ERoute::kOutput) == 2);
  CHECK(plug.HasSidechainInput());

  SECTION("bus counts")
  {
    int configIdx = -1;
    CHECK(plug.MaxNBuses(ERoute::kInput, &configIdx) == 2);
    CHECK(configIdx == 2);
    CHECK(plug.MaxNBuses(ERoute::kOutput) == 1);
  }

  SECTION("maximum channels per bus are taken over all configs")
  {
    CHECK(plug.MaxNChannelsForBus(ERoute::kInput, 0) == 2);
    CHECK(plug.MaxNChannelsForBus(ERoute::kInput, 1) == 2);
    CHECK(plug.MaxNChannelsForBus(ERoute::kOutput, 0) == 2);
  }

  SECTION("looking up a config by bus channel counts")
  {
    std::vector<int> in {1}, out {1};
    CHECK(plug.GetIOConfigWithChanCounts(in, out) == 0);

    in = {2};
    out = {2};
    CHECK(plug.GetIOConfigWithChanCounts(in, out) == 1);

    in = {2, 2};
    out = {2};
    CHECK(plug.GetIOConfigWithChanCounts(in, out) == 2);

    in = {3};
    out = {3};
    CHECK(plug.GetIOConfigWithChanCounts(in, out) == -1);

    in = {1};
    out = {2};
    CHECK(plug.GetIOConfigWithChanCounts(in, out) == -1);

    in = {2, 1};
    out = {2};
    CHECK(plug.GetIOConfigWithChanCounts(in, out) == -1);

    in = {};
    out = {2};
    CHECK(plug.GetIOConfigWithChanCounts(in, out) == -1);

    in = {2, 2};
    out = {2, 2};
    CHECK(plug.GetIOConfigWithChanCounts(in, out) == -1);
  }

  SECTION("legal I/O is checked on total channel counts")
  {
    CHECK(plug.LegalIO(1, 1));
    CHECK(plug.LegalIO(2, 2));
    CHECK(plug.LegalIO(4, 2));
    CHECK_FALSE(plug.LegalIO(1, 2));
    CHECK_FALSE(plug.LegalIO(2, 1));
    CHECK_FALSE(plug.LegalIO(3, 2));
    CHECK_FALSE(plug.LegalIO(4, 4));
    CHECK_FALSE(plug.LegalIO(0, 2));
  }

  SECTION("-1 means don't care")
  {
    CHECK(plug.LegalIO(-1, 1));
    CHECK(plug.LegalIO(-1, 2));
    CHECK_FALSE(plug.LegalIO(-1, 3));
    CHECK(plug.LegalIO(1, -1));
    CHECK(plug.LegalIO(4, -1));
    CHECK_FALSE(plug.LegalIO(3, -1));
    CHECK(plug.LegalIO(-1, -1));
  }
}

TEST_CASE("ChannelIO: multi-output instrument", "[integration][channelio]")
{
  IOPlug plug {"0-2 0-2.2.2", 1};

  CHECK(plug.NIOConfigs() == 2);
  CHECK(plug.MaxNChannels(ERoute::kInput) == 0);
  CHECK(plug.MaxNChannels(ERoute::kOutput) == 6);
  CHECK(plug.MaxNBuses(ERoute::kInput) == 0);
  CHECK_FALSE(plug.HasSidechainInput());

  int configIdx = -1;
  CHECK(plug.MaxNBuses(ERoute::kOutput, &configIdx) == 3);
  CHECK(configIdx == 1);

  CHECK(plug.MaxNChannelsForBus(ERoute::kOutput, 0) == 2);
  CHECK(plug.MaxNChannelsForBus(ERoute::kOutput, 1) == 2);
  CHECK(plug.MaxNChannelsForBus(ERoute::kOutput, 2) == 2);
  CHECK(plug.MaxNChannelsForBus(ERoute::kInput, 0) == 0);

  CHECK(plug.LegalIO(0, 2));
  CHECK(plug.LegalIO(0, 6));
  CHECK_FALSE(plug.LegalIO(0, 4));
  CHECK_FALSE(plug.LegalIO(2, 2));

  std::vector<int> in, out {2, 2, 2};
  CHECK(plug.GetIOConfigWithChanCounts(in, out) == 1);
  out = {2};
  CHECK(plug.GetIOConfigWithChanCounts(in, out) == 0);
  out = {2, 2};
  CHECK(plug.GetIOConfigWithChanCounts(in, out) == -1);
}

TEST_CASE("ChannelIO: MIDI effect without audio", "[integration][channelio]")
{
  IOPlug plug {"0-0", 2};

  CHECK(plug.NIOConfigs() == 1);
  CHECK(plug.MaxNChannels(ERoute::kInput) == 0);
  CHECK(plug.MaxNChannels(ERoute::kOutput) == 0);
  CHECK(plug.MaxNBuses(ERoute::kInput) == 0);
  CHECK(plug.MaxNBuses(ERoute::kOutput) == 0);
  CHECK(plug.MaxNChannelsForBus(ERoute::kInput, 0) == 0);
  CHECK(plug.MaxNChannelsForBus(ERoute::kOutput, 0) == 0);
  CHECK(plug.NInChansConnected() == 0);
  CHECK(plug.NOutChansConnected() == 0);
  CHECK(plug.LegalIO(0, 0));
  CHECK_FALSE(plug.LegalIO(2, 2));
}

TEST_CASE("ChannelIO: default bus names", "[integration][channelio]")
{
  IOPlug plug {"2.2-2"};
  WDL_String str;

  auto busName = [&](ERoute direction, int busIdx, int nBuses) {
    plug.GetBusName(direction, busIdx, nBuses, str);
    return std::string(str.Get());
  };

  CHECK(busName(ERoute::kInput, 0, 1) == "Input");
  CHECK(busName(ERoute::kInput, 0, 2) == "Main");
  CHECK(busName(ERoute::kInput, 1, 2) == "Aux");
  CHECK(busName(ERoute::kInput, 0, 3) == "Input 0");
  CHECK(busName(ERoute::kInput, 2, 3) == "Input 2");
  CHECK(busName(ERoute::kOutput, 0, 1) == "Output");
  CHECK(busName(ERoute::kOutput, 0, 2) == "Output 0");
  CHECK(busName(ERoute::kOutput, 1, 2) == "Output 1");
  CHECK(busName(ERoute::kOutput, 3, 4) == "Output 3");
}

TEST_CASE("ChannelIO: channel labels", "[integration][channelio]")
{
  IOPlug plug {"4-2"};

  SECTION("channels have no label by default")
  {
    for (int i = 0; i < 4; i++)
      CHECK(std::string(plug.HostGetChannelLabel(ERoute::kInput, i)).empty());

    for (int i = 0; i < 2; i++)
      CHECK(std::string(plug.HostGetChannelLabel(ERoute::kOutput, i)).empty());
  }

  SECTION("fixed labels")
  {
    const char* labels[] = {"W", "X", "Y", "Z"};

    for (int i = 0; i < 4; i++)
      plug.SetChannelLabel(ERoute::kInput, i, labels[i]);

    for (int i = 0; i < 4; i++)
      CHECK(std::string(plug.HostGetChannelLabel(ERoute::kInput, i)) == labels[i]);

    CHECK(std::string(plug.HostGetChannelLabel(ERoute::kOutput, 0)).empty());
  }

  SECTION("the channel index can be formatted into the label, one-based by default")
  {
    plug.SetChannelLabel(ERoute::kInput, 0, "in %i");
    plug.SetChannelLabel(ERoute::kInput, 3, "in %i");
    plug.SetChannelLabel(ERoute::kOutput, 1, "out %i", true);

    CHECK(std::string(plug.HostGetChannelLabel(ERoute::kInput, 0)) == "in 1");
    CHECK(std::string(plug.HostGetChannelLabel(ERoute::kInput, 3)) == "in 4");
    CHECK(std::string(plug.HostGetChannelLabel(ERoute::kOutput, 1)) == "out 1");
  }

  SECTION("out of range channel indices are ignored")
  {
    plug.SetChannelLabel(ERoute::kInput, 4, "nope");
    plug.SetChannelLabel(ERoute::kInput, -1, "nope");
    plug.SetChannelLabel(ERoute::kOutput, 2, "nope");

    for (int i = 0; i < 4; i++)
      CHECK(std::string(plug.HostGetChannelLabel(ERoute::kInput, i)).empty());
  }
}

TEST_CASE("ChannelIO: channel connections", "[integration][channelio]")
{
  IOPlug plug {"2.2-2"};

  SECTION("the harness connects everything to begin with")
  {
    CHECK(plug.NInChansConnected() == 4);
    CHECK(plug.NOutChansConnected() == 2);
    CHECK(plug.NChannelsConnected(ERoute::kInput) == 4);
    CHECK(plug.NChannelsConnected(ERoute::kOutput) == 2);
  }

  SECTION("the host connects a subset")
  {
    plug.HostSetChannels(2, 1);

    CHECK(plug.NInChansConnected() == 2);
    CHECK(plug.NOutChansConnected() == 1);
    CHECK(plug.IsChannelConnected(ERoute::kInput, 0));
    CHECK(plug.IsChannelConnected(ERoute::kInput, 1));
    CHECK_FALSE(plug.IsChannelConnected(ERoute::kInput, 2));
    CHECK_FALSE(plug.IsChannelConnected(ERoute::kInput, 3));
    CHECK(plug.IsChannelConnected(ERoute::kOutput, 0));
    CHECK_FALSE(plug.IsChannelConnected(ERoute::kOutput, 1));

    // The maximum counts are unaffected
    CHECK(plug.MaxNChannels(ERoute::kInput) == 4);
    CHECK(plug.MaxNChannels(ERoute::kOutput) == 2);
  }

  SECTION("nothing connected")
  {
    plug.HostSetChannels(0, 0);
    CHECK(plug.NInChansConnected() == 0);
    CHECK(plug.NOutChansConnected() == 0);
  }

  SECTION("channels that don't exist are not connected")
  {
    CHECK_FALSE(plug.IsChannelConnected(ERoute::kInput, 4));
    CHECK_FALSE(plug.IsChannelConnected(ERoute::kOutput, 2));
    CHECK_FALSE(plug.IsChannelConnected(ERoute::kOutput, 100));
  }

  SECTION("asking for more channels than exist connects what there is")
  {
    plug.HostSetChannels(8, 8);
    CHECK(plug.NInChansConnected() == 4);
    CHECK(plug.NOutChansConnected() == 2);
  }
}

TEST_CASE("ChannelIO: LimitToStereoIO() disconnects inputs beyond the first two", "[integration][channelio]")
{
  IOPlug plug {"4-4"};
  plug.LimitToStereoIO();

  CHECK(plug.NInChansConnected() == 2);
  CHECK(plug.IsChannelConnected(ERoute::kInput, 0));
  CHECK(plug.IsChannelConnected(ERoute::kInput, 1));
  CHECK_FALSE(plug.IsChannelConnected(ERoute::kInput, 2));
  CHECK_FALSE(plug.IsChannelConnected(ERoute::kInput, 3));

  SECTION("a stereo plug-in is left alone")
  {
    IOPlug stereo {"2-2"};
    stereo.LimitToStereoIO();
    CHECK(stereo.NInChansConnected() == 2);
    CHECK(stereo.NOutChansConnected() == 2);
  }
}

// BUG: IPlugProcessor::LimitToStereoIO() (IPlug/IPlugProcessor.cpp) disconnects the inputs beyond the first two, but
// passes `true` when it does the same for the outputs, so outputs 3+ are (re)connected instead of disconnected.
TEST_CASE("ChannelIO: LimitToStereoIO() disconnects outputs beyond the first two", "[.][bug]")
{
  IOPlug plug {"4-4"};
  plug.LimitToStereoIO();

  CHECK(plug.NOutChansConnected() == 2);
  CHECK_FALSE(plug.IsChannelConnected(ERoute::kOutput, 2));
  CHECK_FALSE(plug.IsChannelConnected(ERoute::kOutput, 3));
}

// BUG: IPlugProcessor::MaxNBuses() (IPlug/IPlugProcessor.cpp) is documented to report, when several configs have the
// same (maximum) number of buses, "the one with the higher channel count". It actually reports the last such config,
// whatever its channel count.
TEST_CASE("ChannelIO: MaxNBuses() reports the widest of the configs with the most buses", "[.][bug]")
{
  IOPlug plug {"2-2 1-1"};

  int configIdx = -1;
  CHECK(plug.MaxNBuses(ERoute::kInput, &configIdx) == 1);
  CHECK(configIdx == 0);

  configIdx = -1;
  CHECK(plug.MaxNBuses(ERoute::kOutput, &configIdx) == 1);
  CHECK(configIdx == 0);
}
