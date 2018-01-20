#define PLUG_MFR "AcmeInc"
#define PLUG_NAME "IPlugEffect"
#define PLUG_CLASS_NAME IPlugEffect
#define BUNDLE_MFR "AcmeInc"
#define BUNDLE_NAME "IPlugEffect"
#define PLUG_ENTRY IPlugEffect_Entry
#define PLUG_VIEW_ENTRY IPlugEffect_ViewEntry
#define PLUG_FACTORY IPlugEffect_Factory
#define PLUG_ENTRY_STR "IPlugEffect_Entry"
#define PLUG_VIEW_ENTRY_STR "IPlugEffect_ViewEntry"
#define VIEW_CLASS IPlugEffect_View
#define VIEW_CLASS_STR "IPlugEffect_View"
#define PLUG_VER 0x00010000
#define VST3_VER_STR "1.0.0"
#define PLUG_UNIQUE_ID 'Ipef'
#define PLUG_MFR_ID 'Acme'
#define PLUG_COPYRIGHT  "Copyright 2017 Acme Inc"
#if defined(AAX_API) && !defined(_PIDS_)
  #define _PIDS_
  const int PLUG_TYPE_IDS[2] = {'EFN1', 'EFN2'};
  const int PLUG_TYPE_IDS_AS[2] = {'EFA1', 'EFA2'}; // AudioSuite
#endif
#define PLUG_MFR_PT "AcmeInc\nAcmeInc\nAcme"
#define PLUG_NAME_PT "IPlugEffect\nIPEF"
#define PLUG_TYPE_PT "Effect"
#define PLUG_DOES_AUDIOSUITE 1
#define PLUG_CHANNEL_IO "1-1 2-2"
#define PLUG_LATENCY 0
#define PLUG_IS_INST 0
#define PLUG_DOES_MIDI 0
#define PLUG_DOES_STATE_CHUNKS 0
#define MFR_URL "www.olilarkin.co.uk"
#define MFR_EMAIL "spam@me.com"
#define EFFECT_TYPE_VST3 "Fx"
#define ENABLE_SYSEX 0
#define ENABLE_MIDICLOCK 0
#define ENABLE_ACTIVE_SENSING 0
#define NUM_CHANNELS 2
#define N_VECTOR_WAIT 50
#define APP_MULT 0.25
//#define KNOB_FN "resources/img/knob.png"

