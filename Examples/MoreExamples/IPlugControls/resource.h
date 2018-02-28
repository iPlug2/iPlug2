#define PLUG_MFR "AcmeInc"
#define PLUG_NAME "IPlugControls"

#define PLUG_CLASS_NAME IPlugControls

#define BUNDLE_MFR "AcmeInc"
#define BUNDLE_NAME "IPlugControls"

#define PLUG_ENTRY IPlugControls_Entry
#define PLUG_VIEW_ENTRY IPlugControls_ViewEntry

#define PLUG_ENTRY_STR "IPlugControls_Entry"
#define PLUG_VIEW_ENTRY_STR "IPlugControls_ViewEntry"

#define VIEW_CLASS IPlugControls_View
#define VIEW_CLASS_STR "IPlugControls_View"

// Format        0xMAJR.MN.BG - in HEX! so version 10.1.5 would be 0x000A0105
#define PLUG_VER 0x00010000
#define VST3_VER_STR "1.0.0"

// http://service.steinberg.de/databases/plugin.nsf/plugIn?openForm
// 4 chars, single quotes. At least one capital letter
#define PLUG_UNIQUE_ID 'Ipct'
// make sure this is not the same as BUNDLE_MFR
#define PLUG_MFR_ID 'Acme'

// ProTools stuff

// Unique IDs for each configuration of the plug-in
#if (defined(AAX_API) || defined(RTAS_API)) && !defined(_PIDS_)
  #define _PIDS_
  const int PLUG_TYPE_IDS[2] = {'CCN1', 'CCN2'};
  //const int PLUG_TYPE_IDS_AS[2] = {'CCA1', 'CCA2'}; // AudioSuite
#endif

#define PLUG_MFR_PT "AcmeInc\nAcmeInc\nAcme"
#define PLUG_NAME_PT "IPlugControls\nIPCT"
#define PLUG_TYPE_PT "Effect"
#define PLUG_DOES_AUDIOSUITE 0

/* PLUG_TYPE_PT can be "None", "EQ", "Dynamics", "PitchShift", "Reverb", "Delay", "Modulation", 
"Harmonic" "NoiseReduction" "Dither" "SoundField" "Effect" 
instrument determined by PLUG _IS _INST
*/

#define PLUG_CHANNEL_IO "1-1 2-2"

#define PLUG_LATENCY 0
#define PLUG_IS_INST 0

// if this is 0 RTAS can't get tempo info
#define PLUG_DOES_MIDI 0

#define PLUG_DOES_STATE_CHUNKS 0

// Unique IDs for each image resource.
#define KNOB_ID 101

// Unique IDs for each image resource.
#define BG_ID                   100
#define ISWITCHCONTROL_2_ID     101
#define ISWITCHCONTROL_3_ID     102
#define IRADIOBUTTONSCONTROL_ID 103
#define ICONTACTCONTROL_ID      104
#define IFADERCONTROL_HORIZ_ID  105
#define IFADERCONTROL_VERT_ID   106
#define IKNOBROTATERCONTROL_ID  107
#define IKNOBMULTICONTROL_ID    108
#define IKRMC_BASE_ID           109
#define IKRMC_MASK_ID           110
#define IKRMC_TOP_ID            111
#define IBOC_ID                 112

// Image resource locations for this plug.
#define BG_FN                     "resources/img/BG_1024x640.png"
#define ISWITCHCONTROL_2_FN       "resources/img/ISwitchControl_x2.png"
#define ISWITCHCONTROL_3_FN       "resources/img/ISwitchControl_x3.png"
#define IRADIOBUTTONSCONTROL_FN   "resources/img/IRadioButtonsControl_x2.png"
#define ICONTACTCONTROL_FN        "resources/img/IContactControl_x2.png"
#define IFADERCONTROL_HORIZ_FN    "resources/img/IFaderControl_Horiz.png"
#define IFADERCONTROL_VERT_FN     "resources/img/IFaderControl_Vert.png"
#define IKNOBROTATERCONTROL_FN    "resources/img/IKnobRotaterControl.png"
#define IKNOBMULTICONTROL_FN      "resources/img/IKnobMultiControl_x14.png"
#define IKRMC_BASE_FN             "resources/img/IKRMC_Base.png"
#define IKRMC_MASK_FN             "resources/img/IKRMC_Mask.png"
#define IKRMC_TOP_FN              "resources/img/IKRMC_Top.png"
#define IBOC_FN                   "resources/img/IBitmapOverlayControl.png"

// GUI default dimensions
#define GUI_WIDTH   1024
#define GUI_HEIGHT  640

// on MSVC, you must define SA_API in the resource editor preprocessor macros as well as the c++ ones
#if defined(SA_API) && !defined(OS_IOS)
#include "app_wrapper/app_resource.h"
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
