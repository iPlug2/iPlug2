#define PLUG_NAME "IPlugARAExample"
#define PLUG_MFR "AcmeInc"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'IpAr'
#define PLUG_MFR_ID 'Acme'
#define PLUG_URL_STR "https://iplug2.github.io"
#define PLUG_EMAIL_STR "spam@me.com"
#define PLUG_COPYRIGHT_STR "Copyright 2025 Acme Inc"
#define PLUG_CLASS_NAME IPlugARAExample

#define BUNDLE_NAME "IPlugARAExample"
#define BUNDLE_MFR "AcmeInc"
#define BUNDLE_DOMAIN "com"

#define PLUG_CHANNEL_IO "1-1 2-2"

#define PLUG_LATENCY 0
#define PLUG_TYPE 0
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 768
#define PLUG_HEIGHT 480
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0

#define ROBOTO_FN "Roboto-Regular.ttf"

// Plug-ins that require ARA to operate should use the "OnlyARA" subcategory,
// plug-ins that also work without ARA should use "ARA" instead
#define VST3_SUBCATEGORY "Fx|OnlyARA"

#define ARA_DOC_CONTROLLER_CLASS IPlugARAExampleDocumentController
#define ARA_FACTORY_ID "com.AcmeInc.IPlugARAExample.arafactory"
#define ARA_DOC_ARCHIVE_ID "com.AcmeInc.IPlugARAExample.aradocumentarchive.2"

// This plug-in stretches playback regions to fit the borders the host gives them, so hosts may
// let the user drag a region's borders independently of its content
#define ARA_PLAYBACK_TRANSFORMATION_FLAGS (ARA::kARAPlaybackTransformationTimestretch | ARA::kARAPlaybackTransformationTimestretchReflectingTempo)

// The content the plug-in analyses and can then export back to the host
#define ARA_ANALYZEABLE_CONTENT_TYPES ARA::kARAContentTypeNotes

// The analysis results can also be stored into an ARA audio file chunk, so that they travel with
// the audio file rather than only living in the host's project
#define ARA_SUPPORTS_AUDIO_FILE_CHUNKS 1
