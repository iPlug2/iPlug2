#ifndef _IGRAPHICS_
#define _IGRAPHICS_

#include "IPlugStructs.h"
#include "IPopupMenu.h"
#include "IControl.h"
#include "../lice/lice.h"

// Specialty stuff for calling in to Reaper for Lice functionality.
#ifdef REAPER_SPECIAL
  #include "../IPlugExt/ReaperExt.h"
  #define _LICE ReaperExt
#else
  #define _LICE
#endif

#ifdef AAX_API
  #include "AAX_IViewContainer.h"

  static uint32_t GetAAXModifiersFromIMouseMod(const IMouseMod* pMod)
  {
    uint32_t aax_mods = 0;

    if (pMod->A) aax_mods |= AAX_eModifiers_Option; // ALT Key on Windows, ALT/Option key on mac

    #ifdef OS_WIN
    if (pMod->C) aax_mods |= AAX_eModifiers_Command;
    #else
    if (pMod->C) aax_mods |= AAX_eModifiers_Control;
    if (pMod->R) aax_mods |= AAX_eModifiers_Command;
    #endif
    if (pMod->S) aax_mods |= AAX_eModifiers_Shift;
    if (pMod->R) aax_mods |= AAX_eModifiers_SecondaryButton;
    
    return aax_mods;
  }
#endif

#define MAX_PARAM_LEN 32

class IPlugBase;
class IControl;
class IParam;

class IGraphics
{
public:
  void PrepDraw();    // Called once, when the IGraphics class is attached to the IPlug class.

  bool IsDirty(IRECT* pR);        // Ask the plugin what needs to be redrawn.
  bool Draw(IRECT* pR);           // The system announces what needs to be redrawn.  Ordering and drawing logic.
  virtual bool DrawScreen(IRECT* pR) = 0;  // Tells the OS class to put the final bitmap on the screen.

  // Methods for the drawing implementation class.
  bool DrawBitmap(IBitmap* pBitmap, IRECT* pDest, int srcX, int srcY, const IChannelBlend* pBlend = 0);
  bool DrawRotatedBitmap(IBitmap* pBitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg = 0, const IChannelBlend* pBlend = 0);
  bool DrawRotatedMask(IBitmap* pBase, IBitmap* pMask, IBitmap* pTop, int x, int y, double angle, const IChannelBlend* pBlend = 0);
  bool DrawPoint(const IColor* pColor, float x, float y, const IChannelBlend* pBlend = 0, bool antiAlias = false);
  // Live ammo!  Will crash if out of bounds!  etc.
  bool ForcePixel(const IColor* pColor, int x, int y);
  bool DrawLine(const IColor* pColor, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool DrawArc(const IColor* pColor, float cx, float cy, float r, float minAngle, float maxAngle, const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool DrawCircle(const IColor* pColor, float cx, float cy, float r, const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool RoundRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend, int cornerradius, bool aa);
  bool FillRoundRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend, int cornerradius, bool aa);

  bool FillIRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend = 0);
  bool FillCircle(const IColor* pColor, int cx, int cy, float r, const IChannelBlend* pBlend = 0, bool antiAlias = false);
  bool FillIConvexPolygon(const IColor* pColor, int* x, int* y, int npoints, const IChannelBlend* pBlend = 0);
  bool FillTriangle(const IColor* pColor, int x1, int y1, int x2, int y2, int x3, int y3, IChannelBlend* pBlend);

  bool DrawIText(IText* pTxt, char* str, IRECT* pR, bool measure = false);
  virtual bool MeasureIText(IText* pTxt, char* str, IRECT* pR) { return DrawIText(pTxt, str, pR, true); } ;

  IColor GetPoint(int x, int y);
  void* GetData();

  void PromptUserInput(IControl* pControl, IParam* pParam, IRECT* pTextRect);

  // Methods for the OS implementation class.

  virtual void ForceEndUserEdit() = 0;
  virtual void Resize(int w, int h);
  virtual bool WindowIsOpen() { return (GetWindow()); }
  virtual const char* GetGUIAPI() { return ""; };

  // type can be MB_OKCANCEL/MB_YESNO/MB_YESNOCANCEL, return val is either IDOK, IDCANCEL or IDNO
  virtual int ShowMessageBox(const char* pText, const char* pCaption, int type) = 0;

  // helper
  IPopupMenu* CreateIPopupMenu(IPopupMenu* pMenu, int x, int y)
  {
    IRECT tempRect = IRECT(x,y,x,y);
    return CreateIPopupMenu(pMenu, &tempRect);
  }

