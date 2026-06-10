---
name: duplicate-example
description: Create a new iPlug2 plugin project by duplicating an example template
---

# Clone an iPlug2 Example

Use this skill when the user wants to create a new plugin project from an iPlug2 example.

## Workflow

1. **Ask for project details:**
   - **Plugin name** (required): No spaces or special characters
   - **Manufacturer name** (required): Default to "AcmeInc" if not provided
   - **Base template**: Which example to clone from

2. **Available templates:**
   | Template | Description |
   |----------|-------------|
   | IPlugEffect | Basic audio effect (volume control) - **recommended for effects** |
   | IPlugInstrument | MPE-capable polyphonic synth - **recommended for instruments** |
   | IPlugControls | Widget demonstration |
   | IPlugWebUI | HTML/CSS/JS UI via WebView |
   | IPlugSwiftUI | SwiftUI for macOS/iOS |
   | IPlugSvelteUI | Svelte-based UI |

3. **Run the duplicate script:**
   ```bash
   cd Examples
   ./duplicate.py [SourceExample] [NewPluginName] [ManufacturerName]
   ```

4. **Customize config.h** (offer to user):
   - `PLUG_UNIQUE_ID` - Auto-generated 4-char ID (verify it's unique)
   - `PLUG_MFR_ID` - 4-char manufacturer ID
   - Copyright, email, URLs - prompt but allow skipping

5. **Warn about sensitive settings:**
   - Don't change `BUNDLE_NAME` without updating plist files
   - Keep config.h free of `#include` statements

## Example

```bash
cd Examples
./duplicate.py IPlugEffect MyGainPlugin MyCompany
```

Creates `Examples/MyGainPlugin/` with all project files renamed and configured.
