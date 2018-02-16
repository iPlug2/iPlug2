#pragma once
#include <cstring>
#include "wdltypes.h"
#include "wdlstring.h"

#include "IPlugPlatform.h"

#ifdef OS_MAC
#include "IGraphicsMac.h"
#include "swell.h"
#define PostQuitMessage SWELL_PostQuitMessage
#elif defined (OS_WIN)
#include "IGraphicsWin.h"
#elif defined (OS_LINUX)
#include "IGraphicsLinux.h"
#include "swell.h"
#define PostQuitMessage SWELL_PostQuitMessage
#endif

#include "resources/resource.h"
#include "IPlugParameter.h"
#include "IControls.h"

extern HWND gHWND;
extern HINSTANCE gHINSTANCE;
extern WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

#define UI_WIDTH 700
#define UI_HEIGHT 700
#define FPS 60

const int gSizes[4] = { 100, 300, 600, 1000 };

class IGraphicsTest : public IDelegate
{
public:
  void init()
  {
#ifdef OS_MAC
    IGraphicsMac* pGraphics = new IGraphicsMac(*this, UI_WIDTH, UI_HEIGHT, FPS);
    pGraphics->SetBundleID("com.OliLarkin.app.IGraphicsTest");
    pGraphics->CreateMetalLayer();
#elif defined OS_WIN
    IGraphicsWin* pGraphics = new IGraphicsWin(*this, UI_WIDTH, UI_HEIGHT, FPS);
    pGraphics->SetPlatformInstance(gHINSTANCE);
#elif defined OS_LINUX
    
#endif
    
#ifndef OS_LINUX
    pGraphics->OpenWindow((void*) gHWND);
    pGraphics->AttachPanelBackground(COLOR_RED);
    pGraphics->HandleMouseOver(true);
    //  pGraphics->EnableLiveEdit(true);
    
    const int nRows = 2;
    const int nColumns = 2;
    IRECT bounds = pGraphics->GetBounds();
    
    IRECT cellRect = bounds.GetGridCell(0, nRows, nColumns);
    pGraphics->AttachControl(new IVSwitchControl(*this, cellRect, kNoParameter, [pGraphics, this](IControl* pCaller)
                                                 {
                                                   pGraphics->Resize(gSizes[mSizeIdx], gSizes[mSizeIdx], 1.);
                                                   mSizeIdx = mSizeIdx + 1;
                                                   mSizeIdx %= 4;
                                                 }));
    //  for (int i = 0; i < nRows * nColumns; i++)
    //  {
    //    IRECT cellBounds = bounds.GetGridCell(i, nRows, nColumns).GetPadded(-5.);
    //    pGraphics->AttachControl(new IVKnobControl(dummyDelegate, cellBounds, kGain));
    //  }
    
    mGraphics = pGraphics;
#endif
  }
  
  void ResizeWindow(HWND hWnd, int w, int h, bool centerOnScreen)
  {
    int x, y, cx, cy;
    RECT clientRect, windowRect;
    GetClientRect(hWnd, &clientRect);
    GetWindowRect(hWnd, &windowRect);
    const int screenwidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenheight = GetSystemMetrics(SM_CYSCREEN);
    
    if(centerOnScreen)
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
#define TITLEBAR_BODGE 22
      y -= TITLEBAR_BODGE;
      cy += TITLEBAR_BODGE;
#endif
    }
    
    SetWindowPos(hWnd, 0, x, y, cx, cy, 0);
  }
  
  //IDelegate
  void SetControlValueFromDelegate(int controlIdx, double normalizedValue) override {};
  void SendParameterValueToUIFromDelegate(int paramIdx, double value, bool normalized) override {};
  
  const IParam* GetParamObjectFromUI(int paramIdx) override { return nullptr; }
  void SetParameterValueFromUI(int paramIdx, double value) override { DBGMSG("SetParameterValueFromUI p %i %f\n", paramIdx, value); }
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override { ; }
  void EndInformHostOfParamChangeFromUI(int paramIdx) override { ; }
  void ResizeGraphicsFromUI() override { ResizeWindow(gHWND, mGraphics->Width(), mGraphics->Height(), false ); }
  
private:
  IGraphics* mGraphics = nullptr;
  int mSizeIdx = 0;
};
