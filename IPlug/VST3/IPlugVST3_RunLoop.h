/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/
#ifndef __IPLUGVST3_RUNLOOP
#define __IPLUGVST3_RUNLOOP

#include <memory>

#include "IPlugPlatform.h"

#include "IPlugTimer.h"

#include <ptrlist.h>
#include <functional>
#include "base/source/fobject.h"
#include "pluginterfaces/gui/iplugview.h"

#include "xcbt.h"

BEGIN_IPLUG_NAMESPACE

struct VST3Timer;
struct EventHandler;
struct TimerHandler;

// This struct MUST be pointer-compatible with xcbt_embed.
struct IPlugVST3_RunLoop final : xcbt_embed
{
public:
  using Self = IPlugVST3_RunLoop;

private:
  Steinberg::Linux::IRunLoop *runLoop;
  xcbt x;
  struct EventHandler* eHandler;
  bool eHandlerSet;
  struct TimerHandler* tHandler;
  bool tHandlerSet;

  WDL_PtrList<VST3Timer> mTimers;

  static void xt_dtor(xcbt_embed* self);
  static int  xt_set_x(xcbt_embed* self, xcbt x);
  static int  xt_set_timer(xcbt_embed* self, int msec);
  static int  xt_watch(xcbt_embed* self, int fd);
  
  friend class EventHandler;
  friend class TimerHandler;

public:
  /** Create XCBT embed and IPlugVST3_RunLoop using VST3 IRunLoop interface
   * @param frame an object with IRunLoop interface
   * @return allocated embed structure in case the initialization could be done on nullptr*/
  static IPlugVST3_RunLoop* Create(Steinberg::FUnknown *frame);
  static void Destory(IPlugVST3_RunLoop* self);

  VST3Timer* CreateTimer(std::function<void()> callback, int msec);
  void DestroyTimer(VST3Timer* timer);
};

END_IPLUG_NAMESPACE

#endif
