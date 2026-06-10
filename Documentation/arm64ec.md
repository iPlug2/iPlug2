# ARM64EC Support

iPlug2 supports building plugins for Windows ARM64EC (ARM64 Emulation Compatible), enabling native performance on Windows ARM devices while maintaining compatibility with x64 hosts.

## What is ARM64EC?

ARM64EC is a Windows ABI that allows ARM64 code to interoperate with x64 code. This is particularly useful for audio plugins because:

- **Native performance**: ARM64EC code runs natively on ARM64 processors
- **x64 host compatibility**: ARM64EC plugins can be loaded by x64 DAWs running under emulation
- **Gradual migration**: Allows mixing ARM64 and x64 code in the same process

This means you can ship ARM64EC plugins that will:
- Run at full native speed when loaded by ARM64 native hosts
- Still work when loaded by x64 hosts (DAWs) running under emulation on ARM64 Windows

## Supported Platforms

ARM64EC builds are supported for:
- **Plugin Formats**: VST2, VST3, CLAP, AAX, Standalone App
- **Graphics Backend**: NanoVG only (Skia not yet supported for ARM64EC)

## Building for ARM64EC

### Visual Studio Requirements

ARM64EC builds require:
- Visual Studio 2022 (v17.0+)
- ARM64EC build tools installed via Visual Studio Installer

#### Installing ARM64EC Build Tools

1. Open **Visual Studio Installer**
2. Click **Modify** on your Visual Studio 2022 installation
3. Go to **Individual Components**
4. Search for "ARM64EC" and install:
   - **MSVC v143 - VS 2022 C++ ARM64EC build tools (Latest)**
### Cross-Compilation

ARM64EC builds use cross-compilation from an x64 host. When building from command line, the build environment must be initialized for ARM64 cross-compilation:

```batch
:: Initialize environment for ARM64EC cross-compilation
call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" amd64_arm64

:: Then build
msbuild YourPlugin.sln /p:Configuration=Release /p:Platform=ARM64EC
```

The `amd64_arm64` argument tells vcvarsall to set up the x64-hosted ARM64 cross-compiler toolchain, which is used for ARM64EC builds.

> **Note**: You can verify the ARM64EC tools are installed by checking if vswhere finds them:
> ```powershell
> & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.ARM64EC -property installationPath
> ```

### Build Configurations

Each iPlug2 solution includes ARM64EC configurations alongside x64:

| Configuration | Platform | Description |
|---------------|----------|-------------|
| Debug | x64 | Debug build for x64 |
| Debug | ARM64EC | Debug build for ARM64EC |
| Release | x64 | Release build for x64 |
| Release | ARM64EC | Release build for ARM64EC |
| Tracer | x64 | Tracer build for x64 |
| Tracer | ARM64EC | Tracer build for ARM64EC |

### Building from Visual Studio

1. Open the solution file (e.g., `IPlugEffect.sln`)
2. Select the desired configuration (Debug/Release/Tracer)
3. Select the platform (ARM64EC)
4. Build the solution

### Building from Command Line

```batch
msbuild YourPlugin.sln /p:Configuration=Release /p:Platform=ARM64EC
```

## Debugger Configuration

Debugger settings are configured via MSBuild properties in `common-win.props`. The following variables control debugger host paths:

### x64 Variables
- `VST2_X64_HOST_PATH` - Host application for x64 VST2 debugging
- `VST3_X64_HOST_PATH` - Host application for x64 VST3 debugging
- `CLAP_X64_HOST_PATH` - Host application for x64 CLAP debugging

### ARM64EC Variables
- `VST2_ARM64EC_HOST_PATH` - Host application for ARM64EC VST2 debugging
- `VST3_ARM64EC_HOST_PATH` - Host application for ARM64EC VST3 debugging
- `CLAP_ARM64EC_HOST_PATH` - Host application for ARM64EC CLAP debugging

### Plugin Output Paths
- `VST3_X64_PATH` - Installation path for x64 VST3 plugins
- `VST3_ARM64EC_PATH` - Installation path for ARM64EC VST3 plugins
- `CLAP_X64_PATH` - Installation path for x64 CLAP plugins
- `CLAP_ARM64EC_PATH` - Installation path for ARM64EC CLAP plugins

## Output File Naming

Built binaries use platform suffixes:

| Format | x64 Output | ARM64EC Output |
|--------|------------|----------------|
| VST2 | `PluginName_x64.dll` | `PluginName_ARM64EC.dll` |
| VST3 | `PluginName.vst3/Contents/x86_64-win/` | `PluginName.vst3/Contents/arm64ec-win/` |
| CLAP | `PluginName.clap` | `PluginName.clap` (in ARM64EC folder) |
| APP | `PluginName_x64.exe` | `PluginName_ARM64EC.exe` |

### Skia Limitation

ARM64EC builds are automatically skipped when `graphics: 'SKIA'` is specified, as Skia does not yet support ARM64EC on Windows.

## Known Limitations

1. **Skia graphics**: Not yet supported for ARM64EC. Use NanoVG for ARM64EC builds.
2. **Testing on x64 machines**: ARM64EC binaries cannot be executed on x64 Windows. Testing requires ARM64 Windows hardware or emulation.
3. **AAX**: ARM64EC AAX support depends on Avid's AAX SDK support for ARM64EC.

## VST3 Bundle Structure

VST3 plugins support multi-architecture bundles. After building both architectures, the bundle structure is:

```
PluginName.vst3/
├── Contents/
│   ├── x86_64-win/
│   │   └── PluginName.vst3
│   └── arm64ec-win/
│       └── PluginName.vst3
└── Resources/
    └── ...
```

This allows a single VST3 bundle to contain both x64 and ARM64EC binaries, with the host selecting the appropriate one at runtime.

## Resources

- [Windows on ARM Overview](https://learn.microsoft.com/en-us/windows/arm/overview)
- [Microsoft ARM64EC Documentation](https://learn.microsoft.com/en-us/windows/arm/arm64ec)
- [VST3 SDK ARM64EC Support](https://steinbergmedia.github.io/vst3_dev_portal/)
