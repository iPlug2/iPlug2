#define PLUG_NAME "IPlugDrumSynth"
#define PLUG_MFR "AcmeInc"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'GZDU'
#define PLUG_MFR_ID 'Acme'
#define PLUG_URL_STR "https://iplug2.github.io"
#define PLUG_EMAIL_STR "spam@me.com"
#define PLUG_COPYRIGHT_STR "Copyright 2019 Acme Inc"
#define PLUG_CLASS_NAME IPlugDrumSynth

#define BUNDLE_NAME "IPlugDrumSynth"
#define BUNDLE_MFR "AcmeInc"
#define BUNDLE_DOMAIN "com"

#define PLUG_CHANNEL_IO " \
0-2 \
0-2.2 \
0-2.2.2 \
0-2.2.2.2"

#define PLUG_LATENCY 0
#define PLUG_TYPE 1
#define PLUG_DOES_MIDI_IN 1
#define PLUG_DOES_MIDI_OUT 1
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 335
#define PLUG_HEIGHT 335
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0

#define SHARED_RESOURCES_SUBPATH "IPlugDrumSynth"

#define AUV2_ENTRY IPlugDrumSynth_Entry
#define AUV2_ENTRY_STR "IPlugDrumSynth_Entry"
#define AUV2_FACTORY IPlugDrumSynth_Factory
#define AUV2_VIEW_CLASS IPlugDrumSynth_View
#define AUV2_VIEW_CLASS_STR "IPlugDrumSynth_View"

#define AAX_TYPE_IDS 'IPD1'
#define AAX_PLUG_MFR_STR "Acme"
#define AAX_PLUG_NAME_STR "IPlugDrumSynth\nIPDS"
#define AAX_DOES_AUDIOSUITE 0
#define AAX_PLUG_CATEGORY_STR "Synth"
#define AAX_AOS_STRS "Drum2", "Drum3", "Drum4"
#define VST3_SUBCATEGORY "Instrument|Synth"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64

#define ROBOTO_FN "Roboto-Regular.ttf"
