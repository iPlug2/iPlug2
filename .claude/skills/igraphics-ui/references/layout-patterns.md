# Layout Patterns

## IRECT Quick Reference

IRECT represents a rectangle with float members `L`, `T`, `R`, `B` (left, top, right, bottom). All methods return new IRECTs unless noted otherwise.

### Size Queries
- `W()`, `H()` -- width, height
- `MW()`, `MH()` -- midpoint X, midpoint Y
- `Area()` -- total area
- `Empty()` -- true if zero area
- `Contains(x, y)` -- point-in-rect test

### Padding & Centering
- `GetPadded(amount)` -- shrink all edges uniformly (negative = expand)
- `GetPadded(l, t, r, b)` -- per-edge padding
- `GetHPadded(amount)` -- shrink left/right only
- `GetVPadded(amount)` -- shrink top/bottom only
- `GetMidHPadded(halfW)` -- center horizontally with given half-width
- `GetMidVPadded(halfH)` -- center vertically with given half-height
- `GetCentredInside(w, h)` -- center a w*h rect inside
- `GetCentredInside(size)` -- center a square inside
- `GetCentredInside(IRECT)` -- center another rect's dimensions inside

### Edge Extraction (non-mutating)
- `GetFromTop(h)` -- top strip of height h
- `GetFromBottom(h)` -- bottom strip
- `GetFromLeft(w)` -- left strip of width w
- `GetFromRight(w)` -- right strip
- `GetFromTLHC(w, h)` -- top-left corner rect
- `GetFromTRHC(w, h)` -- top-right corner rect
- `GetFromBLHC(w, h)` -- bottom-left corner rect
- `GetFromBRHC(w, h)` -- bottom-right corner rect

### Reduce (mutating -- modifies the source rect, returns the removed portion)
- `ReduceFromTop(h)` -- removes h from top, returns removed strip
- `ReduceFromBottom(h)` -- removes h from bottom
- `ReduceFromLeft(w)` -- removes w from left
- `ReduceFromRight(w)` -- removes w from right

### Get-Reduced (non-mutating -- returns remaining portion after removing from edge)
- `GetReducedFromTop(h)` -- rect with top h removed
- `GetReducedFromBottom(h)` -- rect with bottom h removed
- `GetReducedFromLeft(w)` -- rect with left w removed
- `GetReducedFromRight(w)` -- rect with right w removed

### Fractional Division
- `FracRectVertical(frac, fromTop)` -- fraction of height from top or bottom
- `FracRectHorizontal(frac, fromRight)` -- fraction of width from left or right
- `SubRectVertical(nSlices, idx)` -- divide height into N equal slices, get idx
- `SubRectHorizontal(nSlices, idx)` -- divide width into N equal slices, get idx

### Grid
- `GetGridCell(row, col, nRows, nCols)` -- cell at row/col position
- `GetGridCell(cellIdx, nRows, nCols)` -- cell at linear index (row-major)
- `GetGridCell(row, col, nRows, nCols, dir)` -- with direction override

### Transform
- `GetTranslated(dx, dy)` -- move by offset
- `GetVShifted(dy)` -- shift vertically
- `GetHShifted(dx)` -- shift horizontally
- `GetScaledAboutCentre(scale)` -- scale uniformly about center
- `Union(other)` -- smallest rect containing both
- `Intersect(other)` -- overlap region

### Construction Helpers
- `IRECT::MakeXYWH(l, t, w, h)` -- from position and size
- `IRECT::MakeMidXYWH(cx, cy, w, h)` -- centered at point

---

## Pattern: Manual Placement

For UIs with 1-3 controls. Simple IRECT math on `GetBounds()`.

```cpp
const IRECT b = pGraphics->GetBounds().GetPadded(-10.f);
pGraphics->AttachControl(new IVKnobControl(b.GetCentredInside(100), kParamGain, "Gain"));
```

From `Examples/IPlugEffect/`.

## Pattern: Grid Layout

For regular grids of controls. Uses a cell index counter with helper lambdas.

