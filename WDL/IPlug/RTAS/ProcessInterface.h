#ifndef _PROCESS_INTERFACE_H_
#define _PROCESS_INTERFACE_H_

class ProcessInterface
{
public:
  ProcessInterface(void) {};
  virtual ~ProcessInterface(void) {};

  virtual long SetControlValue(long aControlIndex, long aValue) = 0;
  virtual long GetControlValue(long aControlIndex, long *aValue) = 0;
  virtual long GetControlDefaultValue(long aControlIndex, long* aValue) = 0;

  virtual void SetEditor(void *editor) = 0;

  virtual int ProcessTouchControl (long aControlIndex) = 0;
  virtual int ProcessReleaseControl (long aControlIndex) = 0;
  virtual void ProcessDoIdle() = 0;
  virtual void* ProcessGetModuleHandle() = 0;
  virtual short ProcessUseResourceFile() = 0;
  virtual void ProcessRestoreResourceFile(short resFile) = 0;
};

#endif //_PROCESS_INTERFACE_H_


