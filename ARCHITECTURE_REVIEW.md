# iPlug2 Architecture Review

**Date:** December 2024
**Reviewer:** Senior Software Engineer Assessment
**Scope:** IPlug core, IGraphics, plugin implementation patterns

---

## Executive Summary

iPlug2 is a mature, functional cross-platform audio plugin framework that successfully abstracts 6+ plugin formats (VST3, CLAP, AU, AAX, WAM, App) from a single codebase. However, when compared to JUCE's architectural maturity, several areas require modernization to compete professionally:

| Area | Current State | JUCE Comparison |
|------|--------------|-----------------|
| **Separation of Concerns** | Weak - monolithic classes, tight coupling | Strong - modular components |
| **Testability** | Poor - no mocking support, deep inheritance | Better - some DI patterns |
| **Type Safety** | Moderate - enum-based params, manual casts | Better - typed parameters |
| **DX (Developer Experience)** | High boilerplate, manual wiring | Lower boilerplate, declarative |
| **Documentation** | Good API docs | Better tutorials/guides |

**Key Recommendation:** Adopt an incremental modernization strategy that introduces new abstractions alongside existing code, allowing gradual migration without breaking the robust plugin wrappers.

---

## Part 1: Core Architecture Analysis

### 1.1 Class Hierarchy Complexity

The current inheritance chain is deep and uses multiple inheritance:

```
IEditorDelegate
    └── IPluginBase
            └── IPlugAPIBase
                    └── IPlugVST3 (also inherits SingleComponentEffect, IPlugVST3ProcessorBase)
                            └── YourPlugin
```

**Problems:**
1. **5+ levels of inheritance** before user code
2. **Diamond inheritance risks** with multiple base classes
3. **Unclear responsibility boundaries** - where does preset logic end and API logic begin?
4. **friend class proliferation** - IPlugAPIBase has 11 friend classes (line 216-225)

**Evidence:** `IPlugAPIBase.h:216-225`
```cpp
friend class IPlugAPP;
friend class IPlugAAX;
friend class IPlugAU;
// ... 8 more
```

### 1.2 Parameter System Coupling

Parameters are stored in `IEditorDelegate` but accessed throughout the hierarchy:

```cpp
// In IEditorDelegate (base)
WDL_PtrList<IParam> mParams;

// Accessed via GetParam() everywhere
GetParam(kGain)->Value()  // Called from ProcessBlock, OnParamChange, UI...
```

**Problems:**
1. **No parameter versioning** - adding/removing params breaks state
2. **Thread safety ambiguity** - `IParam::mValue` is atomic, but `GetParam()` uses non-thread-safe `WDL_PtrList`
3. **Weak typing** - all params accessed by integer index, easy to use wrong index
4. **No grouping API** - groups are string-based afterthought

### 1.3 WDL Dependency Lock-in

Heavy reliance on Cockos WDL types throughout:
- `WDL_PtrList<T>` instead of `std::vector<std::unique_ptr<T>>`
- `WDL_String` instead of `std::string`
- `WDL_TypedBuf` instead of `std::vector<T>`
- `WDL_Mutex` instead of `std::mutex`

**Impact:**
- Cannot use standard C++ algorithms directly
- Harder for new contributors familiar with STL
- Testing frameworks expect STL types

---

## Part 2: IGraphics Architecture Analysis

### 2.1 Monolithic Class Sizes

| Class | Lines (h+cpp) | Single Responsibility? |
|-------|--------------|----------------------|
| IGraphics | ~4,850 | ❌ Drawing + Events + Resources + Platform |
| IControl | ~3,500 | ❌ State + Rendering + Events + Animation |
| IParam | ~600 | ⚠️ Mostly OK but includes display logic |

**IGraphics responsibilities (should be separate):**
1. Control management (`mControls`)
2. Event routing (mouse, keyboard, touch)
3. Drawing API abstraction
4. Resource caching (bitmaps, fonts)
5. Layer/framebuffer management
6. Platform windowing bridge
7. Popup menus and dialogs

