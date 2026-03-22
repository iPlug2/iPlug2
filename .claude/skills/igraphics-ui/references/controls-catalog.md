# Controls Catalog

All controls are in `IGraphics/Controls/IControls.h` unless noted. Include with `#include "IControls.h"`.

## Vector Controls (IV prefix)

Resolution-independent, styleable via IVStyle, work on all backends.

### Knobs & Sliders
- **IVKnobControl**(bounds, paramIdx, "Label", style, drawFromCenter) -- rotary dial, most common continuous control
- **IVSliderControl**(bounds, paramIdx, "Label", style, showParamLabel, direction, gearing, handleSize, trackSize, drawFromCenter) -- linear fader, vertical or horizontal
- **IVRangeSliderControl**(bounds, {paramLo, paramHi}, "Label", style, direction) -- two-handle range selector
- **IVXYPadControl**(bounds, {paramX, paramY}, "Label", style) -- 2D pad controlling two parameters
- **IVMultiSliderControl\<N\>**(bounds, "Label", style, paramStart, direction, minVal, maxVal) -- N independent sliders (sequencer steps, EQ bands)
- **IVNumberBoxControl**(bounds, paramIdx, "Label", style, showParamLabel, drawTriangles) -- numeric input with up/down

### Buttons & Switches
- **IVButtonControl**(bounds, actionFunc, "Label", style, labelInButton, valueLabelInButton) -- momentary push button (no parameter; uses action function)
- **IVSwitchControl**(bounds, paramIdx, "Label", style) -- cyclic multi-state switch
- **IVToggleControl**(bounds, actionFunc, "Label", style, offText, onText) -- two-state on/off toggle
- **IVSlideSwitchControl**(bounds, paramIdx, "Label", style, showLabel) -- animated sliding switch
- **IVTabSwitchControl**(bounds, actionFunc, {"Tab1", "Tab2"}, "Label", style, shape, direction) -- tab bar
- **IVRadioButtonControl**(bounds, paramIdx, {"Opt1", "Opt2"}, "Label", style, shape, direction, spacing) -- radio button group
- **IVMenuButtonControl**(bounds, paramIdx, "Label", style) -- button that opens dropdown menu

### Meters & Visualization
- **IVMeterControl\<N\>**(bounds, "Label", style, direction, {"L","R"}) -- N-channel peak meter
- **IVLEDMeterControl\<N\>**(bounds, "Label", style, direction, {"L","R"}) -- LED-style meter with peak hold
- **IVPeakAvgMeterControl\<N\>**(bounds, "Label", style, direction, {"L","R"}) -- peak + RMS meter
- **IVScopeControl\<N, BufSize\>**(bounds, "Label", style) -- oscilloscope waveform display
- **IVSpectrumAnalyzerControl\<N\>**(bounds, "Label", style) -- FFT spectrum analyzer with configurable FFT size, overlap, window
- **IVBarGraphSpectrumAnalyzerControl\<N\>**(bounds, "Label", style, nBars, nSegments, freqScale, colorMode, channelMode, gradient) -- bar graph spectrum (in `IVBarGraphSpectrumAnalyzerControl.h`)
- **IVDisplayControl**(bounds, "Label", style, direction, lo, hi, defaultVal, historySize) -- rolling value graph
- **IVPlotControl**(bounds, {"Plot1"}, numPoints, "Label", style, lo, hi, useLayer) -- function plotter

### Keyboard
- **IVKeyboardControl**(bounds, minNote, maxNote, roundedKeys, drawShadows, drawFrame) -- MPE-capable piano keyboard
  - Use with `SetQwertyMidiKeyHandlerFunc` for computer keyboard input
  - Update from MIDI with `SetNoteFromMidi(noteNum, isOn)`

### Containers & Layout
- **IVGroupControl**("Title", "groupName", padL, padT, padR, padB, style) -- draws frame around a named group of controls
- **IVPanelControl**(bounds, "Label", style) -- styled panel/background area
- **IVTabbedPagesControl**(bounds, {"Page1", "Page2"}, "Label", style) -- tab-based page switcher

### Misc
- **IVLabelControl**(bounds, "Text", style) -- styled text label with optional shadow
- **IVColorSwatchControl**(bounds, "Label", colorSelectedFunc, style, shape, colors) -- color display/picker
- **IVPresetManagerControls**(bounds, style) -- preset save/load/navigate UI

---

## Bitmap Controls (IB prefix)

Frame-based bitmap strips. Load bitmaps with `pGraphics->LoadBitmap(RESOURCE_FN, nFrames, framesAreHorizontal)`.

- **IBKnobControl**(bounds, bitmap, paramIdx) -- knob from frame strip
- **IBKnobRotaterControl**(bounds, bitmap, paramIdx) -- single bitmap rotated by value
- **IBButtonControl**(bounds, bitmap, actionFunc) -- button from frame strip
- **IBSwitchControl**(bounds, bitmap, paramIdx) -- cyclic switch from frame strip
- **IBSliderControl**(bounds, handleBitmap, trackBitmap, paramIdx, direction) -- slider with handle + track bitmaps
- **IBTextControl**(bounds, bitmap, labelText, str, charWidth, charHeight, charOffset, multiLine) -- text from character bitmap
- **IBMeterControl**(bounds, bitmap, direction) -- meter from frame strip

---

## SVG Controls (ISVG prefix)

Scalable vector graphics assets. Load SVGs with `pGraphics->LoadSVG(RESOURCE_FN)`.

- **ISVGKnobControl**(bounds, svg, paramIdx) -- rotated SVG knob
- **ISVGButtonControl**(bounds, onSVG, offSVG) -- SVG button with on/off states
- **ISVGToggleControl**(bounds, onSVG, offSVG) -- SVG toggle
- **ISVGSwitchControl**(bounds, {svg1, svg2, ...}, paramIdx) -- multi-state SVG switch
- **ISVGSliderControl**(bounds, handleSVG, trackSVG, paramIdx, direction) -- SVG slider

---

## Special Controls

- **ITextControl**(bounds, "text", textStyle, bgColor) -- static text display
- **IEditableTextControl**(bounds, "text", textStyle) -- editable text field
- **ICaptionControl**(bounds, paramIdx, textStyle, bgColor, showParamLabel) -- displays parameter value, click to edit
- **IRTTextControl\<N, T\>**(bounds, fmtStr, separator, "Label") -- real-time text updated from ISender data
- **IURLControl**(bounds, "label", "url", textStyle) -- clickable URL link
- **ITextToggleControl**(bounds, paramIdx, offStr, onStr, textStyle) -- text/icon toggle (pairs with ForkAwesome/Fontaudio icon fonts)
- **ILEDControl**(bounds, offIntensity) -- LED indicator light, set value to flash
- **ILambdaControl**(bounds, drawFunc, animDuration, loop, startImmediately) -- inline drawing with lambda
- **IColorPickerControl**(bounds, selectedColor, style) -- HSL/RGB color picker dialog
- **IWebViewControl**(bounds, enableDevTools, enableScroll, opaque) -- embedded web view (in `IWebViewControl.h`)
- **IPlatformViewControl**(bounds, opaque) -- embedded native platform view
- **IShaderControl**(bounds) -- GPU shader rendering (in `IShaderControl.h`)
- **ICornerResizerControl** -- corner drag handle for window resize (attached via `AttachCornerResizer`)
- **IBubbleControl** -- tooltip bubble (attached via `AttachBubbleControl`)
- **IFPSDisplayControl** -- FPS counter (enabled via `ShowFPSDisplay(true)`)
