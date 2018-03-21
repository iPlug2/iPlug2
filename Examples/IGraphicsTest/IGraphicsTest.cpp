#include "IGraphicsTest.h"

#include "IPlugParameter.h"
#include "IControls.h"
#include "IVDropDownListControl.h"
#include "Easing.h"

#define MAC_TITLEBAR_BODGE 22

void IGraphicsTest::init()
{
#ifdef OS_MAC
  IGraphicsMac* pGraphics = new IGraphicsMac(*this, UI_WIDTH, UI_HEIGHT, UI_FPS);
  pGraphics->SetBundleID("com.OliLarkin.app.IGraphicsTest");
  pGraphics->CreateMetalLayer();
#elif defined OS_WIN
  IGraphicsWin* pGraphics = new IGraphicsWin(*this, UI_WIDTH, UI_HEIGHT, UI_FPS);
  pGraphics->SetPlatformInstance(gHINSTANCE);
#elif defined OS_LINUX

#endif

#ifndef OS_LINUX
  pGraphics->OpenWindow((void*)gHWND);
  pGraphics->AttachPanelBackground(COLOR_RED);
  pGraphics->HandleMouseOver(true);
//  pGraphics->EnableLiveEdit(true);

  const int nRows = 10;
  const int nColumns = 10;
  IRECT bounds = pGraphics->GetBounds();
  

  for (int i = 0; i < 50; i++)
  {
    IRECT cellBounds = bounds.GetGridCell(i, nRows, nColumns).GetPadded(-5.);
    IRECT endRect = bounds.GetGridCell(i + 50, nRows, nColumns).GetPadded(-5.);;
    
    IVSwitchControl* pSw = new IVSwitchControl(*this, cellBounds);
    pGraphics->AttachControl(pSw);

//    IAnimationFunction animationFunction = [pGraphics, cellBounds, endRect](IControl* pCaller)
//    {
//      auto progress = pCaller->GetAnimationProgress();
//      printf("%f progress\n", progress);
//
//      if(progress > 1.)
//      {
//        pCaller->EndAnimation();
//        return;
//      }
//      
//      IRECT nb;
//      IRECT::LinearInterpolateBetween(cellBounds, endRect, nb, IEaseSineOut(progress));
//      pCaller->SetTargetAndDrawRECTs(nb);
//      pCaller->Animate(progress);
//    };
//
//    IActionFunction actionFunction = [animationFunction, pGraphics](IControl* pCaller)
//    {
//      pCaller->SetAnimation(animationFunction, 1000);
//    };
//    
//    pSw->SetActionFunction(actionFunction);
  }

#endif
}

void IGraphicsTest::ResizeWindow(HWND hWnd, int w, int h, bool centerOnScreen)
{
  int x, y, cx, cy;
  RECT clientRect, windowRect;
  GetClientRect(hWnd, &clientRect);
  GetWindowRect(hWnd, &windowRect);
  const int screenwidth = GetSystemMetrics(SM_CXSCREEN);
  const int screenheight = GetSystemMetrics(SM_CYSCREEN);

  if (centerOnScreen)
  {
    x = (screenwidth / 2) - (w / 2);
    y = (screenheight / 2) - (h / 2);
    cx = w + (windowRect.right - windowRect.left) - clientRect.right;
    cy = h + (windowRect.bottom - windowRect.top) - clientRect.bottom;
  }
  else
  {
    x = windowRect.left;
    y = windowRect.bottom - h;
    cx = w;
    cy = h;
#ifdef OS_MAC
    y -= MAC_TITLEBAR_BODGE;
    cy += MAC_TITLEBAR_BODGE;
#endif
  }

  SetWindowPos(hWnd, 0, x, y, cx, cy, 0);
}

void IGraphicsTest::SetParameterValueFromUI(int paramIdx, double value)
{
  DBGMSG("SetParameterValueFromUI p %i %f\n", paramIdx, value);
}

void IGraphicsTest::ResizeGraphicsFromUI()
{
  ResizeWindow(gHWND, mGraphics->Width(), mGraphics->Height(), false );
}

extern IGraphicsTest gIGraphicsTest;

WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      gHWND = hwndDlg;
      gIGraphicsTest.init();
      gIGraphicsTest.ResizeWindow(gHWND, UI_WIDTH, UI_HEIGHT, true);
      ShowWindow(gHWND, SW_SHOW);
      
      return 1;
    }
    case WM_DESTROY:
      gHWND = NULL;
      PostQuitMessage(0);
      return 0;
    case WM_CLOSE:
      DestroyWindow(hwndDlg);
      return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam))
    {
      case ID_QUIT:
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
