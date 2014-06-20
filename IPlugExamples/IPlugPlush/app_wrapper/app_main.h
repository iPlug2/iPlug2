#ifndef _IPLUGAPP_APP_MAIN_H_
#define _IPLUGAPP_APP_MAIN_H_

#include "IPlugOSDetect.h"

/*

 Standalone osx/win app wrapper for iPlug, using SWELL
 Oli Larkin 2012

 Notes:

 App settings are stored in a .ini file. The location is as follows:

 Windows7: C:\Users\USERNAME\AppData\Local\IPlugPlush\settings.ini
 Windows XP/Vista: C:\Documents and Settings\USERNAME\Local Settings\Application Data\IPlugPlush\settings.ini
 OSX: /Users/USERNAME/Library/Application\ Support/IPlugPlush/settings.ini

*/

#ifdef OS_WIN
  #include <windows.h>
  #include <commctrl.h>

  #define DEFAULT_INPUT_DEV "Default Device"
  #define DEFAULT_OUTPUT_DEV "Default Device"

  #define DAC_DS 0
  #define DAC_ASIO 1
#elif defined OS_OSX
  #include "swell.h"
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )

  #define DEFAULT_INPUT_DEV "Built-in Input"
  #define DEFAULT_OUTPUT_DEV "Built-in Output"

  #define DAC_COREAUDIO 0
//  #define DAC_JACK 1
#endif

#include "wdltypes.h"
#include "RtAudio.h"
#include "RtMidi.h"
#include <string>
#include <vector>

#include "../IPlugPlush.h" // change this to match your iplug plugin .h file

typedef unsigned short UInt16;

struct AppState
{
  // on osx core audio 0 or jack 1
  // on windows DS 0 or ASIO 1
  UInt16 mAudioDriverType;

  // strings
  char mAudioInDev[100];
  char mAudioOutDev[100];
  char mAudioSR[100];
  char mAudioIOVS[100];
  char mAudioSigVS[100];

  UInt16 mAudioInChanL;
  UInt16 mAudioInChanR;
  UInt16 mAudioOutChanL;
  UInt16 mAudioOutChanR;
  UInt16 mAudioInIsMono;

  // strings containing the names of the midi devices
  char mMidiInDev[100];
  char mMidiOutDev[100];

  UInt16 mMidiInChan;
  UInt16 mMidiOutChan;

  AppState():
    mAudioDriverType(0), // DS / CoreAudio by default
    mAudioInChanL(1),
    mAudioInChanR(2),
    mAudioOutChanL(1),
    mAudioOutChanR(2),
    mMidiInChan(0),
    mMidiOutChan(0)
  {
    strcpy(mAudioInDev, DEFAULT_INPUT_DEV);
    strcpy(mAudioOutDev, DEFAULT_OUTPUT_DEV);
    strcpy(mAudioSR, "44100");
    strcpy(mAudioIOVS, "512");
    strcpy(mAudioSigVS, "32");

    strcpy(mMidiInDev, "off");
    strcpy(mMidiOutDev, "off");
  }
};

extern WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern WDL_DLGRET PreferencesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern HINSTANCE gHINST;
extern HWND gHWND;
extern UINT gScrollMessage;
extern IPlug* gPluginInstance; // The iplug plugin instance

extern std::string GetAudioDeviceName(int idx);
extern int GetAudioDeviceID(char* deviceNameToTest);

extern void ProbeAudioIO();
extern bool InitialiseAudio(unsigned int inId,
                            unsigned int outId,
                            unsigned int sr,
                            unsigned int iovs,
                            unsigned int chnls,
                            unsigned int inChanL,
                            unsigned int outChanL
                           );

extern bool AudioSettingsInStateAreEqual(AppState* os, AppState* ns);
extern bool MIDISettingsInStateAreEqual(AppState* os, AppState* ns);

extern bool TryToChangeAudioDriverType();
extern bool TryToChangeAudio();
extern bool ChooseMidiInput(const char* pPortName);
extern bool ChooseMidiOutput(const char* pPortName);

extern bool AttachGUI();

extern RtAudio* gDAC;
extern RtMidiIn *gMidiIn;
extern RtMidiOut *gMidiOut;

extern AppState *gState;
extern AppState *gTempState; // The state is copied here when the pref dialog is opened, and restored if cancel is pressed
extern AppState *gActiveState; // When the audio driver is started the current state is copied here so that if OK is pressed after APPLY nothing is changed

extern unsigned int gSigVS;
extern unsigned int gBufIndex; // index for signal vector, loops from 0 to gSigVS

extern char *gINIPath; // path of ini file
extern void UpdateINI();

extern std::vector<unsigned int> gAudioInputDevs;
extern std::vector<unsigned int> gAudioOutputDevs;
extern std::vector<std::string> gMIDIInputDevNames;
extern std::vector<std::string> gMIDIOutputDevNames;

#endif //_IPLUGAPP_APP_MAIN_H_

