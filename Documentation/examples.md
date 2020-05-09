# Examples
[TOC]
@section intro Introduction
The Examples folder contains example/template projects to demonstrate how to use different features of iPlug2. They also serve as a testbed to make sure that the various aspects are working. Each folder contains scripts which automate the build process and package everything along with a pdf manual in an installer. The scripts also code sign binaries/installers and set icons where required.

Visual Studio 2019+ and Xcode 10+, which are both available for free are supported. If you know what you're doing you may be able to get things to compile with other IDEs/versions, but you are *highly* recommended to use the latest versions.

@note No effort is made to make the code compatible with other IDEs, or compilers such as MinGW.
@par
Although Visual Studio and Xcode required to build IPlug plug-ins, projects are setup in such a way that you may easily use your favourite text editor and call build scripts, to build the binaries (although then you lack the debugger functionality of the IDE).

@section examples List of examples included
The following projects are included:

Example                  | Description
-------------------------|----------------------------------------
**IPlugEffect**          | A basic audio effect which is a volume control.
**IPlugInstrument**      | An MPE-capable polyphonic synthesiser.
**IPlugControls**        | A demonstration of the widgets available in the IControls library.
**IPlugFaustDSP**        | A plug-in that uses FAUST to implent its DSP and JIT-compile FAUST code in debug builds.
**IPlugMidiEffect**      | A basic MIDI effect plugin.
**IPlugReaperExtension** | This is a template project for making a [Reaper Extension](http://reaper.fm/sdk/plugin/plugin.php). 
**IPlugSwift**           | An iOS AUv3 project using Swift/UIKit for the user interface 
**IPlugWebView**         | An example (macOS/iOS only) that uses an embedded WebKit view

@note There are included scripts `buildall-mac.sh` and `buildall-win.bat` to quickly build all examples.
@todo Add pretty pictures here

@section duplicate Using duplicate.py

These examples all serve as templates, which can be duplicated to form the start of your project. To create a new project, use `duplicate.py`.

    duplicate.py [inputprojectname] [outputprojectname] [manufacturername] (outputpath)

@todo Explain the command line parameters
@note If you're using Windows, you will need to install Python (both version 2 and 3 should be fine) and set it up so you can run it from the command line.
@par
See http://www.voidspace.org.uk/python/articles/command_line.shtml for help. This involves adding the Python folder (e.g. C:\Python27\) to your %PATH% environment variable.

@warning This script was not designed to be foolproof - please think carefully about what you choose for a project name.
@par
It's best to stick to standard characters in your project names - avoid spaces, numbers and dots.

@section IPlugEffect
@section IPlugInstrument
@section IPlugControls
@section IPlugFaustDSP

@section IPlugMidiEffect
@note Only AudioUnits really have a notion of a MIDI effect.

@section IPlugReaperExtension
@section IPlugSwitft
@section IPlugWebView