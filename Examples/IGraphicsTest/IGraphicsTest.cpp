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
  pGraphics->AttachCornerResizer();
//  pGraphics->EnableLiveEdit(true);
  //pGraphics->HandleMouseOver(true);
  pGraphics->LoadFont("Roboto-Regular.ttf");
  pGraphics->LoadFont("Montserrat-LightItalic.ttf");
  //
  IRECT bounds = pGraphics->GetBounds();
  IBitmap knobBitmap = pGraphics->LoadBitmap("knob.png", 60);
  int rows = 4;
  int cols = 4;
  int cellIdx = 0;
  //
  pGraphics->AttachControl(new IGradientControl(*this, bounds.GetGridCell(cellIdx++, rows, cols), -1));
  pGraphics->AttachControl(new IPolyControl(*this, bounds.GetGridCell(cellIdx++, rows, cols), -1));
  pGraphics->AttachControl(new IArcControl(*this, bounds.GetGridCell(cellIdx++, rows, cols), -1));
  pGraphics->AttachControl(new IBKnobControl(*this, bounds.GetGridCell(cellIdx++, rows, cols), knobBitmap, -1));
  pGraphics->AttachControl(new RandomTextControl(*this, bounds.GetGridCell(cellIdx++, rows, cols)));

  pGraphics->AttachControl(new ILambdaControl(*this, bounds.GetGridCell(cellIdx++, rows, cols),
                                              [&](IControl* pCaller, IGraphics& g, IRECT& b, IMouseInfo& mi, double t)
                                              {
                                                static IBitmap knobBitmap = g.LoadBitmap("smiley.png");
                                                
                                                g.DrawBitmap(knobBitmap, b, 0, 0);
//                                                g.DrawFittedBitmap(knobBitmap, b);
                                              }));

  AttachGraphics(pGraphics);
}

void IGraphicsTest::SendParameterValueFromUI(int paramIdx, double value)
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
#else
      SWELL_PostQuitMessage(hwndDlg);
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
