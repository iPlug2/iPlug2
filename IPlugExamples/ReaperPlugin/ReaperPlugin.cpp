#include "ReaperPlugin.h"
#include "IPlug_include_in_plug_src.h"

#include "resource.h"

const int kNumPrograms = 1;

enum EParams
{
  kAzimuth = 0,
  kElevation,
  kNumParams
};

ReaperPlugin::ReaperPlugin(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.)
{
  TRACE; 

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kAzimuth)->InitDouble("Azimuth", 0, -180., 180., 0.1, "");
  GetParam(kElevation)->InitDouble("Elevation", 0, 0., 90., 0.1, "");

  OnGUICreated();
  
  //MakePreset("preset 1", ... );
  MakeDefaultPreset("-", kNumPrograms);
}

ReaperPlugin::~ReaperPlugin() {}

void ReaperPlugin::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
}

void ReaperPlugin::Reset()
{
}

static WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  ReaperPlugin* _this = (ReaperPlugin*) lParam;
//  SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
  
  switch (uMsg) {
    case WM_INITDIALOG:
//      hwndDlg = _this->mHWND;
      ShowWindow(hwndDlg, SW_SHOW); // TODO: how to embed window?
      return 1;
    default:
      break;
  }
  
  return 0;
}

void* ReaperPlugin::OpenWindow(void* handle)
{
  mHWND = CreateDialogParam(0, MAKEINTRESOURCE(MAPPING_DIALOG),(HWND)handle, MainDlgProc,(LPARAM) this);
  
  return mHWND;
}

#ifndef OS_WIN
#include "swell-dlggen.h"
#define AUTOCHECKBOX CHECKBOX
#define TRACKBAR_CLASS "msctls_trackbar32"
#define CBS_HASSTRINGS 0
#define WS_EX_LEFT
#define SS_WORDELLIPSIS 0
#include "ReaperPlugin_ui.rc_mac_dlg"
//#include "swell-menugen.h"
//#include "sample_project.rc_mac_menu"
#endif