### 2.2 No MVC/MVVM Pattern

Controls directly manage both state AND presentation:

```cpp
class IVKnobControl : public IKnobControlBase, public IVectorBase {
  void Draw(IGraphics& g) override {
    // Directly reads mValue and draws
    DrawIndicatorTrack(g, mAngle1, mAngle2, ...);
  }
  void OnMouseDrag(...) override {
    // Directly modifies mValue
    SetValue(GetValue() + delta);
  }
};
```

**Problems:**
1. **Cannot test logic without rendering** - no separation
2. **Cannot have multiple views of same model** - tied together
3. **Difficult to implement undo/redo** - no command pattern
4. **Animation state mixed with model state**

### 2.3 Manual Parameter Binding

Developers must manually wire parameters:

```cpp
// In constructor - manual binding
pGraphics->AttachControl(new IVKnobControl(bounds, kGain));

// In control - manual notification
void SetDirty(bool triggerAction = true, int valIdx = kNoValIdx) {
  if (triggerAction) {
    // Must remember to call this
    GetDelegate()->SendParameterValueFromUI(paramIdx, value);
  }
}
```

**Contrast with declarative binding:**
```cpp
// Hypothetical improved API
pGraphics->Bind<IVKnobControl>(bounds, param(kGain));  // Auto-wired
```

---

## Part 3: Plugin Implementation Analysis

### 3.1 config.h Boilerplate Problem

Analyzed 20 examples: **~90% of config.h content is duplicated**.

**Always identical (could be defaults):**
```cpp
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64
```

**Mostly identical (template pattern):**
```cpp
#define AUV2_ENTRY PluginName_Entry
#define AUV2_ENTRY_STR "PluginName_Entry"
#define AUV2_FACTORY PluginName_Factory
// Could be auto-generated from PLUG_CLASS_NAME
```

**Recommendation:** Reduce to ~10 essential defines, auto-generate the rest.

### 3.2 Constructor Ceremony

Every plugin repeats this pattern:

```cpp
MyPlugin::MyPlugin(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPresets))
{
  // Parameter init - always same pattern
  GetParam(kGain)->InitDouble("Gain", 0., -70., 12., 0.1, "dB");
  GetParam(kPan)->InitDouble("Pan", 0., -100., 100., 1., "%");
  // ... repeat for all params

  // Graphics setup - always same pattern
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
  };

  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(...);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    // ... control attachment
  };
}
```

**~30 lines of boilerplate** before any plugin-specific code.

### 3.3 ISender Pattern Verbosity

Audio-to-UI communication requires manual setup:

```cpp
// Declaration
IPeakAvgSender<2> mMeterSender;

// In ProcessBlock
mMeterSender.ProcessBlock(outputs, nFrames, kCtrlTagMeter);

// In OnIdle
mMeterSender.TransmitData(*this);

// In layout
pGraphics->AttachControl(new IVMeterControl<2>(bounds, ""), kCtrlTagMeter);
```

**Four separate locations** must be kept in sync.

---

## Part 4: Testability Assessment

### 4.1 Current State: Untestable

**Barriers to unit testing:**

1. **No dependency injection** - classes create their own dependencies
2. **Virtual method coupling** - can't mock without full inheritance
3. **Platform code in headers** - `#ifdef __APPLE__` scattered throughout
4. **No mock plugin format** - must use real VST3/AU to test
5. **IControl requires IGraphics** - circular dependency

**Example - cannot test IControl logic:**
```cpp
// This is impossible today:
TEST(KnobControl, ValueClampingWorks) {
  MockGraphics graphics;  // Doesn't exist
  IVKnobControl knob(IRECT(), kGain);
  knob.SetValue(1.5);  // Needs IGraphics context
  EXPECT_EQ(knob.GetValue(), 1.0);
}
```

### 4.2 What Would Be Needed

1. **Interface extraction** - `IGraphicsContext` interface for mocking
2. **Dependency injection** - pass dependencies to constructors
3. **Mock plugin API** - test harness that simulates host
4. **Separate model from view** - test control logic independently

