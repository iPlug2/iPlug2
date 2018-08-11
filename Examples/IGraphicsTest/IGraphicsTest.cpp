#include "IGraphicsTest.h"
#include "resources/resource.h"

#include "IGraphics_include_in_plug_src.h"

#include "IGraphicsTest_controls.h"

#include "IControls.h"
#include "config.h"

extern HWND gHWND;
extern HINSTANCE gHINSTANCE;
extern UINT gScrollMessage;
extern IGraphicsTest* gIGraphicsTest;

IGraphicsTest::IGraphicsTest()
: IGEditorDelegate(0)
{
}

IGraphics* IGraphicsTest::CreateGraphics()
{
  return MakeGraphics(*this, UI_WIDTH, UI_HEIGHT, 60);
}

void IGraphicsTest::LayoutUI(IGraphics* pGraphics)
{
  pGraphics->AttachCornerResizer();
  //pGraphics->EnableLiveEdit(true);
  //pGraphics->HandleMouseOver(true);
  pGraphics->LoadFont(ROBOTTO_FN);
  pGraphics->LoadFont(MONTSERRAT_FN);
  //
  IRECT bounds = pGraphics->GetBounds();
  IBitmap knobBitmap = pGraphics->LoadBitmap(PNGKNOB_FN, 60);
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
