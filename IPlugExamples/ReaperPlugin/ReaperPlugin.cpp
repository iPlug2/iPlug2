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
  ReaperPlugin *_this = (ReaperPlugin*) lParam;
  
  if(!_this)
    return 0;
  
  switch (uMsg)
  {
    case WM_INITDIALOG:
      _this->mHWND=hwndDlg;
      for (auto p = 0; p < _this->NParams(); p++)
      {
        SetWindowLong(GetDlgItem(hwndDlg,IDC_AZI), 0, 1);
        double dp=_this->GetParam(p)->GetDefaultNormalized()*1000.0;
        
        //if (param_infos[x].slidershape>1.0) dp=sliderscale_sq(dp,1,param_infos[x].slidershape);
//
//        if (dp<0) dp = 0;
//        else if (dp>1000)dp = 1000;
        SendDlgItemMessage(hwndDlg, IDC_AZI, TBM_SETTIC, 0, (LPARAM)(dp+0.5));
      }
      
      SetTimer(hwndDlg, 2, 200, NULL);
      ShowWindow(hwndDlg, SW_SHOWNA);
    case WM_USER+6606:
      for (auto p = 0; p < _this->NParams(); p++)
      {
//        if (param_infos[x].ui_id_lbl)
//        {
//          char buf[512];
//          format_parm(x,m_parms[x],buf);
//          SetDlgItemText(hwndDlg,param_infos[x].ui_id_lbl,buf);
//        }
//        if (param_infos[x].ui_id_slider)
//        {
//          double val;
//          if (param_infos[x].minval == USE_DB) val=(DB2SLIDER(VAL2DB(m_parms[x])));
//          else if (param_infos[x].slidershape>1.0)
//            val=sliderscale_sq(m_parms[x]*1000.0/param_infos[x].parm_maxval,1,param_infos[x].slidershape);
//          else val=(m_parms[x]*1000.0)/param_infos[x].parm_maxval;
//          SendDlgItemMessage(hwndDlg,param_infos[x].ui_id_slider,TBM_SETPOS,0,(LPARAM)val);
//        }
//        }
      }
      return 0;
    case WM_NOTIFY:
      break;
    case WM_TIMER:
      return 0;
    case WM_COMMAND:
      return 0;
    case WM_HSCROLL:
    case WM_VSCROLL:
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
