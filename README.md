# iPlug 2
### C++ audio plug-in framework for desktop, mobile (iOS) and web

[![Build Status](https://dev.azure.com/iplug2/iPlug2/_apis/build/status/iPlug2?branchName=master)](https://dev.azure.com/iplug2/iPlug2/_build/latest?definitionId=2&branchName=master)

iPlug 2 is a simple-to-use C++ framework for developing cross-platform audio plug-ins/apps and targeting multiple plug-in APIs with the same minimalistic code. It abstracts an audio plug-in (IPlug) and its drawing engine/GUI toolkit (IGraphics). IGraphics is a simple graphics abstraction layer with good performance which contains a collection of common controls well suited for audio plug-ins, either using bitmap or vector graphics. IGraphics can use [NanoVG](https://github.com/memononen/nanovg), [Skia](https://skia.org/) and the HTML5 Canvas behind the scenes, providing many options depending on your requirements. Alternatively, iPlug2 can be used with other UI toolkits. [Examples](https://github.com/iPlug2/iPlug2/tree/master/Examples) are included showing how you can use technologies such as HTML/CSS or SwiftUI on top of a C++ DSP layer.

The recommended starting point for an iPlug2 project in 2024 can be found in a separate repo, [iPlug2OOS (out-of-source)](https://github.com/iPlug2/iPlug2OOS)

[iPlug2GPT](https://chat.openai.com/g/g-doomto3Ff-iplug2gpt) is a customized GPT that you can use to learn how to use iPlug2.

iPlug2 API Documentation is published [here](https://iplug2.github.io/docs) and be sure to check out the [iPlug2 Wiki](https://github.com/iPlug2/iPlug2/wiki)

The original version of iPlug was released in 2008 as part of Cockos' WDL library. iPlug 2 (2018) is a substantial reworking that brings multiple vector graphics backends to IGraphics (including GPU accelerated options and HiDPI/scaling), a better approach to concurrency, support for distributed plug-in formats and compiling to WebAssembly via [emscripten](https://github.com/kripken/emscripten), amongst many other things.

iPlug 2 targets the [CLAP](https://github.com/free-audio/clap), VST2, VST3, AUv2, AUv3, AAX (Native) and the [Web Audio Module](https://webaudiomodules.org) (WAM) plug-in APIs. It can also produce standalone win32/macOS apps with audio and MIDI I/O, as well as [Reaper extensions](https://www.reaper.fm/sdk/plugin/plugin.php). Windows 8, macOS 10.13, and iOS 14 are the official minimum target platforms, but depending on the graphics backend used, you may be able to make it work on earlier operating systems.

iPlug 2 is licensed with a liberal zlib-like [license](https://github.com/iPlug2/iPlug2/blob/master/LICENSE.txt), which means that it is free to use in closed source projects and is free from corporate interference.

iPlug2 discussions happen at the [iPlug2 forum](https://iplug2.discourse.group) and on the [iPlug2 discord server](https://discord.gg/7h9HW8N9Ke) - see you there!

We welcome any help with bug fixes, features or documentation.

You can help support the project financially via [github sponsors](https://github.com/sponsors/iplug2). With regular financial support, more time can be spent maintaining and improving the project. Even small contributions are very much appreciated.
