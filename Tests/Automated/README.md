# iPlug2 automated tests

Unit and integration tests for IPlug and IGraphics, written with [Catch2](https://github.com/catchorg/Catch2) and run with CTest.

They need no plug-in SDKs, no graphics libraries, no audio device and no display, so they build in seconds and run the same way on macOS, Windows, Linux and the Web (Emscripten, under node).

## Running

```bash
cmake -S Tests/Automated -B build/tests
cmake --build build/tests --parallel
ctest --test-dir build/tests --output-on-failure
```

With a multi-config generator (Visual Studio, Xcode) add `--config Debug` to the build and `-C Debug` to ctest.

Catch2 is fetched at configure time unless a Catch2 v3 install is found. The tests can also be built as part of the main iPlug2 tree with `-DIPLUG2_BUILD_AUTOMATED_TESTS=ON`.

Useful variations:

```bash
ctest --test-dir build/tests -L unit                 # only unit tests
ctest --test-dir build/tests -L integration          # only integration tests
ctest --test-dir build/tests -R IGraphicsTests       # only IGraphics
build/tests/IPlug/IPlugTests "[param]"               # one area, straight from the test executable
build/tests/IPlug/IPlugTests --list-tags             # what areas exist
emcmake cmake -S Tests/Automated -B build/tests-web  # Web: then build and ctest as above
```

Sanitizers (GCC or Clang):

```bash
SAN="-fsanitize=address,undefined -fno-omit-frame-pointer"
cmake -S Tests/Automated -B build/tests-san -DCMAKE_C_FLAGS="$SAN" -DCMAKE_CXX_FLAGS="$SAN" -DCMAKE_EXE_LINKER_FLAGS="$SAN"
```

## Layout

IPlug and IGraphics are tested separately, each in its own executable:

```
Tests/Automated/
├── IPlug/                  -> IPlugTests (built with NO_IGRAPHICS)
│   ├── Unit/               IParam, IByteChunk, IPlugQueue, IMidiMsg, utilities, ISender, Extras DSP, synth...
│   ├── Integration/        whole plug-in classes driven through a headless host
│   └── Harness/            IPlugTestAPI.h and test plug-ins
└── IGraphics/              -> IGraphicsTests
    ├── Unit/               IRECT, IColor, IMatrix, IPattern, IText, IVStyle, IPopupMenu...
    ├── Integration/        real controls in a headless IGraphics, with and without a plug-in behind them
    └── Harness/            IGraphicsHeadless.h, TestEditorDelegate.h
```

Every source file in `Unit/` and `Integration/` is picked up automatically.

## The harnesses

**`IPlugTestAPI`** (`IPlug/Harness/IPlugTestAPI.h`) is a headless API class. It sits where `IPlugVST3`, `IPlugCLAP`, `IPlugAPP` etc. sit in a real build: a test plug-in derives from `Plugin`, and the test plays the host.

```cpp
MyPlug plug {InstanceInfo{}};
plug.HostActivate(48000., 64);
plug.HostSetParameterNormalized(kGain, 0.5);     // automation
plug.HostSendMidi(noteOn);                       // delivered at the start of the next block
plug.HostProcess(in.Get(), out.Get(), 64);       // float** or double**
plug.HostIdle();                                 // the idle timer: flushes processor -> UI queues
plug.HostSaveState(chunk);
REQUIRE(plug.mParamEvents.size() == 3);          // what the plug-in told the host
```

`TestConfig` builds the `Config` that `MakeConfig()` would otherwise assemble from a `config.h`, so one test file can define several different plug-ins.

**`IGraphicsHeadless`** (`IGraphics/Harness/IGraphicsHeadless.h`) implements the drawing and platform layers of IGraphics in memory. Everything above that line is the real code: the control stack, hit testing, mouse and keyboard dispatch, parameter linking, dirty tracking, layers and resizing. Draw calls are recorded rather than rasterised, text is measured as fixed pitch, and platform services (clipboard, cursor, dialogs, native text entry and pop-up menus) are faked and recorded.

```cpp
TestEditorDelegate dlg {1, 400, 300, [](IGraphics* pGraphics) {
  pGraphics->AttachControl(new IVKnobControl(IRECT(100, 100, 200, 200), 0), kCtrlTagKnob);
}};
auto* pUI = dlg.OpenUI();
pUI->Drag(150, 150, 150, 50);                    // user drags the knob
REQUIRE(dlg.GetParam(0)->Value() > 0.);
dlg.SendParameterValueFromDelegate(0, 0.25, true);
REQUIRE(pUI->RenderFrame());                     // the knob was dirty and got redrawn
```

In `IGraphicsTests` the plug-in base class is an `IGEditorDelegate`, so a test plug-in deriving from `Plugin` can own a headless UI and be driven as host and as user at the same time.

Two things exist only to make this possible: `IPlugAPIBase` declares `IPlugTestAPI` a friend (so the harness can run the idle timer callback and drain the editor queues like the real API classes), and on Linux `Harness/LinuxShim/IGraphicsLinux.h` stands in for the real header, because `IGEditorDelegate` casts its graphics object to `IGraphicsLinux` there.

## Writing tests

- Tag every test case `[unit]` or `[integration]`, plus an area tag: `TEST_CASE("IParam: enum display text", "[unit][param]")`. Test cases without one of the two are not registered with CTest.
- Keep tests deterministic and portable: no sleeps or timing, no file system or network access, no dependence on fonts, locale or pointer size. Guard anything that needs threads with `#ifndef __EMSCRIPTEN__`.
- Compare floating point values with `Catch::Approx` or the `WithinAbs`/`WithinRel` matchers. Results differ slightly between compilers and architectures.
- Framework `assert()`s are live in Debug builds, so a test must not trip one on purpose.
- A test that demonstrates a known framework bug is written for the correct behaviour and tagged only `[.][bug]`, which keeps it out of CTest. Run those with `IPlugTests "[bug]"`.

## What is not covered here

- The real plug-in format wrappers (VST3, CLAP, AU, AAX, WAM). Build an example and use the format validators for those: see the `validate` skill and `Scripts/ci`.
- Real rendering. Pixel output of the NanoVG and Skia backends, fonts and the native window/event code in `IGraphics/Platforms` are exercised by the `IGraphicsTest` project, not here.
- iOS. The code under test is the same portable C++ that runs on macOS, but there is no simulator job.
