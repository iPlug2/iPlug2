#include <catch2/catch_test_macros.hpp>
#include "IPlugTestAPI.h"
#include "AudioBuffers.h"
using namespace iplug;
namespace {
class GainPlug : public Plugin
{
public:
  GainPlug(const InstanceInfo& info) : Plugin(info, [](){ TestConfig c; c.nParams = 1; return c.Make(); }())
  {
    GetParam(0)->InitDouble("Gain", 100., 0., 100., 0.01, "%");
  }
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override
  {
    const double g = GetParam(0)->Value() / 100.;
    for (int c = 0; c < NOutChansConnected(); c++)
      for (int s = 0; s < nFrames; s++)
        outputs[c][s] = inputs[c][s] * g;
  }
};
}
TEST_CASE("Plugin processes audio", "[integration][lifecycle]")
{
  GainPlug plug {InstanceInfo{}};
  plug.HostActivate(48000., 64);
  plug.HostSetParameterNormalized(0, 0.5);
  AudioBuffers<float> in {2, 64, 1.f}, out {2, 64};
  plug.HostProcess(in.Get(), out.Get(), 64);
  REQUIRE(out[1][63] == 0.5f);
  plug.HostIdle();
}
