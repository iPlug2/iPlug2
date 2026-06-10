---
name: igraphics-ui
description: This skill should be used when the user asks to "create a UI", "add controls", "layout controls", "design the interface", "add a knob", "add a slider", "add a button", "add a meter", "add a spectrum analyzer", "style controls", "theme the UI", "use IVStyle", "use ISender", "create a custom control", "add a keyboard", "make a resizable UI", or discusses IGraphics layout, control selection, styling, or visualization in an iPlug2 plugin.
---

# IGraphics UI Authoring

Guidance for building IGraphics-based UIs in iPlug2 plugins. Covers layout strategies, control selection, styling, audio-to-UI visualization, and custom controls.

## Core Pattern

Every IGraphics plugin constructs its UI with two lambdas in the plugin constructor, inside an `#if IPLUG_EDITOR` guard:

```cpp
mMakeGraphicsFunc = [&]() {
  return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS,
                      GetScaleForScreen(PLUG_WIDTH, PLUG_HEIGHT));
};

mLayoutFunc = [&](IGraphics* pGraphics) {
  pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
  pGraphics->AttachPanelBackground(COLOR_GRAY);
  pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
  pGraphics->EnableMouseOver(true);
  pGraphics->AttachTextEntryControl();
  pGraphics->AttachPopupMenuControl(DEFAULT_LABEL_TEXT);
  pGraphics->AttachBubbleControl();

  const IRECT b = pGraphics->GetBounds();
  // Attach controls here...
};
```

For responsive/resizable UIs, add `pGraphics->SetLayoutOnResize(true)` and use the `NControls()` early-return pattern to reposition controls on resize without recreating them:

```cpp
if (pGraphics->NControls()) {
  // Reposition existing controls
  pGraphics->GetControl(idx)->SetTargetAndDrawRECTs(newBounds);
  return;
}
```

Reference examples: `Examples/IPlugEffect/` (minimal), `Examples/IPlugInstrument/` (intermediate), `Examples/IPlugControls/` (comprehensive).

## Choosing a Layout Strategy

Select the simplest strategy that meets the need:

| Scenario | Strategy | Key Methods |
|----------|----------|-------------|
| 1-3 controls | Manual IRECT | `GetBounds().GetPadded(-10)`, `GetCentredInside(100)` |
| Regular grid | GetGridCell | `b.GetGridCell(cellIdx, nRows, nCols)` with `nextCell()` lambda |
| Proportional panels | Fraction/strip | `FracRectVertical()`, `ReduceFromLeft()`, `SubRectVertical()` |
| Resizable window | Responsive | `SetLayoutOnResize(true)` + GetBounds lambda |
| Complex flex | IFlexBox | Yoga wrapper: `Init()`, `AddItem()`, `CalcLayout()`, `GetItemBounds()` |

The grid pattern with `nextCell()`/`sameCell()` lambdas (from IPlugControls) is the most versatile for medium-complexity UIs:

```cpp
int cellIdx = -1;
auto nextCell = [&]() {
  return b.GetPadded(-5.).GetGridCell(++cellIdx, nRows, nCols).GetPadded(-5.);
};
auto sameCell = [&]() {
  return b.GetPadded(-5.).GetGridCell(cellIdx, nRows, nCols).GetPadded(-5.);
};
```

For proportional panel layouts, `ReduceFrom*` methods are powerful because they mutate the source rect and return the removed portion:

```cpp
IRECT b = pGraphics->GetBounds().GetPadded(-5);
const IRECT topBar = b.ReduceFromTop(50.f);    // topBar = 50px strip, b shrinks
const IRECT sidebar = b.ReduceFromLeft(100.f);  // sidebar = 100px strip, b shrinks
// b now contains the remaining area
```

For detailed IRECT method reference and layout examples, consult **`references/layout-patterns.md`**.

## Choosing Controls

**Prefer IV (vector) controls** -- no bitmap assets needed, styleable via IVStyle, backend-agnostic. Use IB/ISVG controls only when artist-provided assets dictate a specific look.

| Need | Control Type |
|------|-------------|
| Standard UI widgets | IV controls (IVKnobControl, IVSliderControl, etc.) |
| Artist-designed bitmap look | IB controls + `LoadBitmap(RESOURCE_FN, nFrames)` |
| Scalable artist assets | ISVG controls + `LoadSVG(RESOURCE_FN)` |
| Quick inline custom drawing | `ILambdaControl` with draw lambda |
| Complex custom interaction | Custom `IControl` subclass |

Attach controls with: `pGraphics->AttachControl(new IVKnobControl(bounds, paramIdx, "Label", style), ctrlTag, "groupName")`

