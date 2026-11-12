#include "IPlugCLAP.h"
#include "plugin.hxx"
#include "host-proxy.hxx"
#include <iostream>
#include <stdexcept>
#include <cstring>

static void Require(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

struct Host
{
  enum class Phase { Inactive, Activating, Active, Deactivating };
  Phase phase = Phase::Inactive;
  int notifications = 0, restarts = 0, callbacks = 0;
  bool invalidNotification = false;
  clap_host_t host {};
  static Host& Self(const clap_host_t* pHost) { return *static_cast<Host*>(pHost->host_data); }
  Host()
  {
    host.clap_version = CLAP_VERSION;
    host.host_data = this;
    host.name = "Latency contract host";
    host.vendor = "iPlug2 tests";
    host.url = "https://iplug2.github.io";
    host.version = "1.0.0";
    host.get_extension = [](const clap_host_t*, const char* id) -> const void* {
      static const clap_host_latency_t latency {[](const clap_host_t* pHost) {
        auto& h = Self(pHost);
        ++h.notifications;
        h.invalidNotification |= h.phase != Phase::Activating;
      }};
      // Deliberately omit thread-check support: activation notifications
      // must not depend on the host supporting this optional extension.
      return std::strcmp(id, CLAP_EXT_LATENCY) == 0 ? &latency : nullptr;
    };
    host.request_restart = [](const clap_host_t* pHost) { ++Self(pHost).restarts; };
    host.request_process = [](const clap_host_t*) {};
    host.request_callback = [](const clap_host_t* pHost) { ++Self(pHost).callbacks; };
  }
};

static const char* features[] = {"audio-effect", nullptr};
static const clap_plugin_descriptor_t descriptor {CLAP_VERSION, "org.iplug2.latency-test", "Latency test", "iPlug2", "", "", "", "1.0.0", "", features};

class TestPlugin final : public iplug::IPlugCLAP
{
public:
  int resetLatency = -1, deactivateLatency = -1;
  explicit TestPlugin(Host& host)
    : IPlugCLAP({&descriptor, &host.host}, iplug::Config(0, 1, "2-2", "Latency test", "Latency test", "iPlug2", 0x10000, 1, 1, 0,
        false, false, false, false, 0, false, 0, 0, false, 0, 0, 0, 0, "org.iplug2.latency-test", "")) {}
  void OnReset() override { if (resetLatency >= 0) SetLatency(resetLatency); }
  void OnActivate(bool active) override { if (!active && deactivateLatency >= 0) SetLatency(deactivateLatency); }
};

int main() try
{
  Host host;
  auto* plugin = new TestPlugin(host);
  const auto* api = plugin->clapPlugin();
  Require(api->init(api), "init failed");
  auto* latency = static_cast<const clap_plugin_latency_t*>(api->get_extension(api, CLAP_EXT_LATENCY));
  Require(latency != nullptr, "missing latency extension");
  plugin->SetLatency(48);
  Require(host.notifications == 0 && host.restarts == 0, "inactive latency must be silent");
  host.phase = Host::Phase::Activating;
  Require(api->activate(api, 48000, 1, 512), "activate failed");
  host.phase = Host::Phase::Active;
  Require(latency->get(api) == 48 && host.notifications == 1, "activation must publish latency immediately");
  plugin->SetLatency(72);
  plugin->SetLatency(90);
  plugin->SetLatency(90);
  Require(host.restarts == 1, "restart requests must coalesce");
  Require(latency->get(api) == 48, "active latency must stay stable");
  Require(host.notifications == 1, "active change must not notify");
  host.phase = Host::Phase::Deactivating;
  plugin->deactivateLatency = 84;
  api->deactivate(api);
  host.phase = Host::Phase::Inactive;
  Require(host.notifications == 1 && host.restarts == 1, "deactivation must not notify or restart");
  plugin->resetLatency = 72;
  host.phase = Host::Phase::Activating;
  Require(api->activate(api, 44100, 1, 256), "reactivate failed");
  host.phase = Host::Phase::Active;
  Require(latency->get(api) == 72 && host.notifications == 2, "OnReset latency must be committed at activation");
  api->on_main_thread(api);
  Require(!host.invalidNotification && host.notifications == 2, "notification escaped activation");
  plugin->deactivateLatency = -1;
  host.phase = Host::Phase::Deactivating;
  api->deactivate(api);
  host.phase = Host::Phase::Inactive;
  api->destroy(api);
  std::cout << "CLAP latency lifecycle checks passed\n";
  return 0;
}
catch (const std::exception& error)
{
  std::cerr << error.what() << '\n';
  return 1;
}
