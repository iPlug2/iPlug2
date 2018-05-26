#pragma once

#include "IPlugVST2.h"
#include "reaper_plugin.h"
#include "video_processor.h"

#define REAPERAPI_IMPLEMENT
#include "reaper_plugin_functions.h"

class IPlugReaper : public IPlugVST2
{
public:
  IPlugReaper(IPlugInstanceInfo instanceInfo, IPlugConfig config)
  : IPlugVST2(instanceInfo, config)
  {
    int errorCount = REAPERAPI_LoadAPI([this](const char* str) {
                                         return (void*) mHostCallback(NULL, 0xdeadbeef, 0xdeadf00d, 0, (void*) str, 0.0);
                                       });
    if (errorCount > 0)
      LogToReaper("some errors when loading reaper api functions\n");
  }

  void SetTrackVolume(double gain)
  {
    MediaTrack* tr = GetReaperTrack();

    if (tr != nullptr)
      SetMediaTrackInfo_Value(tr, "D_VOL", gain);
  }
  
  void GetTakeName(WDL_String& str)
  {
    const char* name = ::GetTakeName(GetReaperTake());
    if (name != nullptr)
      str.Set(name);
  }

  void SetTakeName(const char* name)
  {
    MediaItem_Take* tk = GetReaperTake();
    if (tk != nullptr)
    {
      GetSetMediaItemTakeInfo_String(tk, "P_NAME", const_cast<char*>(name), true);
      UpdateArrange();
    }
    else
    {
      LogToReaper("Plugin is not loaded into a Reaper take\n");
    }
  }

  void GetTrackName(WDL_String& str)
  {
    char buf[2048];
    if (GetSetMediaTrackInfo_String(GetReaperTrack(), "P_NAME", buf, false) == true)
    {
      str.Set(buf);
    }
  }

  void SetTrackName(const char* name)
  {
    GetSetMediaTrackInfo_String(GetReaperTrack(), "P_NAME", const_cast<char*>(name), true);
    UpdateArrange();
  }

  MediaTrack* GetReaperTrack()
  {
    return (MediaTrack*)mHostCallback(&mAEffect, 0xdeadbeef, 0xdeadf00e, 1, 0, 0.0);
  }

  MediaItem_Take* GetReaperTake()
  {
    return (MediaItem_Take*)mHostCallback(&mAEffect, 0xdeadbeef, 0xdeadf00e, 2, 0, 0.0);
  }
  
  void LogToReaper(const char* str)
  {
    if(ShowConsoleMsg != nullptr)
      ShowConsoleMsg(str);
  }
  
  void InformHostOfAddedParams(int index, int numAddedParams)
  {
    int listadj[2] = {index, numAddedParams};
    mHostCallback(&mAEffect, audioMasterVendorSpecific, 0xdeadbeef, audioMasterAutomate, listadj, 0.0);
  }
  
  void InformHostOfRemovedParams(int index, int numRemovedParams)
  {
    int listadj[2] = {index, -numRemovedParams};
    mHostCallback(&mAEffect, audioMasterVendorSpecific, 0xdeadbeef, audioMasterAutomate, listadj, 0.0);
  }
  
  void InitializeVideo(void* staticProcessVideoFrame, void* staticGetVideoParam)
  {
    void* pCtx = (void*) mHostCallback(&mAEffect, 0xdeadbeef, 0xdeadf00e, 4, NULL, 0.0f);
    
    if (pCtx)
    {
      IREAPERVideoProcessor *(*video_CreateVideoProcessor)(void *fxctx, int version);
      *(void**)&video_CreateVideoProcessor = (void *) mHostCallback(&mAEffect, 0xdeadbeef, 0xdeadf00d, 0, (void *) "video_CreateVideoProcessor", 0.0f);
      if (video_CreateVideoProcessor)
      {
        mVideoProc = video_CreateVideoProcessor(pCtx, IREAPERVideoProcessor::REAPER_VIDEO_PROCESSOR_VERSION);
        if (mVideoProc)
        {
          mVideoProc->userdata = this;
          mVideoProc->process_frame = (IVideoFrame* (*)(IREAPERVideoProcessor*, const double*, int, double, double, int)) staticProcessVideoFrame;
          mVideoProc->get_parameter_value = (bool (*)(IREAPERVideoProcessor*, int, double *)) staticGetVideoParam;
        }
      }
    }
  }
  
private:
//  template<typename T>
//  T (*GetFunc(VstInt32 p1, VstIntPtr p2, const char* str = NULL, float p3 = 0.f))()
//  {
//    T (*func)();
//    *(long *)&func = mHostCallback(&mAEffect, 0xdeadbeef, p1, p2, (void*) str, p3);
//    return func;
//  }
  
  IREAPERVideoProcessor* mVideoProc = nullptr;
};
