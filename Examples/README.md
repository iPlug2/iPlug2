These examples all serve as templates, which can be duplicated to form the start of your project.

There are three examples you should check out first:

* **IPlugEffect** : A basic audio effect which is a volume control.
* **IPlugInstrument** : An MPE-capable polyphonic synthesiser.
* **IPlugControls** : A demonstration of the widgets available in the IControls library.

The following examples are more specialized:

* **IPlugChunks** : Shows how to store data other than just parameter values in the plug-in state and how to get tempo info from the host.
* **IPlugMidiEffect** : A basic MIDI effect plugin. Note: only AudioUnits really have a notion of a midi effect.
* **IPlugSideChain** : Demonstrates how to do a plug-in with two input buses for effects that require sidechain inputs such as compressors/gates.
* **IPlugSurroundEffect** : A  multichannel volume control effect plug-in that should work on different surround buses.
* **IPlugDrumSynth** : A drum synthesiser example with multiple output buses.
* **IPlugResponsiveUI** : An example of how to make a responsive UI that adapts to the platform window that can be maximized and resized using the OS window chrome. 
* **IPlugFaustDSP** : A plug-in that uses FAUST to implent its DSP and JIT-compile FAUST code in debug builds.
* **IPlugSOUL** : An plug-in where the DSP is generated using the SOUL programming language
* **IPlugOSCEditor** : Demonstrates how to use the Open Sound Control classes in iPlug2, as well as the IWebViewControl
* **IPlugReaperExtension** : This is a template project for making a [Reaper Extension](http://reaper.fm/sdk/plugin/plugin.php). No realtime audio processing code, obviously. Making a reaper extension can be painful since it is all based around the Win32 APIs. This abstracts away some of the nastyness.
* **IPlugSwift** : An iOS AUv3 project using Swift/UIKit for the user interface 
* **IPlugWebUI** : An example showing how UI can written in HTML/CSS/JS, using a platform web view
