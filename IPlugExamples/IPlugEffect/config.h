#define PLUG_NAME "IPlugEffect"
#define PLUG_MFR "AcmeInc"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
#define PLUG_UNIQUE_ID 'Ipef'
#define PLUG_MFR_ID 'Acme'
#define PLUG_URL_STR "www.olilarkin.co.uk"
#define PLUG_EMAIL_STR "spam@me.com"
#define PLUG_COPYRIGHT_STR  "Copyright 2017 Acme Inc"

#define BUNDLE_NAME "IPlugEffect"
#define BUNDLE_MFR "AcmeInc"

#define PLUG_CLASS_NAME IPlugEffect
#define PLUG_ENTRY IPlugEffect_Entry
#define PLUG_VIEW_ENTRY IPlugEffect_ViewEntry
#define PLUG_FACTORY IPlugEffect_Factory
#define PLUG_ENTRY_STR "IPlugEffect_Entry"
#define PLUG_VIEW_ENTRY_STR "IPlugEffect_ViewEntry"
#define VIEW_CLASS IPlugEffect_View
#define VIEW_CLASS_STR "IPlugEffect_View"

#define PLUG_CHANNEL_IO "1-1 2-2"
#define PLUG_LATENCY 0
#define PLUG_IS_INSTRUMENT 0
#define PLUG_DOES_MIDI 0
#define PLUG_DOES_STATE_CHUNKS 0

#if defined(AAX_API) && !defined(_PIDS_)
#define _PIDS_
const int PLUG_TYPE_IDS[2] = {'EFN1', 'EFN2'};
const int PLUG_TYPE_IDS_AS[2] = {'EFA1', 'EFA2'}; // AudioSuite
#endif
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
//#define KNOB_FN "resources/img/knob.png"

