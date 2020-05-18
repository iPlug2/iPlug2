# iPlug 2
### C++ audio plug-in framework for desktop, mobile (iOS) and web

[![Build Status](https://dev.azure.com/iplug2/iplug2/_apis/build/status/iPlug2?branchName=master)](https://dev.azure.com/iplug2/iplug2/_build/latest?definitionId=2?branchName=master)

iPlug 2 is a simple-to-use C++ framework for developing cross platform audio plug-ins/apps and targeting multiple plug-in APIs with the same minimalistic code. It abstracts an audio plug-in (IPlug) and it's drawing engine/GUI toolkit (IGraphics), although IPlug can be used in bring-your-own GUI library mode without IGraphics. IGraphics contains a collection of common controls well suited for audio plug-in GUIs, either using bitmap or vector graphics. 

The original version of iPlug was released in 2008 as part of Cockos' WDL library. iPlug 2 (2018) is a substantial reworking that brings multiple vector graphics backends to IGraphics (including GPU accelerated options and HiDPI/scaling), a better approach to concurrency, support for distributed plug-in formats and compiling to WebAssembly via [emscripten](https://github.com/kripken/emscripten), amongst many other things.

iPlug 2 targets the VST2, VST3, AUv2, AUv3, AAX (Native) and the [Web Audio Module](https://webaudiomodules.org) (WAM) plug-in APIs. It can also produce standalone win32/macOS apps with audio and MIDI I/O, as well as [Reaper extensions](https://www.reaper.fm/sdk/plugin/plugin.php). Windows 8, macOS 10.11, and iOS 13 are the official minimum target platforms, but depending on the graphics backend used, you may be able to make it work on earlier operating systems.

iPlug 2 includes support for [the FAUST programming language](http://faust.grame.fr), and the libfaust JIT compiler. It was the winner of the 2018 FAUST award.

iPlug 2 is licensed with a liberal zlib-like [license](https://github.com/iPlug2/iPlug2/blob/master/LICENSE.txt), which means that it is free to use in closed source projects and is free from corporate interference.

You can discuss iPlug 2 on the [iPlug2 forum](https://iplug2.discourse.group
) or on the [iPlug user's slack channel.](https://join.slack.com/t/iplug-users/shared_invite/enQtNDIyNjk0NDY2ODAwLWE4Zjc1MTk3NWQzMDRlY2YyOTllMWQyMDY2YjRjMjBmYTMwYzBiMTIwNDM0YWY0MmM5NTBmYWJmMjBkYzRkZDc)

We welcome any help with bug fixes, features or documentation. 

**NOTE: We don't yet consider iPlug2 to be "production ready" since the we (the main devs) didn't ship our products with it yet, but it's nearly there.** Read more [here](https://github.com/iPlug2/iPlug2/wiki/04_FAQ)

You can help support the project financially in several ways, preferably via [github sponsors](https://github.com/sponsors/olilarkin) but also [via patreon](https://www.patreon.com/olilarkin) or [paypal donations](https://paypal.me/olilarkin?locale.x=en_GB). With regular financial support, more time can be spent maintaining and improving the project. Even small contributions are very much appreciated.

[![patreon](Documentation/img/become_a_patron_button.png)](https://www.patreon.com/olilarkin) [![paypal](Documentation/img/paypal_donate_button.gif)](https://paypal.me/olilarkin?locale.x=en_GB)[![slack](Documentation/img/join_slack_button.png)](https://join.slack.com/t/iplug-users/shared_invite/enQtNDIyNjk0NDY2ODAwLWE4Zjc1MTk3NWQzMDRlY2YyOTllMWQyMDY2YjRjMjBmYTMwYzBiMTIwNDM0YWY0MmM5NTBmYWJmMjBkYzRkZDc)
