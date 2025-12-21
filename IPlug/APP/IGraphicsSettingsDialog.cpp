/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IGraphicsSettingsDialog.h"
#include "config.h"
#include "resource.h"
#include "IControls.h"

#ifdef OS_WIN
#include "asio.h"
#endif

using namespace iplug;
using namespace igraphics;

namespace iplug {
namespace igraphics {
extern IGraphics* MakeGraphics(IGEditorDelegate& dlg, int w, int h, int fps = 0, float scale = 1.);
}
}

static IColor sColor = COLOR_RED;

/** Reaper extension base class interface */
class IGraphicsSettings : public IGEditorDelegate
{
public:
  IGraphicsSettings()
  : IGEditorDelegate(0)
  {
    mTimer = std::unique_ptr<Timer>(Timer::Create(std::bind(&IGraphicsSettings::OnTimer, this, std::placeholders::_1), IDLE_TIMER_RATE));
    
    mMakeGraphicsFunc = [&]() {
      return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
    };
    
    mLayoutFunc = [&](IGraphics* pGraphics) {
      auto bounds = pGraphics->GetBounds();
      if (pGraphics->NControls())
      {
        pGraphics->GetControl(0)->SetTargetAndDrawRECTs(bounds);
        pGraphics->GetControl(2)->SetRECT(bounds.GetPadded(-20));
        return;
      }
      
      pGraphics->SetLayoutOnResize(true);
      pGraphics->AttachPanelBackground(COLOR_GRAY);
      pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
      pGraphics->AttachControl(new ITextControl(bounds.GetFromTop(100), "Settings", IText(50)));
      pGraphics->AttachControl(new IVButtonControl(bounds.GetPadded(-20), SplashClickActionFunc, "Hello"))
      ->SetAnimationEndActionFunction([](IControl* pCaller){
        pCaller->GetUI()->PromptForColor(sColor, "color", [](const IColor& result){
          
        });
      });
    };
  }
  
  virtual ~IGraphicsSettings()
  {
  }

  //IEditorDelegate
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override {}; // NO-OP
  void EndInformHostOfParamChangeFromUI(int paramIdx) override {}; // NO-OP
  void OnParentWindowResize(int width, int height) override {
    if (GetUI())
    {
      GetUI()->Resize(width, height, 1.f, false);
    }
  }

  virtual void OnIdle() {}; // NO-OP

private:
  void OnTimer(Timer& t)
  {
    OnIdle();
  }
  
  std::unique_ptr<Timer> mTimer;
};

extern HWND gPrefsHWND;

WDL_DLGRET IGraphicsSettingsDialog::SettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  IGraphicsSettingsDialog* pAppDialog = static_cast<IGraphicsSettingsDialog *>(IPlugAPPHost::GetSettingsDialog());
  
  return pAppDialog->SettingsProcess(hwndDlg, uMsg, wParam, lParam);
}

std::unique_ptr<IGraphicsSettings> gIGraphicsSettings;

WDL_DLGRET IGraphicsSettingsDialog::SettingsProcess(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg)
  {
    case WM_CREATE:
      gPrefsHWND = hwndDlg;
      mPreviousSettings = mSettings;
      gIGraphicsSettings = std::make_unique<IGraphicsSettings>();
      gIGraphicsSettings->OpenWindow(gPrefsHWND);
      return TRUE;
    case WM_CLOSE:
      gPrefsHWND = 0;
      DestroyWindow(hwndDlg);
      gIGraphicsSettings->CloseWindow();
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
          gIGraphicsSettings->OnParentWindowResize(static_cast<int>(r.right / scale), static_cast<int>(r.bottom / scale));
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

void IGraphicsSettingsDialog::Refresh()
{
}
