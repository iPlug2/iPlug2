# iPlug2 Project Structure

This document describes the modern structure of an iPlug2 project using `Examples/IPlugEffect` as a reference. Each iPlug2 project follows a standardized folder layout that supports cross-platform development and multiple plugin formats.

## Project Root Files

| File | Description |
|------|-------------|
| `IPlugEffect.cpp` | Main plugin implementation source file containing the plugin logic and audio processing code |
| `IPlugEffect.h` | Plugin class header file with parameter definitions, class declaration, and public interface |
| `config.h` | Plugin configuration header defining plugin metadata, capabilities, and build settings |
| `README.md` | Project description and basic information |
| `IPlugEffect.sln` | Visual Studio solution file for Windows builds (all plugin formats) |
| `IPlugEffect.code-workspace` | VS Code workspace configuration file |
| `IPlugEffect.RPP` | REAPER project file for testing the plugin |
| `IPlugEffect.xcworkspace/` | Xcode workspace for macOS/iOS development (contains multiple projects) |

## Configuration Files (`config/`)

| File | Description |
|------|-------------|
| `IPlugEffect-mac.xcconfig` | Xcode configuration file for macOS builds, includes common settings from `../../common-mac.xcconfig` |
| `IPlugEffect-ios.xcconfig` | Xcode configuration file for iOS builds |
| `IPlugEffect-win.props` | Visual Studio property sheet for Windows builds, includes common settings from `../../common-win.props` |
| `IPlugEffect-web.mk` | Makefile configuration for Web Audio Module (WAM) builds using Emscripten |

## Project Files (`projects/`)

### Windows Visual Studio Projects
| File | Description |
|------|-------------|
| `IPlugEffect-app.vcxproj` | Standalone application project for Windows |
| `IPlugEffect-vst2.vcxproj` | VST2 plugin project for Windows |
| `IPlugEffect-vst3.vcxproj` | VST3 plugin project for Windows |
| `IPlugEffect-clap.vcxproj` | CLAP plugin project for Windows |
| `IPlugEffect-aax.vcxproj` | AAX plugin project for Windows |
| `*.vcxproj.filters` | Visual Studio filter files (auto-generated, defines source file organization in IDE) |
| `*.vcxproj.user` | Visual Studio user settings files (auto-generated, contains local build preferences) |

### macOS Xcode Projects
| File | Description |
|------|-------------|
| `IPlugEffect-macOS.xcodeproj/` | Main Xcode project for macOS builds (APP, VST2, VST3, AU, CLAP, AAX targets) |
| `IPlugEffect-iOS.xcodeproj/` | Xcode project for iOS builds (AUv3 and standalone app) |
| `IPlugEffect-macOS.entitlements` | macOS app entitlements file for sandboxing and security |
| `IPlugEffect-iOS.entitlements` | iOS app entitlements file for App Store distribution |

### Web Audio Module (WAM)
| File | Description |
|------|-------------|
| `IPlugEffect-wam-controller.mk` | Makefile for WAM controller build (UI) |
| `IPlugEffect-wam-processor.mk` | Makefile for WAM processor build (AudioWorklet) |

## Resource Files (`resources/`)

### Platform-Specific Resources
| File | Description |
|------|-------------|
| `IPlugEffect-macOS-Info.plist` | macOS standalone app bundle Info.plist |
| `IPlugEffect-iOS-Info.plist` | iOS standalone app bundle Info.plist |
| `IPlugEffect-AU-Info.plist` | Audio Unit plugin Info.plist (macOS) |
| `IPlugEffect-VST2-Info.plist` | VST2 plugin bundle Info.plist (macOS) |
| `IPlugEffect-VST3-Info.plist` | VST3 plugin bundle Info.plist (macOS) |
| `IPlugEffect-CLAP-Info.plist` | CLAP plugin bundle Info.plist (macOS) |
| `IPlugEffect-AAX-Info.plist` | AAX plugin bundle Info.plist (macOS) |
| `IPlugEffect-macOS-AUv3-Info.plist` | AUv3 plugin Info.plist (macOS) |
| `IPlugEffect-macOS-AUv3Framework-Info.plist` | AUv3 framework Info.plist (macOS) |
| `IPlugEffect-iOS-AUv3-Info.plist` | AUv3 plugin Info.plist (iOS) |
| `IPlugEffect-iOS-AUv3Framework-Info.plist` | AUv3 framework Info.plist (iOS) |
| `IPlugEffect-Pages.xml` | AAX plugin page layout definition |

### Windows Resources
| File | Description |
|------|-------------|
| `main.rc` | Windows resource file defining version info, icons, and metadata (also used by macOS via SWELL) |
| `resource.h` | Windows resource header with resource ID definitions |
| `IPlugEffect.ico` | Windows application icon |

### macOS/iOS Interface Files
| File | Description |
|------|-------------|
| `main.rc_mac_dlg` | SWELL dialog resource generated from main.rc |
| `main.rc_mac_menu` | SWELL menu resource generated from main.rc |
| `IPlugEffect-macOS-MainMenu.xib` | macOS standalone app menu bar definition |
| `IPlugEffect-iOS.storyboard` | iOS app main storyboard |
| `IPlugEffect-iOS-LaunchScreen.storyboard` | iOS app launch screen |
| `IPlugEffect-iOS-MainInterface.storyboard` | iOS app main interface |
| `IPlugAUViewController_vIPlugEffect.xib` | Audio Unit view controller interface |
| `AUv3Framework.h` | AUv3 framework header |
| `IPlugEffect.icns` | macOS bundle icon (for .app, .vst, .vst3, .component bundles) |

### App Icons and Assets
| File | Description |
|------|-------------|
| `Images.xcassets/` | iOS app icon asset catalog |
| `Images.xcassets/IPlugEffectiOSAppIcon.appiconset/` | iOS app icons in various sizes |

### Fonts and Image Resources
| File | Description |
|------|-------------|
| `fonts/Roboto-Regular.ttf` | Default font used by the plugin UI |
| `img/` | Directory for plugin-specific image resources (knobs, backgrounds, etc.) |

## Build Scripts (`scripts/`)

| File | Description |
|------|-------------|
| `postbuild-win.bat` | Windows post-build script (runs after compilation, e.g., for code signing) |
| `prebuild-win.bat` | Windows pre-build script (runs before compilation) |
| `prepare_resources-win.py` | Python script to prepare Windows resources before build |
| `prepare_resources-mac.py` | Python script to prepare macOS resources before build |
| `prepare_resources-ios.py` | Python script to prepare iOS resources before build |
| `makedist-web.sh` | Shell script to build/create Web Audio Module (WAM) package |
