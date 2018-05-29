#include "IGraphicsTest.h"
#include "resources/resource.h"

#include "IGraphics_include_in_plug_src.h"

#include "IGraphicsTest_controls.h"

#include "IControls.h"

extern HWND gHWND;
extern HINSTANCE gHINSTANCE;
extern UINT gScrollMessage;
extern IGraphicsTest* gIGraphicsTest;

IGraphicsTest::IGraphicsTest()
: IGraphicsEditorDelegate(0)
{
  IGraphics* pGraphics = MakeGraphics(*this, UI_WIDTH, UI_HEIGHT, 60);

  pGraphics->AttachPanelBackground(COLOR_RED);
  pGraphics->HandleMouseOver(true);
  IBitmap knobBitmap = pGraphics->LoadBitmap("knob.png", 60);
  pGraphics->LoadFont("Roboto-Regular.ttf");
  pGraphics->LoadFont("Montserrat-LightItalic.ttf");
  
  pGraphics->AttachControl(new IGradientControl(*this, IRECT(20, 20, 150, 120), -1));
  pGraphics->AttachControl(new IPolyControl(*this, IRECT(20, 200, 150, 330), -1));
  pGraphics->AttachControl(new IArcControl(*this, IRECT(220, 20, 320, 120), -1));
  pGraphics->AttachControl(new IBKnobControl(*this, 220, 200, knobBitmap, -1));
  pGraphics->AttachControl(new RandomTextControl(*this, IRECT(220, 260, 380, 380)));
  pGraphics->GetControl(2)->SetValueFromDelegate((double) rand() / RAND_MAX);
  pGraphics->GetControl(3)->SetValueFromDelegate((double) rand() / RAND_MAX);
  pGraphics->GetControl(4)->SetValueFromDelegate((double) rand() / RAND_MAX);

  AttachGraphics(pGraphics);
}

void IGraphicsTest::SetParameterValueFromUI(int paramIdx, double value)
{
  DBGMSG("SetParameterValueFromUI p %i %f\n", paramIdx, value);
}

#ifndef OS_WEB
WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      gHWND = hwndDlg;
      gIGraphicsTest = new IGraphicsTest();
      gIGraphicsTest->OpenWindow(gHWND);
      ShowWindow(gHWND, SW_SHOW);
      RECT r;
      GetWindowRect(gHWND, &r);
      SetWindowPos(gHWND, 0, r.left, r.bottom - UI_HEIGHT - 22, UI_WIDTH, UI_HEIGHT + 2, 0);
      
      return 1;
    }
    case WM_DESTROY:
      gHWND = NULL;
      delete gIGraphicsTest;
#ifdef OS_WIN
      PostQuitMessage(0);
#endif
      return 0;
    case WM_CLOSE:
      gIGraphicsTest->CloseWindow();
      DestroyWindow(hwndDlg);
      return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
      case ID_QUIT:
        gIGraphicsTest->CloseWindow();
        DestroyWindow(hwndDlg);
        return 0;
      case ID_ABOUT:
      {
        WDL_String version;
        version.SetFormatted(100, "Built on %s", __DATE__);
        MessageBox(hwndDlg, version.Get(), "IGraphicsTest", MB_OK);
        return 0;
      }
    }
      return 0;
  }
  return 0;
}
#endif
