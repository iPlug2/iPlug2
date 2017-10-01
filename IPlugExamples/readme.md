WDL-OL IPlug Examples Instructions
www.olilarkin.co.uk

Introduction

This folder contains example/template projects to demonstrate how to use different features of my modified IPlug and WDL. They also serve as a testbed to make sure that the various aspects are working. Each folder contains scripts which automate the build process and package everything along with a pdf manual in an installer. The scripts also code sign binaries/installers and set icons where required.

Projects are provided for Visual Studio 2017+ and Xcode *+. You cannot downgrade the VS2017 projects to work with older versions. There are also codeblocks projects for building VST2 plugins on windows but these are not maintained and will probably require some edits in order to compile. 

Requirements:

Some of these are optional, but without them the build-scripts will need to be edited, otherwise you'll get errors.

Windows

Msysgit http://code.google.com/p/msysgit/
VS2017 Community
Python 2 or 3 http://www.python.org/
Innosetup http://www.jrsoftware.org/isinfo.php
7zip http://www.7-zip.org/ (if you want to zip instead of make an installer)
Pace tools and certificate for code signing AAX binaries

Mac

Xcode 8+ installed, including command-line tools
Packages for building OSX installers http://s.sudre.free.fr/Software/Packages/about.html
setfileicon utility http://maxao.free.fr/telechargements/setfileicon.gz
Mac Developer ID Certificates for code signing installers for 10.8>
Mac 3rd Party App Dev Certificates for code signing binaries and installers for the Mac App Store
Pace tools and certificate for code signing AAX binaries

About the examples:

IPlugChunks - shows how to use chunks in a plugin. Chunks allow you to store arbitrary data in the plugin's state as opposed to just a value for each parameter.
IPlugControls -  demos the various IControl classes (example by Captain Caveman)
IPlugConvoEngine - demos WDL_ConvolutionEngine() - Cockos' fast convolution engine (example by Tale)
IPlugDistortion - demos Tale's bessel filter implementation for realtime oversampling (example by Tale)
IPlugEEL - demonstrates using Cockos' EEL library for run-time expression evaluation 
IPlugEffect - The most basic IPlug plugin, a gain control similar to AGain in the VST2 SDK, however it has a GUI
IPlugGUIResize - has three buttons to choose different gui sizes at runtime
IPlugHostDetect - displays the host name and version (not very reliable at the moment)
IPlugMonoSynth - a basic monophonic IPlug synth, showing how to handle MIDI messages sample accurately. Also shows how to use Tale's IKeyboardControl.
IPlugMouseTest - demonstrates an XY pad IControl which is linked to two plugin parameters
IPlugMultiChannel  - demos a multi-channel IPlug plugin, and how to test if channels are connected
IPlugMultiTargets - a midi effect plugin that also demos compilation to IOS, getting tempo info, and pop up menus
IPlugOpenGL - using OpenGL in IPlug
IPlugPlush -  shows how to use Cockos' Plush to do basic 3D Graphics
IPlugPolySynth - a basic polyphonic IPlug synth
IPlugResampler - demonstrates using WDL_Resampler
IPlugSideChain - a plugin that shows how to setup a sidechain input, for VST3, AU
IPlugText - demos different ways to draw text

The IPlugEffect project is the main starting project I use. If you are not interested in AAX / APP etc, I suggest you duplicate this and manually remove those targets to give you a new clean starting template with just your preferred formats in it.

Rather than changing settings for individual targets/projects inside the Xcode Project/Visual Studio solutions, most customisations can be done in the xcconfig and property sheets.

About the supported formats:

VST2

You need to two files from the Steinberg VST2.4 SDK to the folder VST_SDK, see VST_SDK/readme.txt

- On OSX by default I build to the system VST2 folder /Library/Audio/Plug-Ins/VST/ - You will need to have write permissions to this folder. If you want to build to the user VST2 folder, you'll need to edit the common.xcconfig file and also modify the installer scripts

VST3

Extract the Steinberg VST3.6.7 SDK to the folder VST3_SDK but get ready to revert two of the files using git... 

