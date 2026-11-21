# IPlugFaustDSP
A plug-in that uses FAUST to implement its DSP and JIT-compile FAUST code (using libfaust) in debug builds.

iPlug2 FAUST integration was presented at the International Faust Conference (IFC 2018).

You can check the [conference paper](https://github.com/iPlug2/iPlug2/raw/master/Documentation/Papers/IFC2018.pdf) and watch a [video](https://youtu.be/SLHGxBYeID4) of the talk to learn more about how it works.

## NOTES
* JIT compilation will only work on macOS and Windows (iOS doesn't allow JIT compilation)

* iPlug2 "pre-built libraries" zips contains a build of faust for mac and windows designed to work with this project. If you know what you are doing you can use your own build of faust, but warning: it is very complicated on Windows.

* iPlug2 project preprocessor directives to choose the various paths for the faust dependencies are set in the .xcconfig and .props files that reside in the /config folder of a project.

* The preprocessor directive *DSP_FILE* specified in debug builds is a hardcoded path to the FAUST .dsp file. You may need to modifiy that path in order for the JIT compilation to work.

* The preprocessor directive *FAUST_SHARE_PATH* can be set to specify the install paths for faust libraries like stdfaust.lib. By default it looks in the standard faust install path, so if you didn't install faust the compilation will fail.

* The preprocessor directive *FAUST_COMPILED* is specified in release builds to take out the libfaust JIT compiler and just use the FaustCode.hpp file (the generated C++ code). If the JIT compiler is not working for you, you can disable it (with the FAUST_COMPILED macro) and complile the faust .dsp file to C++ using the command line faust executable.

* Since this project watches a FAUST .dsp file on disk, It must be able to access the file via the filesystem. In the case of macOS, if the app is running in the app-sandbox, it's likely that it can't load the .dsp file or the faust library files and will trigger an assertion. In order to disable the app sandbox you can make sure the "Code signing identity" field is empty in the xcode project target build settings.

* On Windows there are currently some issues with the libfaust compiler, so you may have to manually compile the C++ file, for example assuming you have FAUST installed and in your PATH you could compile it like this on the command line:

    ```faust.exe -cn Faust1 -i -a ..\..\IPlug\Extras\Faust\IPlugFaust_arch.cpp -o FaustCode.hpp IPlugFaustDSP.dsp```
