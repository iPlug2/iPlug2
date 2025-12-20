#define PLUG_NAME "IPlugParametricEQ"
#define PLUG_MFR "AcmeInc"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'PrEQ'
#define PLUG_MFR_ID 'Acme'
#define PLUG_URL_STR "https://iplug2.github.io"
#define PLUG_EMAIL_STR "spam@me.com"
#define PLUG_COPYRIGHT_STR "Copyright 2024 Acme Inc"
#define PLUG_CLASS_NAME IPlugParametricEQ

#define BUNDLE_NAME "IPlugParametricEQ"
#define BUNDLE_MFR "AcmeInc"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "IPlugParametricEQ"

#ifdef APP_API
#define PLUG_CHANNEL_IO "1-2"
#else
#define PLUG_CHANNEL_IO "1-1 2-2"
#endif

#define PLUG_LATENCY 0
#define PLUG_TYPE 0
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 1
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 800
#define PLUG_HEIGHT 500
#define PLUG_MIN_WIDTH 400
#define PLUG_MIN_HEIGHT 250
#define PLUG_MAX_WIDTH 8192
#define PLUG_MAX_HEIGHT 8192
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 1

#define AUV2_ENTRY IPlugParametricEQ_Entry
#define AUV2_ENTRY_STR "IPlugParametricEQ_Entry"
#define AUV2_FACTORY IPlugParametricEQ_Factory
#define AUV2_VIEW_CLASS IPlugParametricEQ_View
#define AUV2_VIEW_CLASS_STR "IPlugParametricEQ_View"

#define AAX_TYPE_IDS 'PEQ1', 'PEQ2'
#define AAX_TYPE_IDS_AUDIOSUITE 'PEA1', 'PEA2'
#define AAX_PLUG_MFR_STR "Acme"
#define AAX_PLUG_NAME_STR "IPlugParametricEQ\nIPEQ"
#define AAX_PLUG_CATEGORY_STR "EQ"
#define AAX_DOES_AUDIOSUITE 1

#define VST3_SUBCATEGORY "Fx|EQ"

#define CLAP_MANUAL_URL "https://iplug2.github.io/manuals/example_manual.pdf"
#define CLAP_SUPPORT_URL "https://github.com/iPlug2/iPlug2/wiki"
#define CLAP_DESCRIPTION "5-band parametric EQ with spectral analyzer"
#define CLAP_FEATURES "audio-effect", "equalizer"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

#define ROBOTO_FN "Roboto-Regular.ttf"

// Number of EQ bands
#define NUM_EQ_BANDS 5
