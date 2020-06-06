These examples all serve as templates, which can be duplicated to form the start of your project.

* **IPlugEffect** : A basic audio effect which is a volume control.
* **IPlugInstrument** : An MPE-capable polyphonic synthesiser.
* **IPlugControls** : A demonstration of the widgets available in the IControls library.
* **IPlugFaustDSP** : A plug-in that uses FAUST to implent its DSP and JIT-compile FAUST code in debug builds.
* **IPlugMidiEffect** : A basic MIDI effect plugin. Note: only AudioUnits really have a notion of a midi effect.
* **IPlugSideChain** : Demonstrates how to do a plug-in with two input buses for effects that require sidechain inputs such as compressors/gates.
* **IPlugSurroundEffect** : A  multichannel volume control effect plug-in that should work on different surround buses..
* **IPlugDrumSynth** : A drum synthesiser example with multiple output buses.
* **IPlugResponsiveUI** : An example of how to make a responsive UI that adapts to the platform window that can be maximized and resized using the OS window chrome. 

More specialized examples:

* **IPlugReaperExtension** : This is a template project for making a [Reaper Extension](http://reaper.fm/sdk/plugin/plugin.php). No realtime audio processing code, obviously. Making a reaper extension can be painful since it is all based around the Win32 APIs. This abstracts away some of the nastyness.
* **IPlugSwift** : An iOS AUv3 project using Swift/UIKit for the user interface 
* **IPlugWebUI** : An example showing how UI can written in HTML/CSS/JS, using a platform web view
