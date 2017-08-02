#ifndef _IPLUGCUSTOMUI_H_
#define _IPLUGCUSTOMUI_H_

#if WINDOWS_VERSION
  #define VC_EXTRALEAN // Exclude rarely-used stuff from Windows headers
#endif

#include "EditorInterface.h"
#include "ProcessInterface.h"
#include "../IPlugRTAS.h"

#if MAC_VERSION

extern void* attachSubWindow (void* hostWindowRef, IGraphics* pGraphics);
extern void removeSubWindow (void* cocoaHostWindow, IGraphics* pGraphics);

class IPlugCustomUI : public EditorInterface
{
public:
  IPlugCustomUI(void *processPtr);
  ~IPlugCustomUI();

  bool Open(void *hwin, short leftOffset, short topOffset);
  bool Close(void);
  void GetRect(short *left, short *top, short *right, short *bottom);
  ProcessInterface *GetProcessPtr() { return (ProcessInterface*)mProcess; }
  void SetControlHighlight(long controlIndex, short isHighlighted, short color);
  void GetControlIndexFromPoint(long x, long y, long *aControlIndex);
  void Draw(long left, long top, long right, long bottom);

protected:
  WindowRef mLocalWindow; // carbon PT window

  IPlugRTAS*  mPlug;
  IGraphics*  mGraphics;
};

#elif WINDOWS_VERSION

// Callback to Win32 Plug-in window - processes Windows messages.
LRESULT CALLBACK IPlugMainWindow( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

class IPlugCustomUI : public EditorInterface
{
  friend LRESULT CALLBACK IPlugMainWindow(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
public:
  IPlugCustomUI(void *processPtr);
  ~IPlugCustomUI();

  bool Open(void *hwin, short leftOffset, short topOffset);
  bool Close(void);
  bool Init();
  long UpdateGraphicControl(long index, long value);
  void Draw(long left, long top, long right, long bottom);
  void GetRect(short *left, short *top, short *right, short *bottom);
  ProcessInterface *GetProcessPtr() { return (ProcessInterface*)mProcess; }
  void SetControlHighlight(long controlIndex, short isHighlighted, short color);
  void GetControlIndexFromPoint(long x, long y, long *aControlIndex);

  HWND GetParentHWND() { return mPluginWindow; }
protected:
  HINSTANCE mPlugInWndHINST;  // plug-in's HINSTANCE

  HWND    mPluginWindow;    // main window created by Pro Tools
  HWND    mLocalPIWin;    // sub-window that has local message loop

  WNDCLASSEX  mLocalWinClass;   // Window class struct for local window
  ATOM    mLocalWindowID;

  long    mHeaderHeight;  // Height of header created by Pro Tools in Plug-in window
  long    mHeaderWidth; // Width of header created by Pro Tools in Plug-in window
  RECT    mPIRect;
  IPlugRTAS*  mPlug;
  IGraphics*  mGraphics;
};

#endif

IPlugCustomUI* CreateIPlugCustomUI(void *processPtr);

#endif  // _IPLUGCUSTOMUI_H_