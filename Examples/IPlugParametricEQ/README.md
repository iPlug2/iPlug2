# IPlugParametricEQ

A 5-band parametric equalizer with integrated spectral analyzer.

## Features

- **5-band parametric EQ** using SVF (State Variable Filter) DSP
- **Real-time spectrum analyzer** showing the processed output
- **EQ response curve overlay** showing the combined filter response
- **Draggable band handles** - click and drag to adjust frequency and gain
- **Mouse wheel Q adjustment** - scroll over a band handle to adjust Q
- **Right-click filter type selection** - choose from 8 filter types per band

## Filter Types

Each band supports the following filter types:
- LowPass
- HighPass
- BandPass
- Notch
- Peak
- Bell (parametric EQ)
- LowShelf
- HighShelf

## Default Band Configuration

| Band | Default Freq | Default Type |
|------|-------------|--------------|
| 1    | 80 Hz       | Low Shelf    |
| 2    | 250 Hz      | Bell         |
| 3    | 1 kHz       | Bell         |
| 4    | 4 kHz       | Bell         |
| 5    | 12 kHz      | High Shelf   |

## Usage

- **Drag handles** to adjust frequency (horizontal) and gain (vertical)
- **Mouse wheel** over a handle to adjust Q/bandwidth
- **Right-click** on a handle to change filter type
- **Right-click** on the analyzer background to access FFT options
- **Toggle buttons** at the bottom to enable/disable each band
- **Gain knobs** at the bottom for quick gain adjustments

## Parameters

Each band exposes 5 parameters (25 total):
- Frequency (20 Hz - 20 kHz)
- Gain (-24 dB to +24 dB)
- Q (0.1 to 20)
- Filter Type
- Enable/Bypass
