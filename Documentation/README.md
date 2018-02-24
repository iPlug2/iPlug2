## WDL-OL/IPlug

###Disclaimer

This documentation is work-in-progress. You'll need to have some understanding of C++ concepts to find your way around. We're working on some simpler tutorials as well. Stay tuned.
<hr>

## About

IPlug is a simple-to-use C++ framework for developing cross-platform audio plugins and targeting multiple plugin APIs with the same minimalistic code. Originally developed by [John Schwartz (schwa)](https://www.cockos.com/team.php) and released in 2008, IPlug has been enhanced by various contributors.

IPlug depends on [Cockos' WDL](https://www.cockos.com/wdl/), and that is why this project is called WDL-OL, although the differences from Cockos' WDL are to do with IPlug and the build system around it.

### Supported platforms
- VST2
- VST3
- AU
- AAX (Native)
- Standalone - Windows/macOS apps (with audio and MIDI I/O)

WDL-OL/IPlug is not a fully blown application framework such as Qt or JUCE, and lacks many of the useful functionality that those frameworks provide. It is designed for making audio plug-ins and experimenting/hacking, saving you from the painful task of supporting many different plug-in APIs on multiple platforms and architectures, and lets you focus on the DSP and the UI/UX (the fun stuff).
<hr>
## Support
Ask your questions in the [WDL Users Forum](http://forum.cockos.com/forumdisplay.php?f=32
)

<a href="https://join.slack.com/t/iplug-users/shared_invite/enQtMzA1NzA1NzE0OTY1LWYyODdjNzkyYTk4MDRmYzZjZTI4ZGVkYTIxZTk0OWRiYWE2MTA0ZWVlODM1NjkzNDAyNDFhMDdjNGI4OTY2YTU" class="slack">
    <i class="icon-slack"></i>
     Join us on <strong>Slack</strong>
</a>

### Requirements
WDL-OL/IPlug requires a compiler that supports C++11, and is tested with MS Visual Studio 2017 and Xcode 9. It supports Windows XP or higher and macOS 10.7+.

### Where do I begin?
See [Getting Started](md_quickstart.html) and have a look at the [Examples](md_examples.html). If you're an experienced developer, [Advanced Documentation](md_advanced.html) might be a better fit for you.

### How do I upgrade an old WDL-OL/IPlug project?
See [How to Upgrade](md_upgrade.html)

### Can I help?
If you'd like to contribute to the project, see [Code Style](md_codingstyle.html) and then head to [GitHub](https://github.com/olilarkin/wdl-ol)
<hr>
## Credits
Some of the bug fixes and extra features in WDL-OL/IPlug are thanks to, or inspired by the work of other people. Significant contributions over the years have come from [Theo Niessink](https://www.taletn.com), [Justin Frankel](www.askjf.com), [Julijan Nikolic](https://youlean.co/), [Alex Harker](http://www.alexanderjharker.co.uk/) and [Benjamin Klum](https://www.benjamin-klum.com/it/), amongst others. See individual source code files for any extra credits or license information.

WDL-OL/IPlug uses RtAudio/RtMidi by [Gary Scavone](https://www.music.mcgill.ca/~gary/) to provide cross platform audio and MIDI I/O in standalone app builds.
<hr>
## License
WDL/OL shares the same liberal license as Cockos WDL. It can be used in a closed source product for free. A credit/thankyou in your product manual or website is appreciated, but not required.

See [License](md_license.html)