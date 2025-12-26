# AUv3 Sample-Accurate MIDI Timing Test

A command-line tool to verify that AUv3 plugins correctly handle sample-accurate MIDI timing.

## Purpose

This test verifies the changes in commit `b1f552ec163ecccded6852d8d60eb33b529422eb` which implements sample-accurate MIDI timing for AUv3 plugins.

### Before the fix
All MIDI events were processed at sample 0 of each buffer, regardless of their scheduled timestamp. This introduced up to one buffer of timing jitter (~5ms at 256 samples/48kHz).

### After the fix
MIDI events are processed at their exact sample position using buffer-splitting. The audio buffer is split around events, and each segment is processed with events applied at their precise sample positions.

## Prerequisites

1. **Build IPlugInstrument AUv3**:
   ```bash
   # From the iPlug2 root directory
   cd Examples/IPlugInstrument/projects
   xcodebuild -project IPlugInstrument-macOS.xcodeproj \
              -scheme "macOS-APP" \
              -configuration Debug \
              build
   ```

2. **Register the AUv3** (may happen automatically, or run the app once)

## Build

```bash
cd Tests/AUv3TimingTest
swift build -c release
```

## Run

```bash
# Basic test
.build/release/AUv3TimingTest

# Verbose output with buffer dumps
.build/release/AUv3TimingTest --verbose

# Custom test offsets
.build/release/AUv3TimingTest --offsets 0,50,100,200,300

# Custom buffer size
.build/release/AUv3TimingTest --buffer-size 1024 --offsets 0,256,512,768
```

## Options

| Option | Description | Default |
|--------|-------------|---------|
| `--verbose, -v` | Show detailed output including buffer dumps | off |
| `--offsets <list>` | Comma-separated list of sample offsets to test | 50,100,256,400 |
| `--sample-rate <rate>` | Audio sample rate in Hz | 44100 |
| `--buffer-size <size>` | Buffer size in samples | 512 |
| `--tolerance <samples>` | Allowed timing variance | 2 |
| `--help, -h` | Show help message | - |

**Note:** Avoid testing offset 0 on the first render after instantiation, as there may be minor initialization latency. The default offsets start at 50.

## Expected Output

### With sample-accurate timing (after fix):
```
Test 1: MIDI scheduled at sample 100
  Expected first audio at: sample 100
  Detected first audio at: sample 100
  Delta: 0 samples
  Result: PASS

Overall Result: PASS
```

### Without sample-accurate timing (before fix):
```
Test 1: MIDI scheduled at sample 100
  Expected first audio at: sample 100
  Detected first audio at: sample 0
  Delta: -100 samples
  Result: FAIL

Overall Result: FAIL
```

## How It Works

1. **Loads IPlugInstrument AUv3** via `AVAudioUnit.instantiate()`
2. **Enables offline rendering** using `AVAudioEngine.enableManualRenderingMode()`
3. **Schedules MIDI note-on** at a specific sample offset using `scheduleMIDIEventBlock`
4. **Renders the buffer** offline
5. **Analyzes output** to find the first non-silent sample
6. **Compares** the detected audio start position to the scheduled MIDI position

## Testing the Fix

To compare before/after behavior:

```bash
# Test with the fix
git checkout auv3/sample-accurate
# Build IPlugInstrument AUv3
# Run test - should PASS

# Test without the fix
git checkout auv3/sample-accurate~1  # One commit before
# Rebuild IPlugInstrument AUv3
# Run test - should FAIL
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | All tests passed |
| 1 | One or more tests failed |
| 2 | Error (plugin not found, etc.) |
