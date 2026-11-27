# IPlugFaustDSP

A stereo filter/drive effect implemented in [Faust](https://faust.grame.fr), with a UI that is generated from the Faust code by `IGraphicsFaustUI`.

- In **Debug** builds the DSP is compiled at runtime with the libfaust **interpreter** backend. Edit and save `IPlugFaustDSP.dsp` while the plug-in is running and it is recompiled: the parameters and the UI are rebuilt. If there is a compile error, the previous DSP keeps running and the error is shown at the bottom of the UI.
- In **Release** builds the faust compiler generates C++ from `IPlugFaustDSP.dsp` at build time, using the `IPlug/Extras/Faust/IPlugFaust_arch.cpp` architecture file. There are no runtime dependencies on libfaust.

This example is CMake only.

## Requirements

Faust is not bundled with iPlug2. Install it, e.g. `brew install faust` on macOS, or with the installers from the [Faust releases](https://github.com/grame-cncm/faust/releases). If CMake doesn't find it, set `FAUST_DIR` to the installation prefix (the folder that contains `bin/faust`, `include/faust`, `lib` and `share/faust`). The example is skipped if Faust is not found.

The interpreter (Debug) needs libfaust and its headers, the compiled code (Release) needs the `faust` executable.

```bash
cmake -G Ninja -B build-faust -S Examples/IPlugFaustDSP -DCMAKE_BUILD_TYPE=Debug
cmake --build build-faust --target IPlugFaustDSP-app
```

## How it works

- `iplug_add_faust()` (in `Scripts/cmake/Faust.cmake`) links the Faust support code, defines `FAUST_DSP_FILE_FaustEffect` (the path of the .dsp file) and, for compiled configurations, generates `FaustEffect.hpp` and defines `FAUST_COMPILED`.
- `FAUST_BLOCK(FaustEffect, mFaust, FAUST_DSP_FILE_FaustEffect, 0)` declares `mFaust` as an `iplug::FaustGen` (interpreter) or a `Faust_FaustEffect` (compiled). Both implement `iplug::IPlugFaust`.
- `IPlugFaust::CreateIPlugParameters()` initializes the plug-in's parameters from the Faust widgets. A plug-in can't change its number of parameters, so the example allocates some spare ones, for when the DSP gains parameters while it is being edited.
- `IGraphicsFaustUI` creates IGraphics controls for the Faust UI: boxes become panels or tabs, sliders become sliders or knobs (`[style:knob]`), menus/radio buttons (`[style:menu{...}]`, `[style:radio{...}]`) become menu/radio buttons and bargraphs become meters, LEDs (`[style:led]`) or value displays (`[style:numerical]`). The control creation and sizing functions can be replaced.
- MIDI is routed to the DSP with `IPlugFaust::ProcessMidiMsg()`. Polyphony is enabled with `declare options "[midi:on][nvoices:8]";` in the .dsp file.

Limitations: soundfiles are not supported (a DSP that uses them outputs silence), and the interpreter is much slower than compiled code.
