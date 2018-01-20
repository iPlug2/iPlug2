# WDL-OL/IPlug v2
## Cross-platform C++ audio plug-in framework

**Disclaimer:**  
This documentation is work-in-progress. You'll need to have some understanding of C++ concepts to find your way around. We're working on some simpler tutorials as well. Stay tuned.


## About WDL-OL/IPlug

IPlug is a simple-to-use C++ framework for developing cross platform audio plugins and targeting multiple plugin APIs with the same minimalistic code. Originally developed by [John Schwartz aka schwa](https://www.cockos.com/team.php) and released in 2008, IPlug has been enhanced by various contributors. IPlug depends on [Cockos' WDL](https://www.cockos.com/wdl/), and that is why this project is called WDL-OL, although the differences from Cockos' WDL are to do with IPlug and the build system around it.
This version of IPlug targets the VST2, VST3, AudioUnit and AAX (Native) plug-in APIs. It can also produce standalone Windows/macOS apps with audio and MIDI I/O.

WDL-OL/IPlug is not a fully blown application framework such as Qt or JUCE, and lacks many of the useful functionality that those frameworks provide. It is designed for making audio plug-ins and experimenting/hacking, saving you from the painful task of supporting many different plug-in APIs on multiple platforms and architectures, and lets you focus on the DSP and the UI/UX a.k.a the fun stuff.

Discuss IPlug on the [WDL forum](http://forum.cockos.com/forumdisplay.php?f=32
) or on the [Discord server](https://discord.gg/DySxqNH).


### Requirements
WDL-OL/IPlug requires a compiler that supports C++11, and is tested with MS Visual Studio 2017 and Xcode 9. It supports Windows XP or higher and macOS 10.7+.

## About this documentation
### Where do I begin?
See [Getting Started](md_quickstart.html) and check out the [Examples](md_examples.html)

### How do I upgrade an old WDL-OL/IPlug project?
See [How to Upgrade](md_upgrade.html)

### Experienced developers
See [Advanced Documentation](md_advanced.html) and if you would like to contribute to the project [Code Style](md_codingstyle.html)

## Credits
Some of the bug fixes and extra features in WDL-OL/IPlug are thanks to, or inspired by the work of other people. Significant contributions over the years have come from [Theo Niessink](https://www.taletn.com), [Justin Frankel](www.askjf.com), [Julijan Nikolic](https://youlean.co/), [Alex Harker](http://www.alexanderjharker.co.uk/) and [Benjamin Klum](https://www.benjamin-klum.com/it/), amongst others. See individual source code files for any extra credits or license information.

WDL-OL/IPlug uses RtAudio/RtMidi by [Gary Scavone](https://www.music.mcgill.ca/~gary/) to provide cross platform audio and MIDI I/O in standalone app builds.

## License
WDL/OL shares the same liberal license as Cockos WDL. It can be used in a closed source product for free. A credit/thankyou in your product manual or website is appreciated, but not required.

See [License](md_license.html)