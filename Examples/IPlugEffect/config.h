//#include "IPlugPlatform.h"

#define PLUG_NAME "IPlugEffect"
#define PLUG_MFR "AcmeInc"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'Ipef'
#define PLUG_MFR_ID 'Acme'
#define PLUG_URL_STR "www.olilarkin.co.uk"
#define PLUG_EMAIL_STR "spam@me.com"
#define PLUG_COPYRIGHT_STR  "Copyright 2017 Acme Inc"

#define PLUG_CLASS_NAME IPlugEffect

#define BUNDLE_NAME "IPlugEffect"
#define BUNDLE_MFR "AcmeInc"
#define BUNDLE_DOMAIN "com"

#define PLUG_CHANNEL_IO "2-2"
//#define PLUG_CHANNEL_IO "0-2"
//#define PLUG_CHANNEL_IO "*-16" // wildcard not validating

//#define PLUG_CHANNEL_IO "1-1 1-2 2-2"
//#define PLUG_CHANNEL_IO "0-64" // fails with VST 3

//#define PLUG_CHANNEL_IO "0-2 0-2.2 0-2.2.2 0-2.2.2.2"

//#define PLUG_CHANNEL_IO "1-4 1-16 1-25 1-36 1-64"

//#define PLUG_CHANNEL_IO "1-1 1.0-1 2.1-2" // should fail because we can't define a bus with 0

//#define PLUG_CHANNEL_IO "1-1 \
//                         1.1-1 \
//                         2-2 \
//                         2.2-2" // does not validate
//#define PLUG_CHANNEL_IO "1-1 1.1-1 2.1-2" // does not validate
//#define PLUG_CHANNEL_IO "1-1 1.1-1 1.1-2" // does not validate


#define PLUG_LATENCY 0
#define PLUG_IS_INSTRUMENT 0
#define PLUG_DOES_MIDI 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1
#define PLUG_WIDTH 900
#define PLUG_HEIGHT 600

#define AUV2_ENTRY IPlugEffect_Entry
#define AUV2_ENTRY_STR "IPlugEffect_Entry"
#define AUV2_FACTORY IPlugEffect_Factory
#define AUV2_VIEW_CLASS IPlugEffect_View
#define AUV2_VIEW_CLASS_STR "IPlugEffect_View"

#define AAX_TYPE_IDS 'EFN1', 'EFN2'
#define AAX_TYPE_IDS_AUDIOSUITE 'EFA1', 'EFA2'
#define AAX_PLUG_MFR_STR "AcmeInc\nAcmeInc\nAcme"
#define AAX_PLUG_NAME_STR "IPlugEffect\nIPEF"
#define AAX_PLUG_CATEGORY_STR "Effect"
#define AAX_DOES_AUDIOSUITE 1

#define VST3_SUBCATEGORY "Fx"

#define APP_ENABLE_SYSEX 0
#define APP_ENABLE_MIDICLOCK 0
#define APP_ENABLE_ACTIVE_SENSING 0
#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 50
#define APP_MULT 0.25

#define PNGKNOB_FN "resources/img/knob.png"
#define PNGKNOBROTATE_FN "resources/img/knob-rotate.png"
#define SVGKNOB_FN "resources/img/BefacoBigKnob.svg"
#define TIGER_FN "resources/img/23.svg"
