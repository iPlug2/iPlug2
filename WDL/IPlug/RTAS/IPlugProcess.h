#ifndef __IPLUGPROCESS__
#define __IPLUGPROCESS__

#include "CEffectProcess.h"
#include "ProcessInterface.h"
#include "EditorInterface.h"
#include "IPlugDigiView.h"
#include "CProcessType.h"
#include "CProcessGroup.h"
#include "DirectMidi.h"

#include "../IPlugRTAS.h"
#include "Resource.h"

class IPlugProcess :  virtual public CEffectProcess, ProcessInterface
{
public:
  IPlugProcess(void);
  virtual ~IPlugProcess(void);
  
  // overrides
  virtual void SetViewPort(GrafPtr aPort);
  virtual void GetViewRect(Rect *viewRect);
  virtual void DoTokenIdle(void);
  long SetControlValue (long aControlIndex, long aValue);
  long GetControlValue(long aControlIndex, long *aValue);
  long GetControlDefaultValue(long aControlIndex, long* aValue);
  //ComponentResult UpdateControlGraphic (long aControlIndex, long aValue);
  CPlugInView* CreateCPlugInView();
  ComponentResult SetControlHighliteInfo (long controlIndex, short isHighlighted, short color);
  ComponentResult ChooseControl (Point aLocalCoord, long *aControlIndex);
  
  void setEditor(void *editor) { mCustomUI = (EditorInterface*)editor; };
  virtual int ProcessTouchControl (long aControlIndex);
  virtual int ProcessReleaseControl (long aControlIndex);
  virtual void ProcessDoIdle();
  virtual void* ProcessGetModuleHandle() { return mModuleHandle; }
  virtual short ProcessUseResourceFile() { return fProcessType->GetProcessGroup()->UseResourceFile(); }
  virtual void ProcessRestoreResourceFile(short resFile) { fProcessType->GetProcessGroup()->RestoreResourceFile(resFile); }
  
  virtual void UpdateControlValueInAlgorithm(long aControlIndex);
  //virtual	Boolean HandleKeystroke(EventRecord *theEvent);
  
  virtual IPlugRTAS* getPlug()  { return mPlug; }
  
  virtual IGraphics* getGraphics() 
  {
    if (mPlug) 
      return mPlug->GetGUI(); 
    
    return 0;
  }
  
protected:
  virtual void EffectInit();
  virtual void ConnectSidechain(void);
  virtual void DisconnectSidechain(void);
  
protected:
  UInt32            mLastMeterTicks;
  GrafPtr           mMainPort;    // Mac-based GrafPtr
  EditorInterface   *mCustomUI;   // pointer to UI interface
  Rect              mPluginWinRect;
  IPlugDigiView     *mNoUIView;
  IPlugRTAS         *mPlug;
  void              *mModuleHandle;
  DirectMidiPlugInInterface* mDirectMidiInterface;
};

#endif  // __IPLUGPROCESS__
