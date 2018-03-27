#pragma once

#include "IPlugVST2.h"
#include "reaper_plugin.h"

#define IMPAPI(x) if (!((*((void **)&(x)) = (void *)rec->GetFunc(#x)))) errcnt++;

#define REAPERAPI_IMPLEMENT
#include "reaper_plugin_functions.h"

//REAPER_PLUGIN_HINSTANCE ghInst;

class IPlugReaper : public IPlugVST2
{
public:
  IPlugReaper(IPlugInstanceInfo instanceInfo, IPlugConfig config)
  : IPlugVST2(instanceInfo, config)
  {
    int errorCount = REAPERAPI_LoadAPI([this](const char* str)
                                       {
                                         return (void*)mHostCallback(NULL, 0xdeadbeef, 0xdeadf00d, 0, (void*)str, 0.0);
                                       });
    if (errorCount > 0)
      LogToReaper("some errors when loading reaper api functions\n");
    
    LogToReaper("hello");
  }
  
//  double GetPlayPosition()
//  {
//  }
//
//  double GetPlayPosition2()
//  {
//  }
//
//  double GetCursorPosition()
//  {
//  }
//
//  int GetPlayState()
//  {
//  }
//
//  int GetSetRepeat(int parm)
//  {
//  }
//
//  void GetProjectPath(WDL_String& path)
//  {
//  }
//
//  bool IsInRealTimeAudio()
//  {
//  }
//
//  bool Audio_IsRunning()
//  {
//  }
//
//  void SetEditCurPos(double time, bool moveview, bool seekplay)
//  {
//  }
  
  void executeCallbackFunc(const char* str)
  {
    void (*func)();
    *(long *)&func = mHostCallback(NULL, 0xdeadbeef, 0xdeadf00d, 0, (void*) str, 0.);
    func();
  }
  
  void OnPlayButton() { executeCallbackFunc("OnPlayButton"); }
  void OnPauseButton() { executeCallbackFunc("OnPauseButton"); }
  void OnStopButton() { executeCallbackFunc("OnStopButton"); }
  
//  void setTrackVolume(double gain);
//  String getTakeName();
//  void setTakeName(String name);
//  String getTrackName();
//  void setTrackName(String name);
//  MediaTrack* getReaperTrack();
//  MediaItem_Take* getReaperTake();
//  void extendedStateHasChanged();
  
  void LogToReaper(const char* str)
  {
    if(ShowConsoleMsg != nullptr)
      ShowConsoleMsg(str);
  }
};

