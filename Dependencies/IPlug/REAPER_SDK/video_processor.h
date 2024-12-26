#ifndef _REAPER_VIDEO_PROCESSOR_H_
#define _REAPER_VIDEO_PROCESSOR_H_

#include "video_frame.h"

class IREAPERVideoProcessor
{
public:
  IREAPERVideoProcessor() { process_frame=NULL; get_parameter_value = NULL; userdata = NULL; }
  virtual ~IREAPERVideoProcessor() { }

  enum { REAPER_VIDEO_PROCESSOR_VERSION=0x100 }; 

  // give callbacks to query incoming video frames
  // + ownedSourceInst API + render

  virtual IVideoFrame *newVideoFrame(int w, int h, int fmt)=0; // always use this for IVideoFrames returned by process_frame, never create your own. return it from process_frame(), or Release() it.

  virtual int getNumInputs()=0;
  virtual int getInputInfo(int idx, void **itemptr)=0; // input can be -1 for current context. sets itemptr if non-NULL. trackidx can be 100000000 for preview, or other values
  virtual IVideoFrame *renderInputVideoFrame(int idx, int want_fmt/*0 for native*/)=0; // must treat returned frame as immutable! either return it or Release() it.

  // user-specified
  IVideoFrame *(*process_frame)(IREAPERVideoProcessor *vproc, 
                                const double *parmlist, int nparms,  // first parameter is wet/dry, then subsequent map to that of the plug-in  (up to VIDEO_EFFECT_MAX_PARAMETERS)
                                double project_time, double frate, 
                                int force_format // 0, or 'YV12', 'RGBA', 'YUY2'
                              ); // return video frame

  bool (*get_parameter_value)(IREAPERVideoProcessor *vproc, int idx, double *v); // queries the value of a parameter from the video thread. return false if parameter out of bounds

  void *userdata;

};

/*
 To use IREAPERVideoProcessor in a VST:


    void *ctx=(void*)hostcb(&m_effect,0xdeadbeef,0xdeadf00e,4,NULL,0.0f);
    if (ctx)
    {
      IREAPERVideoProcessor *(*video_CreateVideoProcessor)(void *fxctx, int version);
      *(void **)&video_CreateVideoProcessor = (void *)hostcb(&m_effect,0xdeadbeef,0xdeadf00d,0,"video_CreateVideoProcessor",0.0f);
      if (video_CreateVideoProcessor)
      {
        m_videoproc = video_CreateVideoProcessor(ctx,IREAPERVideoProcessor::REAPER_VIDEO_PROCESSOR_VERSION);
        if (m_videoproc)
        {
          m_videoproc->userdata = _this;
          m_videoproc->process_frame = staticProcessFrame;
          m_videoproc->get_parameter_value = staticGetVideoParam;
        }
      }
    }


  static IVideoFrame *staticProcessVideoFrame(IREAPERVideoProcessor *vproc, const double *parmlist, int nparms, double project_time, double frate,  int force_format)
  {
    IVideoFrame *vf = vproc->newVideoFrame(320,200,'RGBA');
    if (vf)
    {
    // bla blah. parmlist[0] is wet, [1] is parameter, values at video time (which may differ depending on audio). do not update internal state, but use those parameters for rendering
    }
    return vf;
  }

    static bool staticGetVideoParam(IREAPERVideoProcessor *vproc, int idx, double *valueOut)
    {
      // called from video thread, gets current state
      if (idx>=0 && idx < NUM_PARAMS)
      {
        *valueOut = m_parms[idx];
         return true;
      }
      return false;
    }

  */


#endif
