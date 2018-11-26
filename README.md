# iPlug 2 - C++ Audio Plug-in Framework

[![Build Status](https://travis-ci.com/iPlug2/iPlug2-private.svg?token=JaxYSCbvzztBNGHczFA5&branch=master)](https://travis-ci.com/iPlug2/iPlug2-private)

iPlug 2 is a simple-to-use C++ framework for developing cross platform audio plug-ins/apps and targeting multiple plug-in APIs with the same minimalistic code. The original version of iPlug was developed by [John Schwartz aka schwa](https://www.cockos.com/team.php) and released in 2008 as part of Cockos' WDL library. iPlug 2 (2018) is a substantial reworking that brings multiple vector graphics backends (including GPU accelerated options and HiDPI/scaling), support for distributed plug-in formats and compiling to WebAssembly, amongst many other things.

iPlug 2 targets the VST2, VST3, AudioUnit and AAX (Native) and the [Web Audio Module](https://webaudiomodules.org) (WAM) plug-in APIs. It can also produce standalone win32/macOS apps with audio and MIDI I/O, and [Reaper](https://reaper.fm) extensions.

iPlug 2 includes support for [the FAUST programming language](http://faust.grame.fr), and the libfaust JIT compiler. It was the winner of the 2018 FAUST award.

The project relies on many amazing open source libraries by 3rd parties.

Discuss iPlug 2 on the [WDL forum](http://forum.cockos.com/forumdisplay.php?f=32
) or on the [iPlug user's slack channel](https://join.slack.com/t/iplug-users/shared_invite/enQtMzA1NzA1NzE0OTY1LWYyODdjNzkyYTk4MDRmYzZjZTI4ZGVkYTIxZTk0OWRiYWE2MTA0ZWVlODM1NjkzNDAyNDFhMDdjNGI4OTY2YTU)

<a href="https://www.patreon.com/bePatron?u=3140614" data-patreon-widget-type="become-patron-button">Become a Patron!</a><script async src="https://c6.patreon.com/becomePatronButton.bundle.js"></script>

![http://faust.grame.fr/news/2018/07/23/Results-Faust-Awards-2018.html](Documentation/faustaward2018.png)