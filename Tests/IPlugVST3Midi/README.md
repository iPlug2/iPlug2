# VST3 MIDI input regression

A headless executable using the actual iPlug2/VST3 input code and the SDK's
`EventList` and `ParameterChanges`. No plugin installation or DAW is needed.
Download the VST3 SDK into `Dependencies/IPlug/VST3_SDK` before configuring.

```sh
cmake -S Tests/IPlugVST3Midi -B build-vst3-midi -DCMAKE_BUILD_TYPE=Release
cmake --build build-vst3-midi --config Release
ctest --test-dir build-vst3-midi -C Release --output-on-failure
```

Checks all MIDI velocity words, values just below integer velocity boundaries,
midpoint note velocity/poly pressure, CC and channel-pressure values, all 14-bit
pitch-bend words, and negative bends with an exactly centered normalization.
It also checks channels and sample offsets, editor forwarding, multiple controller
points in one callback, empty queues, and unchanged last-point behavior for
ordinary parameter automation. It does not validate a DAW or GUI interaction.