---

## Part 5: Modernization Roadmap

### Phase 1: Non-Breaking Improvements (Low Risk)

#### 1.1 Introduce Optional STL Alternatives
```cpp
// New header: IPlugSTL.h
#ifdef IPLUG_USE_STL
  template<typename T> using IPlugVector = std::vector<T>;
  using IPlugString = std::string;
#else
  template<typename T> using IPlugVector = WDL_TypedBuf<T>;
  using IPlugString = WDL_String;
#endif
```

#### 1.2 Parameter Builder Pattern
```cpp
// New: Declarative parameter definition
static constexpr auto kParams = ParamList{
  Param::Double("Gain", 0., -70., 12.).Unit(dB).Group("Master"),
  Param::Bool("Bypass", false).Flags(kCannotAutomate),
  Param::Enum<LFOShape>("Shape", kTriangle).Group("LFO")
};
```

#### 1.3 Config Generation Script
```yaml
# plugin.yaml - single source of truth
name: MyPlugin
manufacturer: AcmeInc
version: 1.0.0
unique_id: 'Mypl'
type: effect
channels: "1-1 2-2"
```
```bash
# Auto-generates config.h
python scripts/generate_config.py plugin.yaml > config.h
```

### Phase 2: Architectural Refactoring (Medium Risk)

#### 2.1 Extract Interfaces for Testability
```cpp
// New: IParameterStore interface
class IParameterStore {
public:
  virtual int NParams() const = 0;
  virtual IParam* GetParam(int idx) = 0;
  virtual void SetParameterValue(int idx, double value) = 0;
};

// IControl now depends on interface, not concrete class
class IControl {
  IParameterStore* mParams;  // Injected, mockable
};
```

#### 2.2 Split IGraphics Responsibilities
```cpp
// Separate concerns into focused classes
class IGraphicsEventRouter { /* mouse/keyboard dispatch */ };
class IGraphicsResourceCache { /* bitmap/font caching */ };
class IGraphicsLayerManager { /* framebuffer stack */ };
class IGraphicsDrawContext { /* abstract drawing API */ };

// IGraphics becomes a facade
class IGraphics : public IGraphicsEventRouter,
                  public IGraphicsResourceCache,
                  public IGraphicsLayerManager {
  IGraphicsDrawContext* mDrawContext;  // NanoVG, Skia, etc.
};
```

#### 2.3 Introduce Control Model Pattern
```cpp
// Separate model from view
class IControlModel {
  double mValue;
  std::function<void(double)> mOnChanged;
public:
  void SetValue(double v) {
    mValue = std::clamp(v, 0.0, 1.0);
    mOnChanged(mValue);
  }
};

class IKnobView : public IControl {
  IControlModel* mModel;  // Injected
  void Draw(IGraphics& g) override {
    // Pure rendering, no logic
  }
};
```

### Phase 3: New Architecture (Longer Term)

#### 3.1 Plugin Descriptor Pattern
```cpp
// New: Declarative plugin definition
struct MyPluginDescriptor {
  static constexpr auto Name = "MyPlugin";
  static constexpr auto Params = ParamList{...};
  static constexpr auto Channels = "1-1 2-2";

  // Type-safe process function
  void Process(AudioBuffer<float>& buffer, const ParamValues& params) {
    float gain = params.Get<kGain>();
    // ...
  }
};

// Framework generates all format wrappers
using MyPlugin = Plugin<MyPluginDescriptor>;
```

#### 3.2 Reactive UI System
```cpp
// Inspired by modern UI frameworks
class MyPluginUI : public IGraphicsUI {
  void Layout(IGraphicsBuilder& ui) override {
    ui.VStack({
      ui.Knob(param(kGain)).Label("Gain"),
      ui.Knob(param(kPan)).Label("Pan"),
      ui.Meter().Source(sender(kMeter))
    });
  }
};
```

