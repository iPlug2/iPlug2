# IPlugARAExample

An ARA 2 (Audio Random Access) plug-in built with iPlug2, in the VST3 format, demonstrating the
main things ARA lets a plug-in do that a normal plug-in cannot.

ARA is an extension of a companion plug-in format rather than a standalone format. In iPlug2 it is
enabled by defining `ARA_API` at project level in addition to `VST3_API`, which is what the
`VST3_ARA` CMake format does. The resulting `.vst3` bundle is a regular VST3 plug-in that
additionally exposes:

- `ARA::IPlugInEntryPoint` / `ARA::IPlugInEntryPoint2` on the audio effect component
- an `ARA::IMainFactory` class registered under the `"ARA Main Factory Class"` category

## What it demonstrates

**Random access to the host's audio.** When the host binds an instance with the *playback
renderer* role, the plug-in ignores its audio input and instead renders the ARA playback regions
the host assigned to it, reading the audio source samples through the ARA host audio reader
(cached in memory). Without ARA it passes audio through.

**Background analysis with progress reporting.** Each audio source is analysed on a worker thread.
Progress is reported to the host with
`notifyAudioSourceAnalysisProgressStarted/Updated/Completed`, so the host can show it in its own
UI, and the result is published with `notifyAudioSourceContentChanged` from the main thread. The
analysis is deliberately paced so the progress is actually visible.

**Content export back to the host.** The analysis detects notes (onset segmentation plus a
normalized-autocorrelation pitch estimate), and the plug-in publishes them as
`kARAContentTypeNotes` through `doIsAudioSourceContentAvailable` /
`doGetAudioSourceContentGrade` / `doCreateAudioSourceContentReader`. This is the mechanism a pitch
editor uses to let the host read its detection results, e.g. to drag them out as MIDI. Content is
exported at all three model levels, and each level transforms it:

| level | transformation applied |
|---|---|
| audio source | raw detection results |
| audio modification | note positions mirrored if the modification is reversed |
| playback region | mapped into song time, offset by the region start and scaled by its timestretch |

**Audio modifications - the ARA editing layer.** `IPlugARAExampleAudioModification` holds
per-modification state (gain and a reverse switch) that the renderer applies. Several
modifications can share one audio source, and the host clones them when a take is duplicated,
which is what the `optionalModificationToClone` constructor argument handles.

**Timestretch.** The factory advertises
`kARAPlaybackTransformationTimestretch | ...ReflectingTempo`, so hosts let the user drag a
region's borders independently of its content. The renderer maps the region's playback range onto
its modification range as a ratio of the two durations, which covers both sample rate conversion
and stretching.

**Driving the host.** Clicking a region in the plug-in UI calls the ARA playback controller
(`requestSetPlaybackPosition` + `requestStartPlayback`) to move the host transport - the one ARA
interface where the plug-in drives the host rather than the other way round.

**Editor renderer role.** Alt-clicking a region auditions it via `IPlugARAExampleEditorRenderer`,
independently of the host transport. Per the ARA spec, an editor renderer that is not also a
playback renderer forwards its input and adds its preview signal on top.

**Persistence.** The analysis results and the modification edits are stored in the ARA document
archive keyed by persistent ID. Because `ARA_SUPPORTS_AUDIO_FILE_CHUNKS` is enabled, the same
routine also produces the per-source archive for an ARA audio file chunk, so the analysis can
travel with the audio file instead of only living in the host project.

## The UI

The IGraphics timeline visualizes the ARA document this instance is bound to:

- one row per region sequence (host track), using host-provided names and colors
- playback regions with waveforms drawn from the cached audio source samples, reflecting the
  modification's reverse state
- the detected notes drawn as a pitch lane, and analysis progress while it runs
- the timestretch factor the host applied to each region
- the host view selection (via the ARA *editor view* role) and hidden region sequences
- host tempo, time signature and key signature, read with ARA content readers from the musical
  context
- the host transport position, which ARA roles this instance fulfills, and whether the host
  offers transport control

Mouse: click a region to move the host transport there, alt-click to audition it through the
editor renderer, right-click to toggle the modification's reverse edit.

## Configuration

`config.h` drives the ARA factory:

| macro | purpose |
|---|---|
| `ARA_DOC_CONTROLLER_CLASS` | the plug-in's `ARA::PlugIn::DocumentController` subclass |
| `ARA_FACTORY_ID`, `ARA_DOC_ARCHIVE_ID` | ARA identifiers |
| `ARA_PLAYBACK_TRANSFORMATION_FLAGS` | which playback transformations the plug-in applies |
| `ARA_ANALYZEABLE_CONTENT_TYPES` | content types the plug-in analyses and exports |
| `ARA_SUPPORTS_AUDIO_FILE_CHUNKS` | whether analysis can be stored into audio file chunks |

It also uses the `"Fx|OnlyARA"` VST3 subcategory, which marks a plug-in that requires ARA to
operate. Plug-ins that also work without ARA should use `"Fx|ARA"` instead.

N.B. For clarity this example does not synchronize ARA model edits (main thread) with rendering
(audio thread) - a real plug-in must do so. Its archive format is also simplified (native endian),
and the note detection is deliberately crude, standing in for the real analysis a pitch editor
would perform.

## Building

```sh
cmake -S . -B build
cmake --build build --target IPlugARAExample-vst3-ara
```

The ARA SDK (`ARA_API` + `ARA_Library`, Apache 2.0 licensed) is fetched automatically via CMake
FetchContent, pinned to an ARA SDK release tag. To use a local copy instead, clone
[Celemony/ARA_SDK](https://github.com/Celemony/ARA_SDK) (with submodules) into
`Dependencies/IPlug/ARA_SDK`, or pass `-DARA_SDK_DIR=/path/to/ARA_SDK`.

The VST3 SDK must be present in `Dependencies/IPlug/VST3_SDK` (see
`Dependencies/IPlug/download-vst3-sdk.sh`).
