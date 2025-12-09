# iPlug2 CLI Documentation

## Overview

The iPlug2 CLI target provides a headless command-line interface for offline audio processing. It enables programmatic testing and batch processing of plugins without requiring a GUI or DAW.

**Key Features:**
- Process audio files through effects plugins
- Generate audio from synthesizer plugins via MIDI
- Query and manipulate plugin parameters
- Save/load plugin state (presets)
- Generate impulse responses for analysis
- JSON output for automation and scripting

## Building

Add the CLI target to your plugin's `CMakeLists.txt`:

```cmake
# CLI Target (Command Line Interface) - NO IGraphics, for offline processing
add_executable(${PROJECT_NAME}-cli
  ${PROJECT_NAME}.cpp
  ${PROJECT_NAME}.h
  resources/resource.h
)
iplug_add_target(${PROJECT_NAME}-cli PUBLIC
  INCLUDE ${PROJECT_DIR} ${PROJECT_DIR}/resources
  LINK iPlug2::CLI
)
iplug_configure_cli(${PROJECT_NAME}-cli ${PROJECT_NAME})
```

Build with CMake:
```bash
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
ninja -C build MyPlugin-cli
```

The CLI executable will be at `build/out/MyPlugin`.

---

## Command Reference

### Information Commands

#### `--help`
Display usage information.

```bash
MyPlugin --help
```

#### `--info`
Print plugin information as JSON.

```bash
MyPlugin --info
```

Output:
```json
{
  "name": "MyPlugin",
  "version": "1.0.0",
  "manufacturer": "MyCompany",
  "type": "effect",
  "channels": {
    "inputs": 2,
    "outputs": 2
  },
  "latency": 0,
  "midi_in": false,
  "midi_out": false
}
```

#### `--params`
List all parameters as JSON.

```bash
MyPlugin --params
```

Output:
```json
{
  "parameters": [
    {
      "index": 0,
      "name": "Gain",
      "value": 0,
      "min": 0,
      "max": 100,
      "default": 0,
      "label": "%"
    }
  ]
}
```

#### `--param <index>`
Query a single parameter by index.

```bash
MyPlugin --param 0
```

---

### Configuration Commands

#### `--sr <rate>`
Set sample rate in Hz. Default: 44100 (or inherited from input file).

```bash
MyPlugin --sr 48000 --process 48000
```

#### `--bs <size>`
Set processing block size in samples. Default: 512.

```bash
MyPlugin --bs 1024 --process 44100
```

#### `--set <index> <value>`
Set a parameter by index to a specific value.

```bash
MyPlugin --set 0 75 --param 0
# Output: Set param 0 to 75
```

#### `--set-name <name> <value>`
Set a parameter by name.

```bash
MyPlugin --set-name "Gain" 75
# Output: Set param "Gain" (idx 0) to 75
```

#### `--set-norm <index> <normalized_value>`
Set a parameter using normalized value (0.0 to 1.0).

```bash
MyPlugin --set-norm 0 0.5
# Output: Set param 0 to normalized 0.5 (value: 50)
```

Multiple `--set` commands can be chained:
```bash
MyPlugin --set 0 50 --set 1 0.7 --set 2 100 --process-file
```

---

### State Management

#### `--save-params <file.json>`
Save current parameter values to a JSON file.

```bash
MyPlugin --set 0 75 --save-params preset.json
```

Output file:
```json
{
  "parameters": [
    {"index": 0, "name": "Gain", "value": 75}
  ]
}
```

#### `--load-params <file.json>`
Load parameter values from a JSON file.

```bash
MyPlugin --load-params preset.json --param 0
```

#### `--save-state <file.bin>`
Save full plugin state to a binary file. This includes all parameters plus any custom state data.

```bash
MyPlugin --set 0 75 --save-state preset.bin
```

#### `--load-state <file.bin>`
Load full plugin state from a binary file.

```bash
MyPlugin --load-state preset.bin --process-file
```

---

### File I/O

#### `--input <file.wav>`
Specify input WAV file for processing.

#### `--output <file.wav>`
Specify output WAV file (24-bit).

#### `--process-file`
Process the entire input file through the plugin.

```bash
MyPlugin --input dry.wav --output wet.wav --set 0 50 --process-file
```

Output:
```
Set param 0 to 50
Input: dry.wav (2 ch, 44100 Hz, 132300 frames)
Processed 132300 frames
Wrote wet.wav (2 ch, 44100 Hz, 132300 frames)
```

---

### Processing Commands

#### `--process <frames>`
Process N frames of silence through the plugin.

```bash
MyPlugin --process 44100 --output silence.wav
```

#### `--impulse <length>`
Generate an impulse response of specified length (default: 4096 samples).

```bash
MyPlugin --impulse 8192 --output ir.wav
```

