#define PLUG_NAME "IPlugMidiEffect"
#define PLUG_MFR "AcmeInc"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID '3G5m'
#define PLUG_MFR_ID 'Acme'
#define PLUG_URL_STR "https://iplug2.github.io"
#define PLUG_EMAIL_STR "spam@me.com"
#define PLUG_COPYRIGHT_STR "Copyright 2020 Acme Inc"
#define PLUG_CLASS_NAME IPlugMidiEffect

#define BUNDLE_NAME "IPlugMidiEffect"
#define BUNDLE_MFR "AcmeInc"
#define BUNDLE_DOMAIN "com"

#define PLUG_CHANNEL_IO "0-0"
#define SHARED_RESOURCES_SUBPATH "IPlugMidiEffect"

#define PLUG_LATENCY 0
#define PLUG_TYPE 2
#define PLUG_DOES_MIDI_IN 1
#define PLUG_DOES_MIDI_OUT 1
#define PLUG_DOES_MPE 1
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 300
#define PLUG_HEIGHT 300
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0

#define AUV2_ENTRY IPlugMidiEffect_Entry
#define AUV2_ENTRY_STR "IPlugMidiEffect_Entry"
#define AUV2_FACTORY IPlugMidiEffect_Factory
#define AUV2_VIEW_CLASS IPlugMidiEffect_View
#define AUV2_VIEW_CLASS_STR "IPlugMidiEffect_View"

#define AAX_TYPE_IDS 'IPME'
#define AAX_TYPE_IDS_AUDIOSUITE 'IPMA'
#define AAX_PLUG_MFR_STR "Acme"
#define AAX_PLUG_NAME_STR "IPlugMidiEffect\nIPEF"
#define AAX_PLUG_CATEGORY_STR "Effect"
#define AAX_DOES_AUDIOSUITE 0

#define VST3_SUBCATEGORY "Instrument|Synth"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

#define ROBOTO_FN "Roboto-Regular.ttf"
