---
name: setup-deps
description: Download iPlug2 dependencies including plugin format SDKs (VST3, CLAP, WAM) and optional Skia graphics backend libraries
---

# Setup iPlug2 Dependencies

Use this skill when the user needs to download SDKs or prebuilt libraries for iPlug2.

## Workflow

1. **Ask what the user needs:**
   - Plugin format SDKs (VST3, CLAP, WAM) - required for most plugin formats
   - Skia prebuilt libs - required if using Skia IGraphics backend
   - iOS libs - only if targeting iOS

2. **Download plugin SDKs** (if requested):
   ```bash
   cd ./Dependencies/IPlug/
   ./download-iplug-sdks.sh
   ```

3. **Download Skia libs** (if requested):
   ```bash
   cd ./Dependencies/
   ./download-prebuilt-libs.sh
   ```
   For iOS (macOS only):
   ```bash
   ./download-prebuilt-libs.sh ios
   ```

4. **Inform the user about manual SDKs:**
   - VST2 SDK: No longer publicly available, not recommended for new projects
   - AAX SDK: Must be downloaded manually from Avid's developer portal

## Notes

- AUv2/AUv3 require no additional SDKs (macOS-only formats)
- Standalone apps require no additional SDKs
- Always return to repo root after running scripts
