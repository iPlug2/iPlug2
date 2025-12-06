# iPlug 2 Context

## Project Overview

**iPlug 2** is a C++ framework for developing cross-platform audio plug-ins (VST2, VST3, AU, AAX, CLAP, Web Audio Modules) and standalone applications. It abstracts the complexities of various plug-in APIs and provides a high-performance graphics library (`IGraphics`) for creating custom user interfaces.

*   **Core Language:** C++ (C++11 standard).
*   **Key Dependencies:** WDL (Cockos' library), NanoVG/Skia (for graphics).
*   **Target Platforms:** macOS, Windows, iOS, Web (via Emscripten).

## Directory Structure

*   **`IPlug/`**: Contains the core framework code and API abstractions (`IPlugAPIBase`, `IPlugProcessor`, etc.).
*   **`IGraphics/`**: The graphics library, handling drawing, controls, and platform-specific UI integration.
*   **`Examples/`**: A collection of example projects demonstrating various features (e.g., `IPlugEffect`, `IPlugInstrument`).
*   **`Tests/`**: A collection of projects for testing the framework.
*   **`Dependencies/`**: Scripts and folders for managing external dependencies.
*   **`Scripts/`**: Python and Shell scripts for build automation, resource preparation, and CI.
*   **`WDL/`**: The Cockos WDL library, providing low-level utilities (containers, strings, DSP).

## Building and Development

### Prerequisites

Before building any project, you *may* need to download prebuilt dependencies. This is only strictly necessary if you are using the Skia graphics backend.

```bash
# Downloads dependencies for the current platform (macOS/Windows/iOS) - Required for Skia
./Dependencies/download-prebuilt-libs.sh
```

### Building Examples (macOS)

To build all example projects on macOS:

```bash
./Examples/buildall-mac.sh
```

You can also build individual examples without using the `buildall` script by opening the project-specific file (e.g., `.xcodeproj`) or using command-line tools like `xcodebuild` on the specific project.

Individual projects are located in `Examples/<ProjectName>/`. They typically contain:
*   `projects/<ProjectName>-macOS.xcodeproj`: Xcode project for macOS.
*   `projects/<ProjectName>-iOS.xcodeproj`: Xcode project for iOS.
*   `IPlugEffect.sln`: Visual Studio solution for Windows.

### Creating a New Project

While the [iPlug2OOS](https://github.com/iPlug2/iPlug2OOS) repository is recommended for out-of-source projects, within this repository, you can create a new project by duplicating an existing example (e.g., `Examples/IPlugEffect`) and renaming the files and classes.

1.  Duplicate `Examples/IPlugEffect`.
2.  Rename the folder and files (use the `duplicate.py` script in `Examples/` if available, or manual renaming).
3.  Update `config.h` with your unique plugin ID and manufacturer information.

## Code Conventions

*   **Indentation:** 2 spaces. No tabs.
*   **Line Endings:** Unix style (`\n`), except for Windows-specific files.
*   **Naming:**
    *   Classes: `CamelCase`
    *   Member variables: `mVariable`
    *   Pointers: `pPointer` (e.g., `pControl`)
    *   Local variables: `camelCase`
*   **Strings:** Prefer `WDL_String` over `std::string`.
*   **Standard Library:** Avoid STL unless necessary; use WDL containers (`WDL_PtrList`, etc.) where possible to keep binary size small.

## Key Files in a Project

Taking `IPlugEffect` as a template:

*   **`IPlugEffect.h`**: Parameter definitions (`kParamVolume`), class declaration (inherits from `IPlug`).
*   **`IPlugEffect.cpp`**: Plugin logic, `ProcessBlock` (audio processing), `OnReset` (initialization).
*   **`config.h`**: Defines plugin name, version, unique IDs (`PLUG_UNIQUE_ID`), and channel I/O configurations.
*   **`resources/main.rc`**: Windows resource file.
*   **`resources/*.plist`**: macOS/iOS property lists.
