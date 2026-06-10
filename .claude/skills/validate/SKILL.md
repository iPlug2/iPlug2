---
name: validate
description: Validate iPlug2 plugin builds using format-specific validators (auval, pluginval, vstvalidator, clap-validator) (project)
---

# Validate iPlug2 Plugin

Validate a built plugin using format-specific validation tools. Plugin must have been built prior to using this skill.

## Arguments

- `PROJECT_NAME` (required): Name of the project (e.g., `IPlugEffect`)
- `FORMAT` (optional): Specific format to validate (`AU`, `AUv3`, `VST3`, `CLAP`, `all`). Default: `all`

## Validation Workflow

### Step 1: Read config.h to extract plugin identifiers

Read `Examples/[PROJECT_NAME]/config.h` or `Tests/[PROJECT_NAME]/config.h` and extract:
- `PLUG_NAME` - Plugin display name
- `PLUG_UNIQUE_ID` - 4-char subtype (e.g., `'Ipef'` → `Ipef`)
- `PLUG_MFR_ID` - 4-char manufacturer code (e.g., `'Acme'` → `Acme`)
- `PLUG_MFR` - Manufacturer display name
- `PLUG_TYPE` - 0=effect (aufx), 1=instrument (aumu), 2=MIDI effect (aumf)

### Step 2: Determine AU type from PLUG_TYPE

```
PLUG_TYPE 0 → aufx
PLUG_TYPE 1 → aumu
PLUG_TYPE 2 → aumf
```

### Step 3: Validate each format

#### AUv2 (macOS) - auval

```bash
# Restart audio server first to pick up changes
sudo killall -9 AudioComponentRegistrar 2>/dev/null; sleep 1

# Run validation
auval -v [AU_TYPE] [PLUG_UNIQUE_ID] [PLUG_MFR_ID]
```

Example: `auval -v aufx Ipef Acme`

Look for `AU VALIDATION SUCCEEDED` at the end.

**Useful auval options:**
- `-strict` - Enforce strict checks (recommended for release)
- `-r N` - Repeat validation N times
- `-o` - Quick open/init test only (faster debugging)
- `-q` - Quiet mode (errors/warnings only)

#### AUv3 (macOS) - auval

**Note:** The host app must be built codesigned and launched at least once to register the AUv3 extension. AUv2 plugin should be removed to avoid conflict. Auval usage the same as AUv2, but should see "This AudioUnit is a version 3 implementation." 

#### VST3 - vstvalidator

**Build vstvalidator if not present:**
```bash
# macOS/Linux
cd Dependencies/IPlug && ./download-vst3-sdk.sh master build-validator

# Windows (use Git Bash or WSL)
cd Dependencies/IPlug && ./download-vst3-sdk.sh master build-validator
```

The validator binary will be at `Dependencies/IPlug/VST3_SDK/validator` (or `validator.exe` on Windows).

**Run validation:**
```bash
# macOS
Dependencies/IPlug/VST3_SDK/validator ~/Library/Audio/Plug-Ins/VST3/[PLUG_NAME].vst3

# Windows
Dependencies\IPlug\VST3_SDK\validator.exe "C:\Program Files\Common Files\VST3\[PLUG_NAME].vst3"
```

#### CLAP - clap-validator

**Install clap-validator:**
Download from https://github.com/free-audio/clap-validator/releases

**Run validation:**
```bash
# macOS
clap-validator validate ~/Library/Audio/Plug-Ins/CLAP/[PLUG_NAME].clap

# Windows
clap-validator.exe validate "C:\Program Files\Common Files\CLAP\[PLUG_NAME].clap"
```

Look for `X tests run, Y passed, 0 failed` at the end.

**Useful options:**
- `--in-process` - Run in-process (faster)
- `--only-failed` - Show only failed tests

#### Multi-format: pluginval

```bash
# macOS
brew install --cask pluginval
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 --validate ~/Library/Audio/Plug-Ins/VST3/[PLUG_NAME].vst3

# Windows (download from https://github.com/Tracktion/pluginval/releases)
pluginval.exe --strictness-level 5 --validate "C:\Program Files\Common Files\VST3\[PLUG_NAME].vst3"
```

## Plugin Locations

| Format | macOS | Windows |
|--------|-------|---------|
| AUv2 | `~/Library/Audio/Plug-Ins/Components/[NAME].component` | N/A |
| AUv3 | `~/Applications/[NAME].app/Contents/PlugIns/[NAME].appex` | N/A |
| VST3 | `~/Library/Audio/Plug-Ins/VST3/[NAME].vst3` | `C:\Program Files\Common Files\VST3\[NAME].vst3` |
| CLAP | `~/Library/Audio/Plug-Ins/CLAP/[NAME].clap` | `C:\Program Files\Common Files\CLAP\[NAME].clap` |

## Expected Output

Report validation results in a table:

| Format | Status | Details |
|--------|--------|---------|
| AUv2 | PASS/FAIL | Brief summary |
| AUv3 | PASS/FAIL/SKIP | Reason if skipped |
| VST3 | PASS/FAIL/SKIP | Reason if skipped |
| CLAP | PASS/FAIL/SKIP | Reason if skipped |
