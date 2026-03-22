# Custom Controls

## Decision Tree

1. **Built-in control exists** -> use it (see `references/controls-catalog.md`)
2. **Custom drawing, no interaction** -> `ILambdaControl` with draw lambda
3. **Custom drawing + mouse interaction** -> subclass `IControl`
4. **Want IVStyle theming on custom control** -> subclass `IControl` + mix in `IVectorBase`
5. **Bitmap-based custom control** -> subclass `IControl` + mix in `IBitmapBase`
6. **Container with child controls** -> subclass `IContainerBase`

## ILambdaControl (Quick Inline Drawing)

No subclass needed. Receives animation progress for simple animations.

```cpp
pGraphics->AttachControl(new ILambdaControl(bounds,
  [](ILambdaControl* pCaller, IGraphics& g, IRECT& rect) {
    g.FillRect(COLOR_RED, rect);
  }, DEFAULT_ANIMATION_DURATION, false /*loop*/, false /*startImmediately*/));
```

## Minimal Custom IControl

```cpp
class MyControl : public IControl
{
public:
  MyControl(const IRECT& bounds, int paramIdx = kNoParameter)
  : IControl(bounds, paramIdx)
  {}

  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_RED, mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    // Handle click...
    SetDirty(true); // mark for redraw + notify delegate
  }

  // Optional overrides:
  // void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override {}
  // void OnMouseUp(float x, float y, const IMouseMod& mod) override {}
  // void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override {}
};
```

Key members: `mRECT` (draw rect), `mTargetRECT` (hit test rect), `GetValue()`/`SetValue()` (normalized 0-1), `mMouseIsOver`, `mDblAsSingleClick`.

## IVectorBase Mixin (Styled Custom Control)

Adds IVStyle theming to a custom control. Uses multiple inheritance.

```cpp
class MyVControl : public IControl, public IVectorBase
{
public:
  MyVControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE)
  : IControl(bounds)
  , IVectorBase(style)
  {
    AttachIControl(this, label); // required -- links mixin to control
  }

  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    DrawValue(g, mMouseIsOver);
  }

  void DrawWidget(IGraphics& g) override
  {
    g.FillRect(GetColor(kFG), mWidgetBounds);
  }

  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT)); // recalculates label/value/widget sub-rects
  }
};
```

Key IVectorBase methods:
- `GetColor(EVColor)` -- access style colors
- `mWidgetBounds` -- computed widget area (after label/value space deducted)
- `MakeRects(rect)` -- recalculate sub-rects, returns target rect
- `DrawPressableShape(g, shape, bounds, pressed, mouseOver, disabled)` -- draw interactive shape with states

## IBitmapBase Mixin (Bitmap Custom Control)

```cpp
class MyBControl : public IControl, public IBitmapBase
{
public:
  MyBControl(const IRECT& bounds, const IBitmap& bitmap, int paramIdx = kNoParameter)
  : IControl(bounds, paramIdx)
  , IBitmapBase(bitmap)
  {
    AttachIControl(this);
  }

  void Draw(IGraphics& g) override { DrawBitmap(g); }
  void OnRescale() override { mBitmap = GetUI()->GetScaledBitmap(mBitmap); }
};
```

## Layer Caching

For expensive drawing that does not change every frame (e.g., static backgrounds, complex paths). Draw once into a layer, then blit the cached result.

```cpp
class CachedControl : public IControl
{
public:
  CachedControl(const IRECT& bounds) : IControl(bounds) {}

  void Draw(IGraphics& g) override
  {
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(this, mRECT);
      // Expensive drawing here...
      g.FillEllipse(COLOR_BLUE, mRECT);
      mLayer = g.EndLayer();
    }
    g.DrawLayer(mLayer);
  }

  void OnResize() override { mLayer = nullptr; } // invalidate on resize

private:
  ILayerPtr mLayer;
};
```

## Drawing Primitives Quick Reference

Common IGraphics methods available in `Draw(IGraphics& g)`:

### Shape Fills
`FillRect`, `FillRoundRect`, `FillCircle`, `FillArc`, `FillEllipse`, `FillTriangle`, `FillConvexPolygon`

### Shape Strokes
`DrawRect`, `DrawRoundRect`, `DrawCircle`, `DrawArc`, `DrawEllipse`, `DrawLine`, `DrawDottedLine`, `DrawDottedRect`, `DrawTriangle`

### Text
`DrawText(IText, const char*, IRECT)` -- text within bounds

### Bitmaps
`DrawBitmap(bitmap, bounds, frame)`, `DrawRotatedBitmap`, `DrawFittedBitmap`

### Path API (for complex shapes)
```cpp
g.PathClear();
g.PathMoveTo(x, y);
g.PathLineTo(x, y);
g.PathArc(cx, cy, r, startAngle, endAngle);
g.PathCubicBezierTo(cx1, cy1, cx2, cy2, x, y);
g.PathQuadraticBezierTo(cx, cy, x, y);
g.PathClose();
g.PathFill(pattern);               // fill with color/gradient
g.PathStroke(pattern, thickness);   // stroke with color/gradient
```

### Data Arrays
`DrawData(pattern, bounds, data, nPoints, pHighlightData, thickness)`

## Skia vs NanoVG

Both backends implement the same `IGraphics` interface. Controls are backend-agnostic.

| Aspect | NanoVG | Skia |
|--------|--------|------|
| Binary size | Small | Large |
| Dependencies | None (default) | Must download |
| Text quality | Good | Superior (SkParagraph) |
| Drawing primitives | Standard set | More primitives |
| GPU backends | GL2/3, GLES2/3, Metal | Metal, GL, CPU |
| Best for | Most UIs | Complex text, advanced rendering |

To access the Skia canvas directly for Skia-specific drawing:
```cpp
void Draw(IGraphics& g) override
{
  SkCanvas* canvas = static_cast<SkCanvas*>(g.GetDrawContext());
  // Use Skia API directly...
}
```

Backend selection is determined at compile time via `IGraphics_select.h`. The same control code works on either backend -- only direct canvas access is backend-specific.

## Animation

```cpp
// Start an animation on a control
SetAnimation([](IControl* pCaller) {
  float progress = pCaller->GetAnimationProgress();
  if (progress > 1.f) {
    pCaller->OnEndAnimation();
    return;
  }
  // Animate based on progress (0.0 to 1.0+)
  pCaller->SetValue(progress);
}, 200 /* duration ms */);

// Built-in splash animation for IV controls
SetAnimation(SplashAnimationFunc, DEFAULT_ANIMATION_DURATION);

// Action function triggered on click, with animation end callback
control->SetActionFunction(SplashClickActionFunc);
control->SetAnimationEndActionFunction([](IControl* pCaller) {
  // Runs after animation completes
});
```

`DefaultClickActionFunc` triggers a `DEFAULT_ANIMATION_DURATION` animation. `SplashClickActionFunc` triggers a radial splash effect.