- `paramIdx`: links to a plugin parameter. Use `kNoParameter` for unlinked controls.
- `ctrlTag`: integer tag for lookup via `GetControlWithTag(tag)`. Required for ISender targets.
- `groupName`: string for batch operations via `ForControlInGroup("name", func)`.

For the complete catalog of available controls, consult **`references/controls-catalog.md`**.

## Styling with IVStyle

Create a shared style and reuse it across controls for visual consistency:

```cpp
const IVStyle style = DEFAULT_STYLE
  .WithColor(kBG, COLOR_BLACK)
  .WithColor(kFG, IColor(255, 128, 128, 128))
  .WithRoundness(0.1f)
  .WithFrameThickness(3.f)
  .WithDrawShadows(false)
  .WithLabelText(IText(12.f, EAlign::Center));
```

The 9 EVColor slots control different aspects: kBG (background), kFG (foreground/off), kPR (pressed/on), kFR (frame), kHL (highlight/hover), kSH (shadow), kX1 (indicator track), kX2, kX3 (extras).

For the full IVStyle API, IVColorSpec, IText, IColor, and gradient patterns, consult **`references/styling-and-theming.md`**.

## Audio-to-UI Data (ISender)

To display real-time audio data (meters, scopes, spectrums), use the ISender pattern:

| Sender | Control | Use Case |
|--------|---------|----------|
| `IPeakSender<N>` | `IVMeterControl<N>` | Peak meters |
| `IPeakAvgSender<N>` | `IVLEDMeterControl<N>` | LED meters with ballistics |
| `IBufferSender<N>` | `IVScopeControl<N>` | Oscilloscope |
| `ISpectrumSender<N>` | `IVSpectrumAnalyzerControl<N>` | FFT spectrum |
| `ISender<N,Q,T>` | `IVDisplayControl` or custom | Generic data |

Setup steps:
1. Declare sender in plugin `.h`: `IPeakSender<2> mMeterSender;`
2. Define control tag: `enum ECtrlTags { kCtrlTagMeter = 0 };`
3. Attach control with tag: `pGraphics->AttachControl(new IVMeterControl<2>(bounds), kCtrlTagMeter);`
4. In `ProcessBlock`: `mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);`
5. In `OnIdle`: `mMeterSender.TransmitData(*this);`
6. In `OnReset`: `mMeterSender.Reset(GetSampleRate());` (for senders that need it)

For architecture details and spectrum analyzer bidirectional messaging, consult **`references/visualizers-and-isender.md`**.

## Control Communication

- **Parameter linking**: Controls linked to a paramIdx automatically sync with the plugin parameter.
- **Tag lookup**: `pGraphics->GetControlWithTag(kCtrlTag)` for direct access.
- **Groups**: Pass group name as 3rd arg to `AttachControl`, then `ForControlInGroup("name", func)` or `HideControl(paramIdx, bool)`.
- **Action functions**: `control->SetActionFunction([](IControl* pCaller) { ... })` for click callbacks.
- **Animation end**: `SetAnimationEndActionFunction(func)` runs after splash/animation completes.
- **Delegate messages**: `SendControlMsgFromDelegate(ctrlTag, msgTag, dataSize, pData)` for plugin-to-control messages.

## Custom Controls

**Decision tree**:
- Built-in control exists for the need -> use it
- Custom drawing, no interaction -> `ILambdaControl`
- Custom drawing + interaction -> subclass `IControl`
- Want IVStyle theming -> subclass `IControl` + mix in `IVectorBase`
- Container with children -> subclass `IContainerBase`

For custom control patterns, layer caching, drawing primitives, Skia vs NanoVG, and animation, consult **`references/custom-controls.md`**.

## Key Source Files

| Concept | File |
|---------|------|
| IRECT, IColor, IText, IVStyle | `IGraphics/IGraphicsStructs.h` |
| IControl, IVectorBase, IBitmapBase | `IGraphics/IControl.h` |
| All built-in controls | `IGraphics/Controls/IControls.h` |
| ISender, IPeakSender, ISpectrumSender | `IPlug/ISender.h` |
| Yoga flexbox | `IGraphics/Extras/IGraphicsFlexBox.h` |
| VS Code snippets | `.vscode/IGraphics.code-snippets` |
| Minimal example | `Examples/IPlugEffect/` |
| Intermediate example | `Examples/IPlugInstrument/` |
| Comprehensive showcase | `Examples/IPlugControls/` |
| Visualizer example | `Examples/IPlugVisualizer/` |
