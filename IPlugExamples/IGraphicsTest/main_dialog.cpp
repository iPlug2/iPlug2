#include "IGraphicsTest.h"
#ifdef OS_MAC
#include "IGraphicsMac.h"
#else
#include "IGraphicsWin.h"
#endif
#include "IControls.h"

extern void CenterWindow(HWND hwnd, int w, int h);

MyDelegate dummyDelegate;

enum EParams
{
  kGain = 0,
  kNumParams
};

void CreateGraphics()
{
#ifdef OS_MAC
  IGraphicsMac* pGraphics = new IGraphicsMac(dlg, 300, 300, 60);
  pGraphics->SetBundleID("com.OliLarkin.app.IGraphicsTest");
  pGraphics->CreateMetalLayer();
#else
  IGraphicsWin* pGraphics = new IGraphicsWin(dummyDelegate, 300, 300, 60);
  pGraphics->SetPlatformInstance(gHINSTANCE);
#endif

  pGraphics->OpenWindow((void*) gHWND);
  pGraphics->AttachPanelBackground(COLOR_RED);
  
  /*ISVG svg = pGraphics->LoadSVG("resources/img/BefacoBigKnob.svg");*/
  //pGraphics->AttachControl(new IVSVGKnob(dummyDelegate, pGraphics->GetBounds().GetPadded(-20), svg, kGain));

  pGraphics->AttachControl(new IVKnobControl(dummyDelegate, pGraphics->GetBounds().GetPadded(-20), kGain));
}

//static
WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      gHWND = hwndDlg;
      
      CreateGraphics();
      CenterWindow(gHWND, 300, 300);
      ShowWindow(gHWND, SW_SHOW);
     
      return 1;
    }
    case WM_DESTROY:
      gHWND = NULL;
#ifdef OS_WIN
      PostQuitMessage(0);
#else
      SWELL_PostQuitMessage(hwndDlg);
#endif
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
