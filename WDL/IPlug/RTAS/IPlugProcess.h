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
  IPlugProcess(OSType type);
  virtual ~IPlugProcess();

  // overrides
  virtual void SetViewPort(GrafPtr aPort);
  virtual void GetViewRect(Rect *viewRect);
  virtual void DoTokenIdle(void);
  long SetControlValue(long aControlIndex, long aValue);
  long GetControlValue(long aControlIndex, long *aValue);
  long GetControlDefaultValue(long aControlIndex, long* aValue);
  CPlugInView* CreateCPlugInView();
  ComponentResult SetControlHighliteInfo(long controlIndex, short isHighlighted, short color);
  ComponentResult ChooseControl(Point aLocalCoord, long *aControlIndex);

  virtual ComponentResult GetIndexedChunkID(long index, OSType *chunkID);
  virtual ComponentResult GetNumChunks(long *numChunks);
  virtual ComponentResult GetChunkSize(OSType chunkID, long *size);
  virtual ComponentResult SetChunk(OSType chunkID, SFicPlugInChunk *chunk);
  virtual ComponentResult GetChunk(OSType chunkID, SFicPlugInChunk *chunk);
  virtual ComponentResult CompareActiveChunk(SFicPlugInChunk *chunk, Boolean *isEqual);

  void SetEditor(void *editor) { mCustomUI = (EditorInterface*)editor; };
  virtual int ProcessTouchControl(long aControlIndex);
  virtual int ProcessReleaseControl(long aControlIndex);
  virtual void ProcessDoIdle();
  virtual void* ProcessGetModuleHandle() { return mModuleHandle; }
  virtual short ProcessUseResourceFile() { return fProcessType->GetProcessGroup()->UseResourceFile(); }
  virtual void ProcessRestoreResourceFile(short resFile) { fProcessType->GetProcessGroup()->RestoreResourceFile(resFile); }

  virtual void UpdateControlValueInAlgorithm(long aControlIndex);

  virtual IPlugRTAS* GetPlug()  { return mPlug; }
  virtual IGraphics* GetGraphics() { if (mPlug) return mPlug->GetGUI(); return 0; }
  
  virtual void DirtyState() { fNumPlugInChanges++; }

  virtual int GetBlockSize() = 0;
  virtual double GetTempo()  = 0;
  virtual void GetTimeSig(int* pNum, int* pDenom) = 0;
  virtual int GetSamplePos() = 0;
  virtual void GetTime( double *pSamplePos,
                        double *pTempo,
                        double *pMusicalPos,
                        double *pLastBar,
                        int* pNum,
                        int* pDenom,
                        double *pCycleStart,
                        double *pCycleEnd,
                        bool *pTransportRunning,
                        bool *pTransportCycle) = 0;

  void ResizeGraphics(int w, int h);
  virtual int GetHostVersion();

protected:
  virtual void EffectInit();
  virtual void ConnectSidechain();
  virtual void DisconnectSidechain();

protected:
  UInt32            mLastMeterTicks;
  GrafPtr           mMainPort;
  EditorInterface   *mCustomUI;
  Rect              mPluginWinRect;
  short             mLeftOffset;
  short             mTopOffset;
  IPlugDigiView     *mView;
  IPlugRTAS         *mPlug;
  void              *mModuleHandle;
  DirectMidiPlugInInterface* mDirectMidiInterface;
  OSType mPluginID;
};

#endif  // __IPLUGPROCESS__
