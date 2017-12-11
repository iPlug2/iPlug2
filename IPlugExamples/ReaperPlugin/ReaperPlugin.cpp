#include "ReaperPlugin.h"
#include "IPlug_include_in_plug_src.h"

#include "config.h"
#include "resource.h"
#include "Log.h"

audioMasterCallback gHostCallback;

const int kNumPrograms = 1;

enum EParams
{
  kAzimuth = 0,
  kElevation,
  kRadius,
  kNumParams
};

ReaperPlugin::ReaperPlugin(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE; 

  IPlugVST* vst = dynamic_cast<IPlugVST*>(this);
  gHostCallback = vst->mHostCallback;
#ifdef OS_OSX
  if (gHostCallback) SWELL_RegisterCustomControlCreator((SWELL_ControlCreatorProc) gHostCallback (NULL, 0xdeadbeef, 0xdeadf00d,0,(void*)"Mac_CustomControlCreator", 0.0));
#endif
  
  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kAzimuth)->InitDouble("Azimuth", 0, -180., 180., 0.1, "");
  GetParam(kElevation)->InitDouble("Elevation", 0, 0., 90., 0.1, "");
  GetParam(kRadius)->InitDouble("Radius", 1, 0., 1., 0.1, "");

  OnGUICreated();
  
  //MakePreset("preset 1", ... );
  MakeDefaultPreset("-", kNumPrograms);
}

ReaperPlugin::~ReaperPlugin()
{
//  if (mHWND)
//    DestroyWindow(mHWND);
#ifdef OS_OSX
  if (gHostCallback) SWELL_UnregisterCustomControlCreator((SWELL_ControlCreatorProc)gHostCallback(NULL, 0xdeadbeef, 0xdeadf00d,0,(void*)"Mac_CustomControlCreator", 0.0));
#endif
}

void ReaperPlugin::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
}

void ReaperPlugin::Reset()
{
}

static WDL_DLGRET mainProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  ReaperPlugin *_this = nullptr;
  
  switch (uMsg)
  {
    case WM_INITDIALOG:
      _this = (ReaperPlugin*) lParam;
      SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
      _this->mHWND = hwndDlg;
      for (auto p = 0; p < _this->NParams(); p++)
      {
        SetWindowLong(GetDlgItem(hwndDlg, IDC_AZI + p), 0, 1);
        double dp = _this->GetParam(p)->GetDefaultNormalized() * 1000.;
        //need to consider shape, db etc.
        SendDlgItemMessage(hwndDlg, IDC_AZI + p, TBM_SETTIC, 0, (LPARAM)(dp+0.5));
      }
      
      SetTimer(hwndDlg, 2, 200, NULL);
      ShowWindow(hwndDlg, SW_SHOWNA);
      return 0;
    case WM_USER+6606:
      _this = (ReaperPlugin *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

      for (auto p = 0; p < _this->NParams(); p++)
      {
        char buf[512];
        _this->GetParam(p)->GetDisplayForHost(buf);
        SetDlgItemText(hwndDlg, IDC_AZI_EDIT + p, buf);
        SendDlgItemMessage(hwndDlg, IDC_AZI + p, TBM_SETPOS, 0, (LPARAM)((_this->GetParam(p)->GetNormalized() * 1000.) + 0.5));
      }
      return 0;
    case WM_NOTIFY:
      break;
    case WM_TIMER:
      if (wParam == 2)
      {
        SendMessage(hwndDlg, WM_USER+6606, 0, 0);
      }
      return 0;
    case WM_COMMAND:
      return 0;
    case WM_HSCROLL:
    case WM_VSCROLL:
    {
      _this = (ReaperPlugin *) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

      char buf[512];
      auto pos = SendMessage((HWND) lParam, TBM_GETPOS, 0, 0);

      for (auto p = 0; p < _this->NParams(); p++)
      {
        if ((HWND) lParam == GetDlgItem(hwndDlg, IDC_AZI + p))
        {
          _this->BeginInformHostOfParamChange(p); // should be on mouse down

          _this->SetParameterFromGUI(p, pos/1000.0);
          
          if (LOWORD(wParam) == SB_ENDSCROLL)
            _this->EndInformHostOfParamChange(p); // should be on mouse up
          
          _this->GetParam(p)->GetDisplayForHost(buf);
          SetDlgItemText(hwndDlg, IDC_AZI_EDIT + p, buf);
          
          break;
        }
      }
    }
      return 0;
    case WM_DESTROY:
      return 0;
    case WM_LBUTTONDOWN:
      return 0;
  }
  return 0;
}

void* ReaperPlugin::OpenWindow(void* handle)
{
  return CreateDialogParam(0, MAKEINTRESOURCE(IDD_DIALOG1),(HWND)handle, mainProc,(LPARAM) this);
}

void ReaperPlugin::CloseWindow()
{
  if (mHWND)
    DestroyWindow(mHWND);
}

#ifndef OS_WIN
#include "swell-dlggen.h"
#include "ReaperPlugin.rc_mac_dlg"
//#include "swell-menugen.h"
//#include "sample_project.rc_mac_menu"
#endif
