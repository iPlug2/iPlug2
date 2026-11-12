# CLAP latency lifecycle regression

This headless test links the actual iPlug2 CLAP wrapper and drives its exported
CLAP lifecycle callbacks through a minimal host. It needs the CLAP SDK and
clap-helpers headers, but no graphics backend, DAW or audio device.

On Windows or macOS, from the repository root:

```sh
cmake -S Tests/CLAPLatencyTest -B build-latency -DCLAP_SDK_DIR=/path/to/clap -DCLAP_HELPERS_DIR=/path/to/clap-helpers
cmake --build build-latency --config Release
ctest --test-dir build-latency -C Release --output-on-failure
```

The host deliberately omits the optional thread-check extension. It requires
notifications synchronously within activation, a stable active latency value,
coalesced restart requests, silent inactive/deactivation changes, and commitment
of changes made by OnReset during reactivation. This checks CLAP protocol
behavior; it does not reproduce a particular DAW crash or synchronize a plug-in's
own DSP reconfiguration with the restart.
