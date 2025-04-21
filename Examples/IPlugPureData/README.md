# IPlug2 with libpd Integration

This example demonstrates how to integrate libpd with iPlug2 to create a VST plugin that can load and run Pure Data patches.

## Overview

The integration consists of three main components:

1. **IPlugPureData** - The main plugin class that inherits from iPlug2's Plugin class
2. **PdWrapper** - A wrapper class that handles the libpd functionality
3. **libpd** - The Pure Data library that provides the core functionality

## Setup

To use this example, you need to:

1. Install libpd and make sure it's available in your build environment
2. Link against the libpd library in your build configuration
3. Include the necessary header files

## Usage

### Loading a Pure Data Patch

To load a Pure Data patch, call the `OpenPatch` method with the filename and directory:

```cpp
plugin->OpenPatch("my_patch.pd", "/path/to/patch/directory");
```

### Sending Parameters to Pure Data

To send parameters to Pure Data, use the `SetParamValue` method:

```cpp
plugin->SetParamValue(paramIndex, value);
```

### Receiving MIDI from Pure Data

The plugin automatically handles MIDI messages from Pure Data. To send MIDI from your DAW to Pure Data, simply send MIDI to the plugin.

### Processing Audio

The plugin automatically processes audio through the loaded Pure Data patch. The gain parameter in the UI is applied to the output of the Pure Data patch.

## Implementation Details

### PdWrapper Class

The `PdWrapper` class handles all the libpd functionality:

- Initialization and cleanup of libpd
- Loading and unloading Pure Data patches
- Sending parameters to Pure Data
- Processing audio through Pure Data
- Handling MIDI messages

### IPlugPureData Class

The `IPlugPureData` class is the main plugin class that:

- Creates and manages the PdWrapper instance
- Handles parameter changes from the UI
- Processes audio through the PdWrapper
- Handles MIDI messages from the host

## Limitations

- The current implementation only supports stereo input and output
- The audio processing is done in blocks of 64 samples
- The gain parameter is applied to the output of the Pure Data patch

## Future Improvements

- Support for multi-channel audio
- Support for variable block sizes
- Support for more parameter types
- Support for more MIDI message types
- Support for more Pure Data features

## License

This example is provided under the same license as iPlug2 and libpd.