#### `--render <milliseconds>`
Render N milliseconds of audio. Used with MIDI commands for synthesizers.

```bash
MyPlugin --midi 60 100 0 500 --render 1000 --output note.wav
```

---

### MIDI Commands

MIDI commands queue events that are processed when `--render` is called.

#### `--midi <note> <velocity> <start_ms> <duration_ms>`
Queue a MIDI note.

- `note`: MIDI note number (0-127, middle C = 60)
- `velocity`: Note velocity (0-127)
- `start_ms`: Start time in milliseconds
- `duration_ms`: Note duration in milliseconds

```bash
# Play C4 at velocity 100, starting at 0ms, lasting 500ms
MyPlugin --midi 60 100 0 500 --render 1000 --output note.wav
```

Multiple notes can be sequenced:
```bash
# Play a C major arpeggio
MyPlugin \
  --midi 60 100 0 200 \
  --midi 64 100 200 200 \
  --midi 67 100 400 200 \
  --render 1000 \
  --output arpeggio.wav
```

#### `--midi-cc <cc_number> <value> [time_ms]`
Queue a MIDI Control Change message.

- `cc_number`: CC number (0-127)
- `value`: CC value (0-127)
- `time_ms`: Time offset in milliseconds (default: 0)

```bash
# Modulation wheel sweep
MyPlugin \
  --midi-cc 1 0 0 \
  --midi-cc 1 64 250 \
  --midi-cc 1 127 500 \
  --midi 60 100 0 1000 \
  --render 1500 \
  --output mod_sweep.wav
```

#### `--midi-bend <value> [time_ms]`
Queue a pitch bend message.

- `value`: Pitch bend amount (-1.0 to 1.0)
- `time_ms`: Time offset in milliseconds (default: 0)

```bash
# Pitch bend up then down
MyPlugin \
  --midi 60 100 0 1000 \
  --midi-bend 0.5 200 \
  --midi-bend -0.5 500 \
  --midi-bend 0 800 \
  --render 1500 \
  --output bend.wav
```

#### `--midi-pc <program> [time_ms]`
Queue a program change message.

- `program`: Program number (0-127)
- `time_ms`: Time offset in milliseconds (default: 0)

```bash
MyPlugin --midi-pc 5 0 --midi 60 100 0 500 --render 1000 --output prog5.wav
```

---

### Text Output

#### `--output-txt <file>`
Write the first output channel as text (one sample per line). Useful for plotting or analysis.

```bash
MyPlugin --impulse 1024 --output-txt ir.txt
```

Output file format:
```
0.0
0.0
1.0
0.5
0.25
...
```

---

## Usage Examples

### Effect Plugin: Batch Processing

```bash
# Process multiple files with same settings
for f in *.wav; do
  MyEffect --input "$f" --output "processed_$f" --set 0 75 --process-file
done
```

### Effect Plugin: A/B Comparison

```bash
# Generate wet/dry comparison
MyEffect --input audio.wav --output dry.wav --set 0 0 --process-file
MyEffect --input audio.wav --output wet.wav --set 0 100 --process-file
```

### Effect Plugin: Impulse Response Capture

```bash
# Capture IR at different settings
for gain in 25 50 75 100; do
  MyEffect --set 0 $gain --impulse 4096 --output "ir_gain${gain}.wav"
done
```

### Synth Plugin: Render a Melody

```bash
MySynth \
  --midi 60 100 0 250 \
  --midi 62 100 250 250 \
  --midi 64 100 500 250 \
  --midi 65 100 750 250 \
  --midi 67 100 1000 500 \
  --render 2000 \
  --output melody.wav
```

### Synth Plugin: Test Different Presets

```bash
# Save current state as reference
MySynth --save-state default.bin

# Test with preset
MySynth --load-state preset1.bin --midi 60 100 0 500 --render 1000 --output preset1.wav
MySynth --load-state preset2.bin --midi 60 100 0 500 --render 1000 --output preset2.wav
```

### Automation: Query Parameters

```bash
# Get all parameters as JSON for scripting
params=$(MyPlugin --params)
echo "$params" | jq '.parameters[] | select(.name == "Gain") | .index'
```

### Automation: Parameter Sweep

```bash
# Sweep a parameter and capture output
for val in $(seq 0 10 100); do
  MyEffect --set 0 $val --impulse 2048 --output-txt "sweep_${val}.txt"
done
```

---

## Exit Codes

- `0`: Success
- `1`: Error (invalid arguments, file not found, processing failure)

---

## Notes

- WAV output is always 24-bit
- Sample rate defaults to 44100 Hz or is inherited from input file
- MIDI events are queued and processed together with `--render`
- Status messages are written to stderr, JSON output to stdout
- State files (`.bin`) are binary and compatible with host preset formats
- Parameter files (`.json`) are human-readable but only store parameter values