```cpp
const int nRows = 5, nCols = 8;
int cellIdx = -1;

auto nextCell = [&]() {
  return b.GetPadded(-5.).GetGridCell(++cellIdx, nRows, nCols).GetPadded(-5.);
};
auto sameCell = [&]() {
  return b.GetPadded(-5.).GetGridCell(cellIdx, nRows, nCols).GetPadded(-5.);
};

// Use nextCell() to advance, sameCell() to subdivide the current cell
pGraphics->AttachControl(new IVKnobControl(nextCell().GetCentredInside(90), kGain, "Gain"));
pGraphics->AttachControl(new IVSliderControl(nextCell(), kFreq, "Freq"));

// Multi-cell regions via Union:
IRECT wide = nextCell().Union(nextCell()).Union(nextCell());
```

From `Examples/IPlugControls/`.

## Pattern: Proportional Panels

For UIs with distinct regions (header, sidebar, main area). Uses `FracRect`, `ReduceFrom`, and `GetGridCell` for sub-layout within panels.

```cpp
const IRECT b = pGraphics->GetBounds().GetPadded(-20.f);
const IRECT lfoPanel = b.GetFromLeft(300.f).GetFromTop(200.f);
IRECT keyboardBounds = b.GetFromBottom(300);
IRECT wheelsBounds = keyboardBounds.ReduceFromLeft(100.f).GetPadded(-10.f);

// Grid within a panel:
const IRECT controls = b.GetGridCell(1, 2, 2);
pGraphics->AttachControl(new IVKnobControl(controls.GetGridCell(0, 2, 6).GetCentredInside(90), kGain));

// Union cells for wider regions:
const IRECT sliders = controls.GetGridCell(2, 2, 6)
  .Union(controls.GetGridCell(3, 2, 6))
  .Union(controls.GetGridCell(4, 2, 6));
```

From `Examples/IPlugInstrument/`.

## Pattern: Responsive Layout

For resizable windows. Controls are repositioned on resize without being recreated.

```cpp
mLayoutFunc = [&](IGraphics* pGraphics) {
  const IRECT b = pGraphics->GetBounds();

  auto GetBounds = [](int ctrlIdx, const IRECT& b) {
    IRECT main = b.GetPadded(-5.f);
    switch (ctrlIdx) {
      case 0: return b;                                    // background
      case 1: return main.FracRectVertical(0.25, false);   // keyboard
      case 2: return main.FracRectVertical(0.75, true).ReduceFromRight(100.f); // gain
      case 3: return main.FracRectVertical(0.75, true).GetPadded(-10.f);       // scope
      default: return b;
    }
  };

  if (pGraphics->NControls()) {
    for (int i = 0; i < pGraphics->NControls(); i++)
      pGraphics->GetControl(i)->SetTargetAndDrawRECTs(GetBounds(i, b));
    return;
  }

  pGraphics->SetLayoutOnResize(true);
  // First-time control creation...
  pGraphics->AttachPanelBackground(COLOR_GRAY);
  pGraphics->AttachControl(new IVKeyboardControl(GetBounds(1, b)));
  pGraphics->AttachControl(new IVSliderControl(GetBounds(2, b), kGain));
  pGraphics->AttachControl(new IVScopeControl<>(GetBounds(3, b)), kCtrlTagScope);
};
```

From `Examples/IPlugResponsiveUI/`.

## Pattern: Flexbox (IFlexBox)

For CSS-like flexible layouts. Requires `#include "IGraphicsFlexBox.h"` and Yoga dependency.

```cpp
IFlexBox fb;
fb.Init(pGraphics->GetBounds(), YGFlexDirectionRow, YGJustifySpaceBetween, YGWrapNoWrap, 10.f);
fb.AddItem(100, YGUndefined, YGAlignCenter, 1.f);  // grow=1, auto height
fb.AddItem(200, YGUndefined, YGAlignCenter, 2.f);  // grow=2
fb.CalcLayout(YGDirectionLTR);

IRECT item0 = fb.GetItemBounds(0);
IRECT item1 = fb.GetItemBounds(1);
```

Parameters for `AddItem(width, height, alignSelf, grow, shrink, margin)`:
- Positive width/height = pixels, negative = percentage, `YGUndefined` = auto
- `grow`/`shrink` = flex ratios (default 0/1)