  virtual IPopupMenu* CreateIPopupMenu(IPopupMenu* pMenu, IRECT* pTextRect) = 0;
  virtual void CreateTextEntry(IControl* pControl, IText* pText, IRECT* pTextRect, const char* pString = "", IParam* pParam = 0) = 0;

  void SetFromStringAfterPrompt(IControl* pControl, IParam* pParam, char *txt);
  virtual void HostPath(WDL_String* pPath) = 0;   // Full path to host executable.
  virtual void PluginPath(WDL_String* pPath) = 0; // Full path to plugin dll.
  virtual void DesktopPath(WDL_String* pPath) = 0; // Full path to user's desktop.
  
  //Windows7: %LOCALAPPDATA%\
  //Windows XP/Vista: %USERPROFILE%\Local Settings\Application Data\
  //OSX: ~/Library/Application Support/
  virtual void AppSupportPath(WDL_String* pPath) = 0;
  
  // Run the "open file" or "save file" dialog.  Default to host executable path.
  virtual void PromptForFile(WDL_String* pFilename, EFileAction action = kFileOpen, WDL_String* pDir = 0, char* extensions = 0) = 0;  // extensions = "txt wav" for example.
  virtual bool PromptForColor(IColor* pColor, char* prompt = 0) = 0;

  virtual bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0) = 0;

  // Strict (default): draw everything within the smallest rectangle that contains everything dirty.
  // Every control is guaranteed to get no more than one Draw() call per cycle.
  // Fast: draw only controls that intersect something dirty.
  // If there are overlapping controls, fast drawing can generate multiple Draw() calls per cycle
  // (a control may be asked to draw multiple parts of itself, if it intersects with something dirty.)
  void SetStrictDrawing(bool strict);

  virtual void* OpenWindow(void* pParentWnd) = 0;
  virtual void* OpenWindow(void* pParentWnd, void* pParentControl, short leftOffset = 0, short topOffset = 0) { return 0; } // For Carbon / RTAS... mega ugh!

  virtual void AttachSubWindow(void* hostWindowRef) {};
  virtual void RemoveSubWindow() {};

  virtual void CloseWindow() = 0;
  virtual void* GetWindow() = 0;

  ////////////////////////////////////////

  IGraphics(IPlugBase* pPlug, int w, int h, int refreshFPS = 0);
  virtual ~IGraphics();

  int Width() { return mWidth; }
  int Height() { return mHeight; }
  int FPS() { return mFPS; }

  IPlugBase* GetPlug() { return mPlug; }

  IBitmap LoadIBitmap(int ID, const char* name, int nStates = 1, bool framesAreHoriztonal = false);
  IBitmap ScaleBitmap(IBitmap* pSrcBitmap, int destW, int destH);
  IBitmap CropBitmap(IBitmap* pSrcBitmap, IRECT* pR);
  void AttachBackground(int ID, const char* name);
  void AttachPanelBackground(const IColor *pColor);
  void AttachKeyCatcher(IControl* pControl);

  // Returns the control index of this control (not the number of controls).
  int AttachControl(IControl* pControl);

  IControl* GetControl(int idx) { return mControls.Get(idx); }
  int GetNControls() { return mControls.GetSize(); }
  void HideControl(int paramIdx, bool hide);
  void GrayOutControl(int paramIdx, bool gray);

  // Normalized means the value is in [0, 1].
  void ClampControl(int paramIdx, double lo, double hi, bool normalized);
  void SetParameterFromPlug(int paramIdx, double value, bool normalized);
  // For setting a control that does not have a parameter associated with it.
  void SetControlFromPlug(int controlIdx, double normalizedValue);
  
  void SetAllControlsDirty();

  // This is for when the gui needs to change a control value that it can't redraw
  // for context reasons.  If the gui has redrawn the control, use IPlug::SetParameterFromGUI.
  void SetParameterFromGUI(int paramIdx, double normalizedValue);

  // Convenience wrappers.
  bool DrawBitmap(IBitmap* pBitmap, IRECT* pR, int bmpState = 1, const IChannelBlend* pBlend = 0);
  bool DrawRect(const IColor* pColor, IRECT* pR);
  bool DrawVerticalLine(const IColor* pColor, IRECT* pR, float x);
  bool DrawHorizontalLine(const IColor* pColor, IRECT* pR, float y);
  bool DrawVerticalLine(const IColor* pColor, int xi, int yLo, int yHi);
  bool DrawHorizontalLine(const IColor* pColor, int yi, int xLo, int xHi);
  bool DrawRadialLine(const IColor* pColor, float cx, float cy, float angle, float rMin, float rMax, const IChannelBlend* pBlend = 0, bool antiAlias = false);

  void OnMouseDown(int x, int y, IMouseMod* pMod);
  void OnMouseUp(int x, int y, IMouseMod* pMod);
  void OnMouseDrag(int x, int y, IMouseMod* pMod);
  // Returns true if the control receiving the double click will treat it as a single click
  // (meaning the OS should capture the mouse).
  bool OnMouseDblClick(int x, int y, IMouseMod* pMod);
  void OnMouseWheel(int x, int y, IMouseMod* pMod, int d);
  bool OnKeyDown(int x, int y, int key);

  virtual void HideMouseCursor() {};
  virtual void ShowMouseCursor() {};

  int GetParamIdxForPTAutomation(int x, int y);
  int GetLastClickedParamForPTAutomation();

