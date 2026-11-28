# IPlugFaustDSP

A stereo filter/drive effect implemented in [Faust](https://faust.grame.fr), with a UI that is generated from the Faust code by `IGraphicsFaustUI`.

- In **Debug** builds the DSP is compiled at runtime with the libfaust **interpreter** backend. Edit and save `IPlugFaustDSP.dsp` while the plug-in is running and it is recompiled: the parameters and the UI are rebuilt. If there is a compile error, the previous DSP keeps running and the error is shown at the bottom of the UI.
- In **Release** builds the faust compiler generates C++ from `IPlugFaustDSP.dsp` at build time, using the `IPlug/Extras/Faust/IPlugFaust_arch.cpp` architecture file. There are no runtime dependencies on libfaust.

- With `-DIPLUGFAUSTDSP_SHIP_INTERPRETER=ON`, all builds use the interpreter, and the .dsp file and the Faust libraries are copied into the bundle (`iplug_add_faust(... BUNDLE)`), so the DSP is compiled when the plug-in loads, also on other computers and on iOS/visionOS. The original .dsp file is used if it exists, so it can still be edited during development. Bundling is only done on Apple platforms: on Windows and Linux, install the .dsp file and the Faust libraries with your plug-in and pass their location to `FaustGen::SetBundleResourcePath()`.

This example is CMake only.

## Requirements

Faust is not bundled with iPlug2. The interpreter needs libfaust, its headers and the Faust libraries, the compiled code (Release) needs the `faust` executable. Faust 2.85.5 or later is required. The example is skipped if Faust is not found, or is too old.

- Run `Dependencies/download-faust.sh` (in Git Bash on Windows) to get a static libfaust with the matching Faust libraries, built by [faust-builder](https://github.com/iPlug2/faust-builder): an xcframework for macOS, iOS and visionOS, libraries for x64, ARM64 and ARM64EC (MSVC, static runtime) with `faust.exe` on Windows, or `libfaust.a` for x86_64 and aarch64 with the `faust` executable on Linux. This is required for the interpreter on iOS/visionOS, and for shipping it.
- On macOS, install Faust for the `faust` executable, e.g. `brew install faust`, or with the installers from the [Faust releases](https://github.com/grame-cncm/faust/releases).
- An installed Faust can be used instead of the prebuilt libfaust, e.g. `sudo pacman -S faust` on Arch. On Linux that is usually a shared libfaust (`libfaust.so`), which Debug builds of the plug-in load from where it was found at build time. The Faust packages of Ubuntu 24.04 and earlier are too old.
- If CMake doesn't find Faust, set `FAUST_DIR` to the installation prefix (the folder that contains `bin/faust`, `include/faust`, `lib` and `share/faust`).

```bash
# Windows
cmake -G "Visual Studio 17 2022" -A x64 -B build-faust -S Examples/IPlugFaustDSP
cmake --build build-faust --config Debug --target IPlugFaustDSP-app

# macOS, Linux
cmake -G Ninja -B build-faust -S Examples/IPlugFaustDSP -DCMAKE_BUILD_TYPE=Debug
cmake --build build-faust --target IPlugFaustDSP-app

# iOS simulator, shipping the interpreter
cmake -G Xcode -B build-faust-ios -S Examples/IPlugFaustDSP -DCMAKE_SYSTEM_NAME=iOS -DIPLUG2_IOS_DEPLOYMENT_TARGET=15 -DIPLUGFAUSTDSP_SHIP_INTERPRETER=ON
cmake --build build-faust-ios --config Debug --target IPlugFaustDSP-ios-app -- -sdk iphonesimulator
```

## How it works

- `iplug_add_faust()` (in `Scripts/cmake/Faust.cmake`) links the Faust support code, defines `FAUST_DSP_FILE_FaustEffect` (the path of the .dsp file) and, for compiled configurations, generates `FaustEffect.hpp` and defines `FAUST_COMPILED`.
- `FAUST_BLOCK(FaustEffect, mFaust, FAUST_DSP_FILE_FaustEffect, 0)` declares `mFaust` as an `iplug::FaustGen` (interpreter) or a `Faust_FaustEffect` (compiled). Both implement `iplug::IPlugFaust`.
- `IPlugFaust::CreateIPlugParameters()` initializes the plug-in's parameters from the Faust widgets. A plug-in can't change its number of parameters, so the example allocates some spare ones, for when the DSP gains parameters while it is being edited.
- After a recompile, the host is told to re-read the parameters (`InformHostOfParameterDetailsChange()`), so names, groups and ranges update in VST3, CLAP and AUv2 hosts. CLAP hosts restart the plug-in to pick up new ranges. Automation and saved state refer to parameter indices, so adding or reordering widgets in the .dsp file moves them onto different parameters.
- `IGraphicsFaustUI` creates IGraphics controls for the Faust UI: boxes become panels or tabs, sliders become sliders or knobs (`[style:knob]`), menus/radio buttons (`[style:menu{...}]`, `[style:radio{...}]`) become menu/radio buttons and bargraphs become meters, LEDs (`[style:led]`) or value displays (`[style:numerical]`). The control creation and sizing functions can be replaced.
- MIDI is routed to the DSP with `IPlugFaust::ProcessMidiMsg()`. Polyphony is enabled with `declare options "[midi:on][nvoices:8]";` in the .dsp file.

Security: when the interpreter is shipped, the plug-in compiles whatever is in the .dsp file it loads, and whatever that imports from the library paths. Only load DSP code from locations users can't write to by accident (e.g. the bundle), unless loading user code is the point.

Limitations: soundfiles are not supported (a DSP that uses them outputs silence), and the interpreter is much slower than compiled code.
