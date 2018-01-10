# WDL-OL/IPlug

## Disclaimer
This documentation is a work-in-progress reference. You'll need to have some understanding of C++ concepts to find your way around. We're working on some simpler tutorials as well. Stay tuned. Any help is welcome!

### Requirements
WDL-OL/IPlug requires a compiler that supports C++11, and is tested with MS Visual Studio 2017 and Xcode 9. It supports Windows XP or higher and macOS 10.7+.

## What is WDL?
IPlug depends on WDL, which is Cockos' open source library of lightweight reusable code for making cross-platform software. WDL makes very little use of the STL or of modern C++ features. The underlying code is a challenge to understand. It is arcane in places, and poorly documented but it generally works very well. It is developed by some great people who make legendary software and it powers what is undoubtably the best DAW - Reaper (at least from a programmer's perspective)! There is a reason Reaper is blazingly fast and lightweight.

### Where do I begin?
See [Getting Started](md_quickstart.html)

### Credits
Some of the bug fixes and extra features in WDL-OL are thanks to, or inspired by the work of other people. Significant contributions over the years have come from [Theo Niessink](https://www.taletn.com), [Justin Frankel](www.askjf.com), [Julijan Nikolic](https://youlean.co/), [Alex Harker](http://www.alexanderjharker.co.uk/) and [Benjamin Klum](https://www.benjamin-klum.com/it/), amongst others. See individual source code files for any extra credits or license information.

WDL-OL/IPlug uses RtAudio/RtMidi by [Gary Scavone](https://www.music.mcgill.ca/~gary/) to provide cross platform audio and MIDI I/O in standalone app builds.

### License

See [License](md_license.html)