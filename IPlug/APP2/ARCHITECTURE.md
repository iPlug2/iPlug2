# IPlug2 APP2 - Modernized Standalone App Wrapper

## Overview

APP2 is a complete rewrite of the iPlug2 standalone application wrapper, designed for
clean separation of concerns, flexible audio I/O that respects plugin configurations,
and extensibility for custom UIs (including WebView-based settings).

## Design Goals

1. **Clean Code Architecture**: Separate platform code from core logic
2. **Flexible Audio I/O**: Respect `PLUG_CHANNEL_IO` from config.h
3. **Level Visualization**: Real-time input/output metering in settings
4. **Extensible Settings UI**: Support SWELL dialogs, IGraphics overlays, or WebViews
5. **Reliable Device Handling**: Graceful fallbacks for mono/asymmetric devices
6. **Cross-Platform**: Windows, macOS, Linux with clean platform abstractions

## Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         IPlugAPP2Host                                │
│  (Orchestrates all components, owns plugin instance)                │
└─────────────────────────────────────────────────────────────────────┘
         │              │                │               │
         ▼              ▼                ▼               ▼
┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│ AudioEngine │  │ MidiEngine  │  │   AppState  │  │   Settings  │
│             │  │             │  │   Manager   │  │   Dialog    │
└─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘
         │              │                │               │
         ▼              ▼                ▼               ▼
┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐
│  RTAudio    │  │  RTMidi     │  │  INI File   │  │ SWELL/IGraph│
│  Backend    │  │  Backend    │  │  Persistence│  │ /WebView    │
└─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘
```

## File Structure

```
IPlug/APP2/
├── IPlugAPP2.h                    # Main plugin wrapper class
├── IPlugAPP2.cpp
├── IPlugAPP2_Host.h               # Host orchestration
├── IPlugAPP2_Host.cpp
│
├── Audio/
│   ├── AudioEngine.h              # Audio engine interface
│   ├── AudioEngine.cpp            # RTAudio-based implementation
│   ├── AudioDeviceInfo.h          # Device enumeration structs
│   └── ChannelMapping.h           # Flexible channel routing
│
├── Midi/
│   ├── MidiEngine.h               # MIDI engine interface
│   └── MidiEngine.cpp             # RTMidi-based implementation
│
├── State/
│   ├── AppState.h                 # Configuration state with channel maps
│   ├── AppState.cpp
│   └── StateSerializer.h          # INI file persistence
│
├── Settings/
│   ├── ISettingsDialog.h          # Abstract settings dialog interface
│   ├── SettingsDialogSWELL.h      # SWELL implementation
│   ├── SettingsDialogSWELL.cpp
│   └── LevelMeter.h               # Audio level metering component
│
├── Platform/
│   ├── PlatformUtils.h            # Platform detection, paths
│   ├── PlatformUtils_Win.cpp
│   ├── PlatformUtils_Mac.cpp
│   └── PlatformUtils_Linux.cpp
│
└── Main/
    ├── Main_Win.cpp               # Windows entry point
    ├── Main_Mac.cpp               # macOS entry point
    └── Main_Linux.cpp             # Linux entry point
```

## Key Design Decisions

### 1. Flexible Channel I/O

The current APP wrapper only supports stereo L/R pair selection. APP2 will:

- Parse `PLUG_CHANNEL_IO` to understand all valid I/O configurations
- Allow selection of which config to use (e.g., "2-2" vs "6-6")
- Map plugin channels to device channels with full flexibility
- Handle mono inputs gracefully (duplicate to stereo if needed)
- Support instruments (0 inputs) without showing input controls

```cpp
// Example: PLUG_CHANNEL_IO "0-2 0-2.2 0-2.2.2.2" (IPlugDrumSynth)
// User can select which bus configuration to use
// Output channels mapped to available device channels