#ifdef AAX_API
  void SetViewContainer(AAX_IViewContainer* viewContainer) { mAAXViewContainer = viewContainer; }
  AAX_IViewContainer* GetViewContainer() { return mAAXViewContainer; }
#endif
//  void DisplayControlValue(IControl* pControl);

  // For efficiency, mouseovers/mouseouts are ignored unless you explicity say you can handle them.
  void HandleMouseOver(bool canHandle) { mHandleMouseOver = canHandle; }
  bool OnMouseOver(int x, int y, IMouseMod* pMod);   // Returns true if mouseovers are handled.
  void OnMouseOut();
  // Some controls may not need to capture the mouse for dragging, they can call ReleaseCapture when the mouse leaves.
  void ReleaseMouseCapture();

  // Enables/disables tooltips; also enables mouseovers/mouseouts if necessary.
  inline void EnableTooltips(bool enable)
  {
    mEnableTooltips = enable;
    if (enable) mHandleMouseOver = enable;
  }
  
  // in debug builds you can enable this to draw a coloured box on the top of the GUI to show the bounds of the IControls
  inline void ShowControlBounds(bool enable)
  {
    mShowControlBounds = enable;
  }

  // Updates tooltips after (un)hiding controls.
  virtual void UpdateTooltips() = 0;

	// This is an idle call from the GUI thread, as opposed to 
	// IPlug::OnIdle which is called from the audio processing thread.
	void OnGUIIdle();

  void RetainBitmap(IBitmap* pBitmap);
  void ReleaseBitmap(IBitmap* pBitmap);
  LICE_pixel* GetBits();
  // For controls that need to interface directly with LICE.
  inline LICE_SysBitmap* GetDrawBitmap() const { return mDrawBitmap; }

  WDL_Mutex mMutex;

  struct IMutexLock
  {
    WDL_Mutex* mpMutex;
    IMutexLock(IGraphics* pGraphics) : mpMutex(&(pGraphics->mMutex)) { mpMutex->Enter(); }
    ~IMutexLock() { mpMutex->Leave(); }
  };

protected:
  WDL_PtrList<IControl> mControls;
  IPlugBase* mPlug;
  IRECT mDrawRECT;
  bool mCursorHidden;
  int mHiddenMousePointX, mHiddenMousePointY;

  bool CanHandleMouseOver() { return mHandleMouseOver; }
  inline int GetMouseOver() const { return mMouseOver; }
  inline int GetMouseX() const { return mMouseX; }
  inline int GetMouseY() const { return mMouseY; }
  inline bool TooltipsEnabled() const { return mEnableTooltips; }
  
  virtual LICE_IBitmap* OSLoadBitmap(int ID, const char* name) = 0;
  
  LICE_SysBitmap* mDrawBitmap;
  LICE_IFont* CacheFont(IText* pTxt);
  
#ifdef AAX_API
  AAX_IViewContainer* mAAXViewContainer;  
#endif

private:
  LICE_MemBitmap* mTmpBitmap;
  int mWidth, mHeight, mFPS, mIdleTicks;
  int GetMouseControlIdx(int x, int y, bool mo = false);
  int mMouseCapture, mMouseOver, mMouseX, mMouseY, mLastClickedParam;
  bool mHandleMouseOver, mStrict, mEnableTooltips, mShowControlBounds;
  IControl* mKeyCatcher;
};

#endif