#### 3.3 Mock Plugin Format for Testing
```cpp
// New: IPlugMock for unit testing
class IPlugMock : public IPlugAPIBase, public IPlugProcessor {
public:
  // Simulate host parameter changes
  void SimulateHostParamChange(int idx, double value);

  // Capture outgoing host notifications
  std::vector<ParamChange> GetParamChanges();

  // Process test buffers
  void ProcessTestBuffer(std::vector<float>& buffer);
};

TEST(MyPlugin, GainParameterWorks) {
  IPlugMock mock;
  MyPlugin plugin(mock.GetInfo());

  mock.SimulateHostParamChange(kGain, 0.5);
  mock.ProcessTestBuffer(buffer);

  // Verify gain was applied
  EXPECT_NEAR(buffer[0], 0.5f, 0.001f);
}
```

---

## Part 6: Specific Recommendations

### High Priority (Do First)

1. **Create `IPlugSTL.h`** - optional STL types, zero breaking changes
2. **Write config generator script** - Python, generates from YAML
3. **Add parameter validation** - runtime checks for index bounds
4. **Document threading model** - which methods are audio-safe

### Medium Priority

5. **Extract `IParameterStore` interface** - enables mocking
6. **Create `IPlugMock` test harness** - enables unit testing
7. **Split `IGraphics` into components** - better separation
8. **Add parameter versioning** - state migration support

### Lower Priority (Future)

9. **Reactive UI builder** - declarative control creation
10. **Type-safe parameters** - compile-time parameter validation
11. **Plugin descriptor pattern** - minimize boilerplate

---

## Part 7: Preserving Plugin Wrapper Robustness

The plugin wrappers (VST3, AU, CLAP, AAX) are iPlug2's strongest asset. Any modernization must:

1. **Keep wrappers as-is initially** - they work, don't break them
2. **Add abstraction layers above** - new code calls existing infrastructure
3. **Maintain ABI stability** - plugin state format must remain compatible
4. **Test against real hosts** - CI should test all formats

**Suggested approach:**
```
[New Declarative API]
        │
        ▼
[Adapter Layer] ──────► [Existing IPlugAPIBase/IPlugProcessor]
                                │
                                ▼
                        [Proven Plugin Wrappers]
```

The adapter layer translates new patterns to existing infrastructure, allowing gradual migration without touching working wrapper code.

---

## Appendix: Code Metrics

### Lines of Code by Component

| Component | Header | Implementation | Total |
|-----------|--------|---------------|-------|
| IPlug Core | ~3,200 | ~2,800 | ~6,000 |
| IGraphics Core | ~4,200 | ~3,600 | ~7,800 |
| IControl | ~2,300 | ~1,200 | ~3,500 |
| VST3 Wrapper | ~2,400 | ~3,100 | ~5,500 |
| CLAP Wrapper | ~800 | ~1,200 | ~2,000 |
| AU Wrapper | ~1,600 | ~2,800 | ~4,400 |

### Inheritance Depth

| Class | Depth | Multiple Inheritance |
|-------|-------|---------------------|
| IPlugVST3 | 5 | Yes (3 bases) |
| IPlugCLAP | 4 | Yes (2 bases) |
| IPlugAU | 4 | No |
| IVKnobControl | 4 | Yes (2 mixins) |

### Friend Class Count

| Class | Friend Count |
|-------|-------------|
| IPlugAPIBase | 11 |
| IPlugProcessor | 8 |
| IGraphics | 3 |

---

## Conclusion

iPlug2 has a solid foundation that successfully delivers on its cross-platform promise. The plugin wrappers are battle-tested and reliable. However, to compete with JUCE professionally, focus should be on:

1. **Reducing boilerplate** through code generation and better defaults
2. **Improving testability** through interface extraction and DI
3. **Separating concerns** in monolithic classes like IGraphics
4. **Modernizing the parameter system** with type safety and versioning

The key is **additive modernization** - introduce new patterns alongside existing code rather than rewriting. This preserves stability while enabling gradual improvement.
