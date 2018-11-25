#include "IGraphicsTest.h"
#include "resources/resource.h"

#include "IGraphics_include_in_plug_src.h"

#include "Test/TestControls.h"
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
  pGraphics->AttachPanelBackground(COLOR_GRAY);
  pGraphics->AttachCornerResizer();
  pGraphics->EnableLiveEdit(true);
  //pGraphics->HandleMouseOver(true);
//  pGraphics->ShowControlBounds(true);
  pGraphics->LoadFont(ROBOTTO_FN);
  pGraphics->LoadFont(MONTSERRAT_FN);
  //
  IRECT bounds = pGraphics->GetBounds();
  IBitmap knobBitmap = pGraphics->LoadBitmap(PNGKNOB_FN, 60);
  int rows = 4;
  int cols = 4;
  int cellIdx = 0;

  pGraphics->AttachControl(new IBKnobControl(*this, bounds.GetGridCell(cellIdx++, rows, cols), knobBitmap, -1));
  pGraphics->AttachControl(new TestGradientControl(*this, bounds.GetGridCell(cellIdx++, rows, cols), -1));
  pGraphics->AttachControl(new TestPolyControl(*this, bounds.GetGridCell(cellIdx++, rows, cols), -1));
  pGraphics->AttachControl(new TestArcControl(*this, bounds.GetGridCell(cellIdx++, rows, cols), -1));
  pGraphics->AttachControl(new TestTextControl(*this, bounds.GetGridCell(cellIdx++, rows, cols)));

//  pGraphics->AttachControl(new ILambdaControl(*this, bounds.GetGridCell(cellIdx++, rows, cols),
//                                              [&](IControl* pCaller, IGraphics& g, IRECT& b, IMouseInfo& mi, double t)
//                                              {
//                                                static IBitmap knobBitmap = g.LoadBitmap("smiley.png");
//
//                                                g.DrawBitmap(knobBitmap, b, 0, 0);
//                                                g.DrawCircle(COLOR_RED, b.MW(), b.MH(), 30);
//                                                //                                                g.DrawFittedBitmap(knobBitmap, b);
//                                              }));

#if 1
  pGraphics->AttachControl(new ITextControl(*this, {10, 10, 200, 30}, "One!", {12, COLOR_WHITE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignBottom}));
  pGraphics->AttachControl(new ITextControl(*this, {30, 30, 200, 60}, "Two!", {18, COLOR_GREEN, "Montserrat-LightItalic", IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignBottom}));
  pGraphics->AttachControl(new ITextControl(*this, {80, 50, 200, 80.}, "Three!", {24, COLOR_RED, "Roboto-Regular", IText::kStyleItalic, IText::kAlignFar, IText::kVAlignBottom}));
  pGraphics->AttachControl(new ITextControl(*this, {120, 60, 300, 120}, "Four!", {40, COLOR_ORANGE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignBottom}));
  pGraphics->AttachControl(new ITextControl(*this, {10, 100, 370, 170}, "hello iplug!", {50, COLOR_BLUE, "Roboto-Regular", IText::kStyleNormal, IText::kAlignCenter, IText::kVAlignBottom}));
#endif
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
