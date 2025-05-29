# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with iPlug2.

## Overall instructions

- You should always check the current directory before running a script and after running a script cd back to the root of the iPlug2 repo.
- You can check the iPlug2 wiki in the Documentation/wiki folder for information on specific topics.
- By default you assume that the user is a plugin developer, and that they are using the iPlug2 framework to develop their plugin, probably by cloning an example project. More experienced users might also want to modify the framework itself.
- You should respond in a concise and too the point manner, and avoid being verbose unless asked. Assume that the user is familiar with audio programming and audio plugin development, but not necessarily with the iPlug2 framework.
- If asked about JUCE (a commercial alternative to iPlug2), please refer the user to the [JUCE documentation](https://juce.com/learn/documentation/).


## iPlug2 Overview

iPlug2 is a cross-platform C++ audio plugin framework for developing audio plugins/apps that target multiple APIs with minimal code. It consists of two main components and a well defined folder structure that facilitates the development of multi-format cross-platform plugins:

1. **IPlug** - Core abstraction layer for audio plugins
2. **IGraphics** - Default drawing engine/GUI toolkit with multiple backend options

### Plugin Formats and Platforms

iPlug2 supports these plugin formats:
- CLAP
- VST2/VST3
- Audio Unit (AUv2/AUv3)
- AAX
- Web Audio Module (WAM)
- Standalone apps
- Reaper extensions

iPlug2 also includes support for the Reaper API, which can be used to add Reaper-specific functionality to your plugin (CLAP, VST2, VST3 formats only).

iPlug2 supports these platforms:
- macOS
- iOS (AUv3 and standalone app only)
- Windows
- Web, via WebAssembly (WASM)

Linux support is experimental and on a branch, but is not supported or recommended for new users.

You can also use other technologies for the UI, such as SwiftUI (macOS/iOS only) or web technologies via a platform-native WebView and examples/templates are provided.

## iPlug2 repo structure

```
/iPlug2
├── IPlug/          # Source code for core plugin abstraction layer
├── IGraphics/      # Source code for IGraphics the default iPlug2 UI framework
├── Dependencies/   # Third-party libraries and SDKs, some vendored, some source code must be downloaded via scripts, and some prebuilt libraries can also be downloaded for e.g. the Skia IGraphics backend. Should not be modified.
├── Examples/       # Example projects demonstrating different features (including carefully tailored Xcode and Visual Studio projects, and with per-project scripts)
├── Tests/          # Test projects (Similar projects, but designed for framework developers to test functionality)
├── WDL/            # Cockos' WDL library components (should not be modified)
├── Scripts/        # Build and utility scripts
└── Documentation/  # Framework documentation
```

In the following examples, I will use the 'cd' command to navigate to the relevant folder where scripts etc. should be run, starting from the root of the iPlug2 repo.

## Getting Started (for plugin developers)

### Dependencies

The framework can build standalone apps and AUv2/AUv3 plug-ins without downloading additional SDKs however, it requires third party SDKs to build other plugin formats. 

Use the scripts to download the latest versions of the VST3, CLAP and WAM SDKs. There are also individual scripts for each format.

```bash
cd ./Dependencies/IPlug/
./download-iplug-sdks.sh
```

- The VST2 SDK is no longer available publicly and therefore is not recommended for new users.
- AUv2/AUv3 can only be built on macOS (you can't cross-compile on a Windows machine)
- The AAX_SDK must be downloaded from Avid's website

If the user wants to use the SKIA IGraphics backend, they need to also download the pre-built Skia libs (or advanced users can build them themselves using the scripts in the Dependencies/ folder).

```bash
cd ./Dependencies/
./download-prebuilt-libs.sh # will get the pre-built libs and headers for Windows and macOS, depending on the platform you are building on.
./download-prebuilt-libs.sh ios # will get the pre-built libs and headers for iOS, but only if you are building on macOS.
```

### Cloning and building Examples/Templates

Examples demonstrate different aspects of the framework and serve as templates, which can also be cloned as the start of new projects.

The structure of an iPlug2 project is described in `Documentation/structure.md`

To clone an example:

```bash
cd Examples
./duplicate.py [inputprojectname] [outputprojectname] [manufacturername] (outputpath) # outputpath can be e.g. ../Projects if it exists to separate the new project from the iPlug2 repository examples.
```

e.g.

```bash
./duplicate.py IPlugEffect MyGainPlugin MyManufacturerName
```

This will create a new project in the Examples/ folder called MyPlugin. YOU SHOULD PROMPT THE USER FOR THE PLUGIN NAME AND MANUFACTURER NAME, using "AcmeInc" as manufacturername if not provided,, checking the spelling and making sure it doesn't include spaces or special characters. The duplicate script will also create a new folder in the Examples/ folder called MyPlugin, doing a find and replace of the old name with the new name on all the files, including the file names. After duplicating a project it's good to customise the PLUG_UNIQUE_ID and PLUG_MFR_ID in the config.h file. These need to be unique 4char IDs for the plugin and manufacturer, and are used to identify the plugin in the host, for example, for IPlugEffect they are

```c++
#define PLUG_UNIQUE_ID 'Ipef'
#define PLUG_MFR_ID 'Acme'
```

There are other things that can be updated at this stage such as the copyright date, emails etc. You can prompt the user for these edits. It is important to remember that some settings in config.h can break the build, for instance if the BUNDLE_NAME does not match the plist files resources can fail to load on macOS, so just prompt for very high level metadata and give the option to skip and do it later.

#### To build an example/project (e.g. newly cloned MyGainPlugin) on macOS as a macOS standalone app

```bash
cd Examples/MyGainPlugin
xcodebuild -project "./projects/MyGainPlugin-macOS.xcodeproj" -target APP -configuration Release 2> ./build_errors.log
```

- configuration can be `Debug` or `Release` or `Tracer`
- target can be `CLAP` or `APP` or `VST2` or `VST3` or `AU` or `AAX` or `AUv3App` or `AUv3`. The 'All' target will build all of the above, but might fail if you don't have the dependencies/SDKs for all of them. It's better to focus on one target at a time.
- products will be built into the folders specified in the top level `common-mac.xcconfig` file, for example the App will be installed in `~/Applications/MyGainPlugin.app`. VST3 etc will be installed in the `~/Library/Audio/Plug-Ins/VST3` folder.
- The AUv3 format is a bit special since it requires a separate AUv3App project to be built, which is packages the AUv3 plugin as an App Extension. AUv2 is more simple and should be recommended for new users.

Typically a user might want to actually use Xcode alongside Claude Code to build the project, since it's a bit of a pain to build the project from the command line. If you want to open the project in Xcode, you can do so by running the following command to open the top level workspace:

```bash
cd Examples/MyGainPlugin
open ./MyGainPlugin.xcworkspace # or open  ./projects/MyGainPlugin-macOS.xcodeproj if you don't want to see the iOS projects
```

This will open the project in Xcode. The user can then build the project from Xcode, which has the advantage of being able to use the debugger and other tools.

#### To build an example/project on Windows

Projects can be duplicated in a similar way using git bash or the windows command prompt (with adjusted paths).

The user can open the MyGainPlugin.sln solution file (for example) in Visual Studio and build the desired configuration and project.
```

## Example/Template Projects

There are three core examples:

* **IPlugEffect** : A basic audio effect which is a volume control.
* **IPlugInstrument** : An MPE-capable polyphonic synthesiser.
* **IPlugControls** : A demonstration of the widgets available in the IControls library.

And more specialised examples:

* **IPlugChunks** : Shows how to store data other than just parameter values in the plug-in state and how to get tempo info from the host.
* **IPlugMidiEffect** : A basic MIDI effect plugin. Note: only AudioUnits really have a notion of a midi effect.
* **IPlugSideChain** : Demonstrates how to do a plug-in with two input buses for effects that require sidechain inputs such as compressors/gates.
* **IPlugSurroundEffect** : A  multichannel volume control effect plug-in that should work on different surround buses.
* **IPlugDrumSynth** : A drum synthesiser example with multiple output buses.
* **IPlugResponsiveUI** : An example of how to make a responsive UI that adapts to the platform window that can be maximized and resized using the OS window chrome. 
* **IPlugOSCEditor** : Demonstrates how to use the Open Sound Control classes in iPlug2, as well as the IWebViewControl
* **IPlugReaperExtension** : This is a template project for making a [Reaper Extension](http://reaper.fm/sdk/plugin/plugin.php). No realtime audio processing code, obviously. Making a reaper extension can be painful since it is all based around the Win32 APIs. This abstracts away some of the nastyness.
* **IPlugReaperPlugin** : This is a plugin that calls [Reaper Reascript API](https://www.reaper.fm/sdk/reascript/reascripthelp.html#c) functions. Supports VST2, VST3 and CLAP formats. Also allows embedded UIs.
* **IPlugConvoEngine** : UI-less example of WDL_ConvoEngine that reports a delay to the host for plugin-delay-compensation (PDC)
* **IPlugVisualizer** : Demonstrates visualizing audio signals.

* **IPlugCocoaUI** : An iOS/macOS project using AppKit/UIKit for the user interface 
* **IPlugSwiftUI** : An iOS/macOS only project using SwiftUI for the user interface 
* **IPlugWebUI** : An example showing how UI can written in HTML/CSS/JS, using a platform web view
* **IPlugP5js** : Another platform web view project this time using p5js to render using WebGL
* **IPlugSvelteUI** : An example showing how to use [Svelte](https://svelte.dev/) to build the UI.

## Core Concepts relevant to plugin developers

- For each iPlug2 project there are by default three main source code files that are very important. Using IPlugEffect as an example, these are:
  - `IPlugEffect.cpp` : The main plugin class implementation file.
  - `IPlugEffect.h` : The header file for the plugin class. Normally a list of parameters is defined here.
  - `config.h` : The header file for configuration of the plugin with important plugin identity, capabilities and metadata. This file should not include any other header files.
- In these files specific includes are used to switch base classes depending on the format being built, it is important not to modify this behaviour.
- In the `ProcessBlock` method, all code must be realtime safe, i.e. no allocations, mutex locks, file operations, etc that could block.
- Parameter count is fixed at compile time, and cannot be changed at runtime.
- Parameters are gived a unique integer index, usually in an enum
- Parameters details are declared in the plugin CTOR.
- Parameter values are non-normalized
- Audio samples are typedef'd with the `sample` type and can either be 32 or 64 bit float, they are double precision by default.
- The 'GetParam(kParamIndex)->Value()' method is used to get the current value of a parameter and can be called from the audio thread. For complex logic, OnParamChange can be used to handle parameter changes.

- IGraphics UIs are usually written in a lambda function inside e.g. the plugin CTOR.
- IControls can be typically contrstructed either with a parameter ID, which auto-links them to the parameter.
- For responsive UIs you can configure the lamda to be called on resize, and the function must deal with e.g. tearing down or adjusting controls.
- Controls that depend on bitmap, font or svg resources can be the source of many common issues. The wiki document `06_Load_a_resource.md` explains how to load resources.

## Core Concepts and C++ base classes in the framework (not typically edited by plugin developers)

The best way to understand the iPlug2 class hierarchy is to check the [diagrams in the doxygen documentation](https://iplug2.github.io/docs/inherits.html).

### IPlug (non exhaustive list)

- `IEditorDelegate`: The lowest level base class which mediates between the plugin and the UI. For IGraphics plug-ins we use the IGEditorDelegate.
- `IPlugBase`: Base class that contains plug-in info and state manipulation methods
- `IPlugAPIBase`: Base class for all plugin API abstraction functionality e.g. inherited by 'IPlugCLAP`
- `IPlugProcessor`: Realtime Audio processing functionality
- `IParam` in `IPlugParameter.h`: Parameter class
- `IMidiMsg` in `IPlugMidi.h`: MIDI message abstraction
...

### IGraphics (non exhaustive list)

- `IGraphics`: Base class for an IGraphics context. IGraphics is reponsible for drawing using one of the supported backends, platform event handling and other UI functionality.
  - The folder `IGraphics\Platforms` contains the platform-specific implementations of IGraphics (IGraphicsWin, IGraphicsMac, IGraphicsiOS, IGraphicsWeb).
  - The folder `IGraphics\Drawing` contains the backends that are used to draw the UI (IGraphicsNanoVG, IGraphicsSkia).
- `IControl`: IControl is the base class for UI controls defined in `IGraphics\IControl.h`. IControls are the building blocks of the UI.
  - `IGraphics\IControl.h` contains several base classes and simple IControls. IControl base classes and mix-ins add extra behaviours to an IControl including `IVectorBase` for IV...Controls, `IBitmapBase` for IB...Controls. `IKnobControlBase` provides the common functionality for IVKnobControl and IBKnobControl etc.
  - The folder `IGraphics\Controls` contains the source code for more sophisticated IControls.
  - `IGraphics\Controls\IControls.h` contains IVControls, IBControls and ISVGControls.
  - `IV...Controls` are a collection of IControls that use vector graphics, drawn procedurally via IGraphics, sharing an IVStyle for themeing.
  - `IB...Controls` are a collection of IControls that use raster graphics via bitmap resources, e.g. for photorealistic knobs.
  - `ISVG...Controls` are a collection of IControls that use SVG graphics via SVG resources, e.g. for vector controls designed in e.g. Figma.
- `IGraphicsConstants.h` and `IGraphicsStructs.h` contain the important constants, structs, std::function defs and enums used in the framework.

Built in controls (non exhaustive list) are:

- IVLabelControl - A vector label control that can display text with a shadow.
- IVButtonControl - A vector button/momentary switch control.
- IVSwitchControl - A vector switch control.
- IVToggleControl - A vector toggle control.
- IVSlideSwitchControl - A switch with a slide animation when clicked.
- IVTabSwitchControl - A vector "tab" multi switch control.
- IVRadioButtonControl - A vector "radio buttons" switch control.
- IVKnobControl - A vector knob control drawn using graphics primitives.
- IVSliderControl - A vector slider control.
- IVRangeSliderControl - A vector range slider control, with two handles.
- IVXYPadControl - A vector XY Pad slider control.
- IVPlotControl - A vector plot to display functions and waveforms.
- IVGroupControl - A control to draw a rectangle around a named IControl group.
- IVPanelControl - A panel control which can be styled with emboss etc.
- IVColorSwatchControl - A control to show a color swatch of up to 9 colors.
- ISVGKnobControl - A vector knob/dial control which rotates an SVG image.
- ISVGButtonControl - A vector button/momentary switch control which shows an SVG image.
- ISVGToggleControl - A vector toggle switch control which shows an SVG image.
- ISVGSwitchControl - A vector switch control which shows one of multiple SVG states.
- ISVGSliderControl - A Slider control with and SVG for track and handle.
- IBButtonControl - A bitmap button/momentary switch control.
- IBSwitchControl - A bitmap switch control.
- IBKnobControl - A bitmap knob/dial control that draws a frame from a stacked bitmap.
- IBKnobRotaterControl - A bitmap knob/dial control that rotates an image.
- IBSliderControl - A bitmap slider/fader control.
- IBTextControl - A control to display text using a monospace bitmap font.
- IBMeterControl - A bitmap meter control, that can be used for VUMeters.
- IPanelControl - A basic control to fill a rectangle with a color or gradient.
- ILambdaControl - A control that can be specialised with a lambda function, for quick experiments without making a custom IControl.
- IBitmapControl - A basic control to draw a bitmap, or one frame of a stacked bitmap depending on the current value.
- ISVGControl - A basic control to draw an SVG image to the screen.
- ITextControl - A basic control to display some text.
- IEditableTextControl - A basic control to display some editable text.
- IURLControl - A control to show a clickable URL, that changes color after clicking.
- ITextToggleControl - A control to toggle between two text strings on click.
- ICaptionControl - A control to display the textual representation of a parameter.
- PlaceHolder - A control to use as a placeholder during development.

IGraphics is a simple UI framework for typical audio plugin GUIs that don't require many controls. For more complex GUIs that require tabbed pages, the user can use the `IGraphics\Controls\IVTabbedPagesControl.h` control.

## Code Style Guidelines

- Indentation: 2 spaces (no tabs)
- Unix line endings (except Windows-specific files)
- Use C++11 features like `override`, `final`, and appropriate `auto`
- Avoid STL in Core iPlug2 code (use WDL alternatives when possible)
- Variable naming:
  - Member variables: `mCamelCase`
  - Pointer arguments/variables: `pCamelCase` 
  - Member variables that are pointers: `mPointer` (no "p" prefix)
  - Internal methods not intended for user use: `_methodName`

## Platform-Specific Notes

### macOS
- Minimum supported: macOS 10.13+
- Xcode 14+ recommended

### Windows
- Minimum supported: Windows 8+
- Visual Studio 2022 recommended

### iOS
- Minimum supported: iOS 14+

### Web
- Uses Emscripten for compiling to WebAssembly
- Supports Web Audio Module (WAM) v1 API (not v2)

## Additional Resources

- [iPlug2 API Documentation (Doxygen generated)](https://iplug2.github.io/docs)
- [iPlug2 GPT](https://chatgpt.com/g/g-doomto3Ff-iplug2gpt)
- [iPlug2 Wiki](https://github.com/iPlug2/iPlug2/wiki)
- [iPlug2 Forum](https://iplug2.discourse.group)
- [iPlug2 Discord](https://discord.gg/7h9HW8N9Ke)
