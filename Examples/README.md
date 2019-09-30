These examples all serve as templates, which can be duplicated to form the start of your project.

* **IPlugEffect** : A basic audio effect which is a volume control. [NANOVG/WebGL](https://iplug2.github.io/NANOVG/IPlugEffect/) | [HTML5 Canvas](https://iplug2.github.io/CANVAS/IPlugEffect/)
* **IPlugInstrument** : An MPE-capable polyphonic synthesiser.[NANOVG/WebGL](https://iplug2.github.io/NANOVG/IPlugInstrument/) | [HTML5 Canvas](https://iplug2.github.io/CANVAS/IPlugInstrument/)
* **IPlugControls** : A demonstration of the widgets available in the IControls library. [NANOVG/WebGL](https://iplug2.github.io/NANOVG/IPlugControls/) | [HTML5 Canvas](https://iplug2.github.io/CANVAS/IPlugControls/)
* **IPlugFaustDSP** : A plug-in that uses FAUST to implent its DSP and JIT-compile FAUST code in debug builds. [NANOVG/WebGL](https://iplug2.github.io/NANOVG/IPlugFaustDSP/) | [HTML5 Canvas](https://iplug2.github.io/CANVAS/IPlugFaustDSP/)
* **IPlugMidiEffect** : A basic MIDI effect plugin. Note: only AudioUnits really have a notion of a midi effect. [NANOVG/WebGL](https://iplug2.github.io/NANOVG/IPlugMidiEffect/) | [HTML5 Canvas](https://iplug2.github.io/CANVAS/IPlugMidiEffect/)
* **IPlugReaperExtension** : This is a template project for making a [Reaper Extension](http://reaper.fm/sdk/plugin/plugin.php). No realtime audio processing code, obviously. Making a reaper extension can be painful since it is all based around the Win32 APIs. This abstracts away some of the nastyness. 
* **IPlugWebView** : An example (macOS/iOS only) that doesn't use IGraphics for UI, instead it embeds a WKWebKitView to allow UI to be written in HTML/CSS/JS
