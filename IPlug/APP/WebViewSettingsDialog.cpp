/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "WebViewSettingsDialog.h"
#include "config.h"
#include "resource.h"

#ifdef OS_WIN
#include "asio.h"
#endif

#include "IPlugWebViewEditorDelegate.h"

using namespace iplug;

/** Reaper extension base class interface */
class WebViewSettings : public WebViewEditorDelegate
{
public:
  WebViewSettings()
  : WebViewEditorDelegate(0)
  {
    SetEditorSize(300, 400);
    
    mEditorInitFunc = [&]() {
      LoadHTML(R"(
<h1>Audio Settings</h1>

<h1>MIDI Settings</h1>
<button>OK</button>
<button>Apply</button>
<button>Cancel</button>
)");
      EnableScroll(false);
    };
  }


  //IEditorDelegate
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override {}; // NO-OP
  void EndInformHostOfParamChangeFromUI(int paramIdx) override {}; // NO-OP
};

extern HWND gPrefsHWND;

WDL_DLGRET WebViewSettingsDialog::SettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  WebViewSettingsDialog* pAppDialog = static_cast<WebViewSettingsDialog *>(IPlugAPPHost::GetSettingsDialog());
  
  return pAppDialog->SettingsProcess(hwndDlg, uMsg, wParam, lParam);
}

std::unique_ptr<WebViewSettings> gWebViewSettingsDialog;

WDL_DLGRET WebViewSettingsDialog::SettingsProcess(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_CREATE:
      gPrefsHWND = hwndDlg;
      mPreviousSettings = mSettings;
      gWebViewSettingsDialog = std::make_unique<WebViewSettings>();
      gWebViewSettingsDialog->OpenWindow(gPrefsHWND);
      return TRUE;
    case WM_CLOSE:
      gPrefsHWND = 0;
      DestroyWindow(hwndDlg);
      gWebViewSettingsDialog->CloseWindow();
      return TRUE;
    case WM_SIZE:
    {
      switch (LOWORD(wParam))
      {
        case SIZE_RESTORED:
        case SIZE_MAXIMIZED:
        {
          RECT r;
          GetClientRect(hwndDlg, &r);
          float scale = 1.f;
#ifdef OS_WIN
          scale = GetScaleForHWND(hwndDlg);
#endif
          gWebViewSettingsDialog->OnParentWindowResize(static_cast<int>(r.right / scale), static_cast<int>(r.bottom / scale));
          return 1;
        }
        default:
          return 0;
      }
    }
    default:
      return FALSE;
  }
  return TRUE;
}

void WebViewSettingsDialog::Refresh()
{
}
