#define PLUG_MFR "AcmeInc"
#define PLUG_NAME "IPlugConvoEngine"

#define PLUG_CLASS_NAME IPlugConvoEngine

#define BUNDLE_MFR "AcmeInc"
#define BUNDLE_NAME "IPlugConvoEngine"

#define PLUG_ENTRY IPlugConvoEngine_Entry
#define PLUG_VIEW_ENTRY IPlugConvoEngine_ViewEntry

#define PLUG_ENTRY_STR "IPlugConvoEngine_Entry"
#define PLUG_VIEW_ENTRY_STR "IPlugConvoEngine_ViewEntry"

#define VIEW_CLASS IPlugConvoEngine_View
#define VIEW_CLASS_STR "IPlugConvoEngine_View"

// Format        0xMAJR.MN.BG - in HEX! so version 10.1.5 would be 0x000A0105
#define PLUG_VER 0x00010000
#define VST3_VER_STR "1.0.0"

// http://service.steinberg.de/databases/plugin.nsf/plugIn?openForm
// 4 chars, single quotes. At least one capital letter
#define PLUG_UNIQUE_ID 'Ipce'
// make sure this is not the same as BUNDLE_MFR
#define PLUG_MFR_ID 'Acme'

// ProTools stuff
#define PLUG_MFR_DIGI "AcmeInc\nAcmeInc\nAcme\n"
#define PLUG_NAME_DIGI "IPlugConvoEngine\nIPCE"
#define EFFECT_TYPE_DIGI "Effect" // valid options "None" "EQ" "Dynamics" "PitchShift" "Reverb" "Delay" "Modulation" "Harmonic" "NoiseReduction" "Dither" "SoundField" "Effect" instrument determined by PLUG _IS _INST

#define PLUG_CHANNEL_IO "1-1 2-2"

// if you want to do anything unusual re i/o you need to #ifdef PLUG_CHANNEL_IO and PLUG_SC_CHANS depending on the api because they all do it differently...

// PLUGINS WITH SIDE CHAIN INPUTS
// ***************************
//#ifdef RTAS_API
// PLUG_SC_CHANS defines the number of inputs in the PLUG_CHANNEL_IO that should be considered sidechain inputs.
// RTAS can only have one mono sidechain input, so for instance to make a mono/stereo plugin with a side chain input you could do this.
//#define PLUG_CHANNEL_IO "2-1 3-2"
//#define PLUG_SC_CHANS 1
//#else // AU & VST2
// AU sidechains work with audiounit effects or midi controlled effects only... not instruments
// this works for a mono plug with optional mono sidechain...
//#define PLUG_CHANNEL_IO "1-1 2-1"
//#define PLUG_SC_CHANS 1
// this DOESN'T work (in aulab) for a stereo plug with optional mono sidechain...
//#define PLUG_CHANNEL_IO "2-2 3-2"
//#define PLUG_SC_CHANS 1
// this works for a stereo plug with optional stereo sidechain...
//#define PLUG_CHANNEL_IO "2-2 4-2"
//#define PLUG_SC_CHANS 2
// but a combination DOESN'T work right now (in aulab)
//#define PLUG_CHANNEL_IO "1-1 2-1 2-2 4-2"
//#define PLUG_SC_CHANS 1
//#endif

// PLUGIN INSTRUMENTS (WITH MULTIPLE OUTPUTS)
// ***************************
//#ifdef RTAS_API
// rtas instruments have to say they have inputs
// rtas multiple outputs will result in a multichannel bus
//#define PLUG_CHANNEL_IO "1-1 2-2"
//#else // AU & VST2
// in AU these will be grouped as stereo pairs... that is fixed right now
//#define PLUG_CHANNEL_IO "0-2 0-4 0-6 0-8"
//#endif
//#define PLUG_SC_CHANS 0

// MULTI-CHANNEL EFFECT PLUGINS (I.E MONO->QUAD, QUAD->QUAD etc)
// ***************************
// seems to be ok for au and rtas
//#define PLUG_CHANNEL_IO "1-4 4-4"
//#define PLUG_SC_CHANS 0

//

#define PLUG_LATENCY 0
#define PLUG_IS_INST 0
// if this is 0 RTAS can't get tempo info
#define PLUG_DOES_MIDI 0 
#define PLUG_DOES_STATE_CHUNKS 0

// Unique IDs for each image resource.
#define KNOB_ID       101

// Image resource locations for this plug.
#define KNOB_FN       "resources/img/knob.png"

// GUI default dimensions
#define GUI_WIDTH   300
#define GUI_HEIGHT  300

// on MSVC, you must define SA_API in the resource editor preprocessor macros as well as the c++ ones
#ifdef SA_API
  #ifndef OS_IOS
    #include "app_wrapper/app_resource.h"
  #endif
#endif

// vst3 stuff
#define MFR_URL "www.olilarkin.co.uk"
#define MFR_EMAIL "spam@me.com"
#define EFFECT_TYPE_VST3 "Fx"

/* "Fx|Analyzer"", "Fx|Delay", "Fx|Distortion", "Fx|Dynamics", "Fx|EQ", "Fx|Filter",
"Fx", "Fx|Instrument", "Fx|InstrumentExternal", "Fx|Spatial", "Fx|Generator",
"Fx|Mastering", "Fx|Modulation", "Fx|PitchShift", "Fx|Restoration", "Fx|Reverb",
"Fx|Surround", "Fx|Tools", "Instrument", "Instrument|Drum", "Instrument|Sampler",
"Instrument|Synth", "Instrument|Synth|Sampler", "Instrument|External", "Spatial",
"Spatial|Fx", "OnlyRT", "OnlyOfflineProcess", "Mono", "Stereo",
"Surround"
*/