- WDL-OL has customised Xcode and VS2017 projects for the VST3 base library. Make sure you didn't overwrite them when you extracted the VST3 SDK or if you did just revert the changes in git.

   VST3_SDK\base\mac\base.Xcodeproj/*
	 VST3_SDK\base\win\base.vcxproj

- On windows, make sure you have a folder C:\Program Files\Common Files\VST3, otherwise the copy files build stage will cause the build to fail

- On OSX by default I build to the system VST3 folder /Library/Audio/Plug-Ins/VST3/ - You will need to have write permissions to this folder. If you want to build to the user VST3 folder, you'll need to edit the common.xcconfig file and also modify the installer scripts

AAX

- Extract AAX_SDK_2p1p1.zip to the AAX_SDK folder

- If you want to compile against the 10.5 SDK on OSX (see common.xcconfig) , you must change the SDKROOT setting in AAX_Library.Xcodeproj to macosx10.5
 
- Also modify ExamplePlugIns/Common/Mac/CommonDebugSettings.xcconfig and ExamplePlugIns/Common/Mac/CommonReleaseSettings.xcconfig...

GCC_VERSION = com.apple.compilers.gcc.4_2
SDKROOT = macosx10.5
MACOSX_DEPLOYMENT_TARGET = 10.5
ARCHS = x86_64 i386

- In order to compile AAX binaries that run in the release build of ProTools, you will need to code-sign those binaries (see Avid docs)

Audio Unit

- When building AUs, bear in mind that some hosts keep a cache... see debugging notes below.
- there is a special shell script validate_audiounit.command that will run the auvaltool command line app with the correct IDs for your plugin (see below) and can also set up the leaks test, which is useful for debugging. Type man auval  or see here for info.
- By default I build to the system audiounits folder /Library/Audio/Plug-Ins/Components/ - You will need to have write permissions to this folder. If you want to build to the user audiounits folder, you'll need to edit the .xcconfig file and also modify the installer scripts

Standalone

- Audio and Midi is provided via RTAudio and RTMidi by Gary Scavone. To build on windows you need to extract some files into ASIO_SDK, see ASIO_SDK/readme.txt


Windows Issues

The template projects use static linking with the MSVC2013 runtime libraries (/MT). If you change that you may need to provide the MSVC redistributable in your installer, google for "Microsoft Visual C++ 2013 Redistributable Package"

OSX Issues

For OSX 10.8 GateKeeper you will need to code-sign your installer and the .app with a valid signature obtained from Apple (maybe eventually also the plugin binaries). For the app store you need to add entitlements in order to comply with the sandbox regulations. These things are done by the makedist-* build scripts.

If compiling against the 10.7 SDK or higher carbon GUIs will be inefficient due to unnecessary redraws

Duplicating Projects

The IPlugExamples folder contains a python script to duplicate an IPlug project. This allows you to very quickly create a new project based on one of the examples. It does a multiple file find and replace to substitute the new name of the project for the old name, and also to change the manufacturer name. Once you have done this you only need to change two more things by hand in resource.h to make your plugin unique.

You can duplicate a project as follows with the following commands in the OSX terminal or on the windows command prompt. In this example i will copy the IPlugEffect project to a new project called MyNewPlugin…

- open terminal or cmd.exe and navigate to the IPlugExamples folder
- type 
	duplicate.py [inputprojectname] [outputprojectname] [manufacturername]

e.g
	duplicate.py IPlugEffect MyNewPlugin OliLarkin

you might need to do ./duplicate.py on OSX

- open  MyNewPlugin/resource.h and change PLUG_UNIQUE_ID and PLUG_MFR_ID


Debugging Setups

.xcscheme files are set up to use some common hosts for debugging the various formats in Xcode.

To debug an Audiounit using auval, remember to change the auval executable arguments to match plugin's type and IDs: aufx/aumf/aumu PLUG_UNIQUE_ID PLUG_MFR_ID

AU hosts cache information about the plugin I/O channels etc, so I have added a build script that deletes the caches after a build. If this becomes annoying (it will cause Logic to rescan plugins) you can disable it.

There is also a validate_audiounit.command shell script which will is a helper that runs auval with your plugins' unique IDs, and optionally performs the leaks test.

You should install VSTHost to C:\Program Files\VSTHost\vsthost.exe (on x64 you should install the 64bit version here)

To debug AAX you need to install the PT Dev build

Installers & one-click build scripts

The example projects contain shell scripts for both Windows (makedist-win.bat) and OSX (makedist-mac.command) that build everything, code-sign (where relevant) and package the products in an installer including license, readme.txt, changelog.txt and manual. On Windows the installer program "Innosetup" is used, on OSX "Packages". A python script update_version.py is called to look at resource.h and get the version from the PLUG_VER #define. It then updates the info.plist files and installer scripts with the version number (in the format major.minor.bugfix). If you aren't building some components, e.g. RTAS, the build scripts may need to be modified. Please alter the license and readme text and remove my name from them if you release a plugin publicly. On OSX the script can also code-sign the standalone app and builds a .pkg for the appstore (commented out).

