# Examples

\todo This is obsolete, but here to be updated

## Introduction

The Examples folder contains example/template projects to demonstrate how to use different features of my modified IPlug and WDL. They also serve as a testbed to make sure that the various aspects are working. Each folder contains scripts which automate the build process and package everything along with a pdf manual in an installer. The scripts also code sign binaries/installers and set icons where required.

Visual Studio 2019+ and Xcode 10+, which are both available for free are supported. If you know what you're doing you may be able to get things to compile with other IDEs/versions, you are highly recommended to use the latest versions. No effort is made to make the code compatible with other IDEs, or compilers such as mingw. 

Note: although Visual Studio and Xcode required to build IPlug plug-ins, projects are setup in such a way that you may easily use your favourite text editor and call build scripts, to build the binaries (although then you lack the debugger functionality of the IDE).

### Requirements:

Some of these are optional, but without them the build-scripts will need to be edited, otherwise you'll get errors.

**Windows**

* [Microsoft Visual Studio 2019 Community](https://www.visualstudio.com/downloads/) (or one of the premium versions)
\todo Add a list of Visual Studio packages required

* [Python 2.7 or 3](http://www.python.org/) for running various scripts
* [Inno Setup](http://www.jrsoftware.org/isinfo.php) for creating installers
* [7-Zip](http://www.7-zip.org/) (if you want to create a ZIP instead of making an installer)
* PACE tools and certificate for code signing AAX binaries (AAX only, consult Avid documentation)

**Mac**

* Xcode 9 installed, including command-line tools
* [Packages](http://s.sudre.free.fr/Software/Packages/about.html) for building OSX installers 
* [setfileicon utility](http://maxao.free.fr/telechargements/setfileicon.gz) for changing icons
* Mac Developer ID Certificates for code signing installers and dmg files
* PACE tools and certificate for code signing AAX binaries (AAX only, consult Avid documentation)

## Example Projects

* **IPlugChunks** - shows how to use chunks in a plugin. Chunks allow you to store arbitrary data in the plugin's state as opposed to just a value for each parameter.
* **IPlugControls** -  demos the various IControl classes (example by Captain Caveman)
* **IPlugConvoEngine** - demos WDL_ConvolutionEngine() - Cockos' fast convolution engine (example by Tale)
* **IPlugDistortion** - demos Tale's bessel filter implementation for realtime oversampling (example by Tale)
* **IPlugEEL** - demonstrates using Cockos' EEL library for run-time expression evaluation
* **IPlugEffect** - The most basic IPlug plugin, a gain control similar to AGain in the VST2 SDK, however it has a GUI
* **IPlugGUIResize** - has three buttons to choose different gui sizes at runtime
* **IPlugHostDetect** - displays the host name and version (not very reliable at the moment)
* **IPlugMonoSynth** - a basic monophonic IPlug synth, showing how to handle MIDI messages sample accurately. Also shows how to use Tale's IKeyboardControl.
* **IPlugMouseTest** - demonstrates an XY pad IControl which is linked to two plugin parameters
* **IPlugMultiChannel**  - demos a multi-channel IPlug plugin, and how to test if channels are connected
* **IPlugMultiTargets** - a midi effect plugin that also demos compilation to IOS, getting tempo info, and pop up menus
* **IPlugOpenGL** - using OpenGL in IPlug
* **IPlugPlush** -  shows how to use Cockos' Plush to do basic 3D Graphics
* **IPlugPolySynth** - a basic polyphonic IPlug synth
* **IPlugResampler** - demonstrates using WDL_Resampler
* **IPlugSideChain** - a plugin that shows how to setup a sidechain input, for VST3, AU
* **IPlugText** - demos different ways to draw text

The **IPlugEffect** project is the main starting project I use. If you are not interested in AAX/standalone etc, I suggest you [duplicate](md_duplicate.html) this and manually remove those targets to give you a new clean starting template with just your preferred formats in it.

Rather than changing settings for individual targets/projects inside the Xcode Project/Visual Studio solutions, most customisations can be done in the xcconfig and property sheets.

## Supported Formats

### VST2

You need to two files from the Steinberg VST2.4 SDK to the folder VST_SDK, see VST_SDK/readme.txt

### VST3

Extract the Steinberg VST3.X.X SDK to the folder VST3_SDK

- On Windows, make sure *C:\\Program Files\\Common Files\\VST3* exists, otherwise the copy files build stage will cause the build to fail

### AAX

- Extract AAX_SDK_2XXXX.zip to the AAX_SDK folder

- Also modify ExamplePlugIns/Common/Mac/CommonDebugSettings.xcconfig and ExamplePlugIns/Common/Mac/CommonReleaseSettings.xcconfig...

```
GCC_VERSION = com.apple.compilers.llvm.clang.1_0
SDKROOT = macosx10.X
MACOSX_DEPLOYMENT_TARGET = 10.X
ARCHS = x86_64 i386
```

- In order to compile AAX binaries that run in the release build of Pro Tools, you will need to code-sign those binaries (see Avid documentation)

### AU (AudioUnit v2)

- When building AUs, bear in mind that some hosts keep a cache... see debugging notes below.
- There is a shell script *validate_audiounit.command* that will run *auvaltool* with the correct IDs for your plugin (see below) and can also set up the leaks test, which is useful for debugging. Type *man auval* on the command line for documentation. 
- By default I build to the system AU folder /Library/Audio/Plug-Ins/Components/. You will need to have *write permissions* to this folder. If you want to build to the user AU folder, you'll need to edit the .xcconfig file and also modify the installer scripts

### Standalone

- Audio and Midi is provided via RTAudio and RTMidi by Gary Scavone. To build on windows you need to extract some files into ASIO_SDK, see ASIO_SDK/README.md


## Windows Issues

The template projects use static linking with the MSVC2019 runtime libraries (/MT).

If you change your project to dynamic linking, you may need to provide the Visual Studio redistributable in your installer.

\todo This may need to be updated due to recent Microsoft changes

## macOS Issues

Since macOS 10.8 you will need to code-sign your installer and the .app with a valid signature obtained from Apple, to prevent an unidentified developer warning when a user tries to open your installer or dmg file. For the app store you need to add entitlements in order to comply with the sandbox regulations. These things are done by the `makedist-*` build scripts.


### Debugging

`.xcscheme` files are set up to use some common hosts for debugging the various formats in Xcode.

To debug an Audiounit using *auval*, remember to change the auval executable arguments to match plugin's type and IDs:

`aufx/aumf/aumu PLUG_UNIQUE_ID PLUG_MFR_ID`

AU hosts cache information about the plugin I/O channels etc, so I have added a build script that deletes the caches after a build. If this becomes annoying (it will cause Logic to rescan plugins) you can disable it.

There is also a validate_audiounit.command shell script which will is a helper that runs auval with your plugins' unique IDs, and optionally performs the leaks test.

You should install [VSTHost](http://hermannseib.com/english/vsthost.htm) to *C:\\Program Files\\VSTHost\\vsthost.exe* (on x64 you should install the 64bit version)

To debug AAX, you need to install a development build of Pro Tools. Consult Avid documentation for details.

##Installers & one-click build scripts

The example projects contain shell scripts for both Windows (makedist-win.bat) and OSX (makedist-mac.command) that build everything, code-sign (where relevant) and package the products in an installer including license, readme.txt, changelog.txt and manual. On Windows *Inno Setup* is used, on OSX - *Packages*.

A Python script *update_version.py* is called to get the version from `#define PLUG_VER` in *resource.h*. It then updates the info.plist files and installer scripts with the version number (in the format major.minor.bugfix). If you aren't building some components, e.g. AAX, the build scripts may need to be modified.

**Please alter the license and readme text and remove my name from the build scripts if you're releasing a plugin publicly**.

On OSX the script can also code-sign the standalone app and builds a .pkg for the appstore (commented out).