struct ChannelMapping {
  int pluginChannel;     // Channel index in plugin
  int deviceChannel;     // Channel index on audio device
  bool enabled;          // Can disable unused channels
};
```

### 2. Audio Level Metering

Settings dialog will include visual level meters for both input and output.
This helps users verify their audio routing is correct.

```cpp
class LevelMeter {
public:
  void SetLevel(float peakdB, float rmsdB);
  void Paint(HDC hdc, RECT& bounds);  // GDI rendering for SWELL

private:
  std::atomic<float> mPeakLevel{-96.f};
  std::atomic<float> mRmsLevel{-96.f};
};
```

The audio callback will compute levels and post them to the UI thread:

```cpp
// In audio callback (lock-free)
for (int ch = 0; ch < nInputChannels; ch++) {
  float peak = ComputePeak(inputBuffer[ch], nFrames);
  mInputLevels[ch].store(peak, std::memory_order_relaxed);
}
```

### 3. Settings Dialog Abstraction

```cpp
class ISettingsDialog {
public:
  virtual ~ISettingsDialog() = default;

  // Show the dialog (modal or modeless)
  virtual bool Show(void* parentWindow) = 0;

  // Update displayed values from state
  virtual void RefreshFromState(const AppState& state) = 0;

  // Get level meters for real-time updates
  virtual LevelMeter* GetInputMeter(int channel) = 0;
  virtual LevelMeter* GetOutputMeter(int channel) = 0;

  // Callbacks
  std::function<void(const AppState&)> OnApply;
  std::function<void()> OnCancel;
};
```

This allows future implementations using IGraphics or WebView without
changing the host code.

### 4. Robust Audio Initialization

Current issues to fix:
- Mono input device causing complete failure
- No fallback when configured device unavailable

```cpp
class AudioEngine {
public:
  enum class InitResult {
    Success,
    FallbackUsed,      // Started with fallback device
    PartialChannels,   // Requested channels unavailable, using subset
    Failed
  };

  InitResult Initialize(const AudioConfig& config);

  // Detailed error info
  std::string GetLastError() const;

  // Query actual running config (may differ from requested)
  AudioConfig GetActiveConfig() const;
};
```

### 5. AppState Structure

```cpp
struct AppState {
  // Audio settings
  std::string audioDriverType;  // "CoreAudio", "ASIO", "DirectSound", etc.
  std::string inputDeviceName;
  std::string outputDeviceName;
  uint32_t sampleRate = 44100;
  uint32_t bufferSize = 512;

  // Flexible channel mapping
  int selectedIOConfig = 0;  // Index into PLUG_CHANNEL_IO options
  std::vector<ChannelMapping> inputMappings;
  std::vector<ChannelMapping> outputMappings;

  // MIDI settings
  std::string midiInputDevice;
  std::string midiOutputDevice;
  int midiInputChannel = 0;   // 0 = all
  int midiOutputChannel = 0;

  // Serialization
  bool LoadFromINI(const std::string& path);
  bool SaveToINI(const std::string& path) const;
};
```

## Implementation Phases

### Phase 1: Core Framework (Current)
- [x] Architecture document
- [ ] Directory structure and header files
- [ ] AudioEngine with RTAudio backend
- [ ] Basic AppState and serialization
- [ ] Minimal SWELL settings dialog

### Phase 2: Channel Flexibility
- [ ] Parse PLUG_CHANNEL_IO configurations
- [ ] Channel mapping UI in settings
- [ ] Mono input handling
- [ ] Multi-output bus support

### Phase 3: Level Metering
- [ ] Lock-free level computation in audio callback
- [ ] GDI-based level meters for SWELL dialog
- [ ] Timer-based UI refresh

### Phase 4: Platform Polish
- [ ] Windows-specific features (ASIO control panel)
- [ ] macOS-specific features (virtual MIDI)
- [ ] Linux implementation
- [ ] DPI awareness

### Phase 5: Extended UI Options
- [ ] IGraphics-based settings panel
- [ ] WebView-based settings (optional)

## Compatibility

APP2 will coexist with the original APP wrapper during transition.
Plugins can opt-in via config.h:

```cpp
#define APP_USE_APP2 1  // Use new APP2 wrapper
```

## Testing Strategy

1. **IPlugEffect** - Basic stereo effect (2-2)
2. **IPlugInstrument** - Instrument with no inputs (0-2)
3. **IPlugDrumSynth** - Multi-bus output (0-2.2.2.2)
4. **IPlugSurroundEffect** - Multi-channel (up to 12-12)
5. **Device stress tests** - Mono devices, device removal, etc.
