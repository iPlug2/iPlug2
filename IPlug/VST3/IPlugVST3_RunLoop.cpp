/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/
#include "IPlugVST3_RunLoop.h"

BEGIN_IPLUG_NAMESPACE

struct EventHandler;
struct TimerHandler;

extern "C" {

typedef struct {
  xcbt_embed embed;
  
  Steinberg::Linux::IRunLoop *runLoop;
  xcbt x;

  // we support exactly one watch and one timer 
  struct EventHandler *eHandler;
  bool   eHandlerSet;
  struct TimerHandler *tHandler;
  bool   tHandlerSet;
  
} _xcbt_embed_vst3;

};

struct EventHandler : Steinberg::Linux::IEventHandler, public Steinberg::FObject
{
  _xcbt_embed_vst3 *ev;

  void PLUGIN_API onFDIsSet (Steinberg::Linux::FileDescriptor) override 
  { 
    xcbt_process(ev->x); 
  }

  DELEGATE_REFCOUNT (Steinberg::FObject)
  DEFINE_INTERFACES
    DEF_INTERFACE (Steinberg::Linux::IEventHandler)
  END_DEFINE_INTERFACES (Steinberg::FObject)
};

struct TimerHandler : Steinberg::Linux::ITimerHandler, public Steinberg::FObject
{
  _xcbt_embed_vst3 *ev;

  void PLUGIN_API onTimer () override
  {
    // printf("Timer\n");
    ev->runLoop->unregisterTimer(this);
    ev->tHandlerSet = false;
    xcbt_process(ev->x);
  }
  DELEGATE_REFCOUNT (Steinberg::FObject)
  DEFINE_INTERFACES
    DEF_INTERFACE (Steinberg::Linux::ITimerHandler)
  END_DEFINE_INTERFACES (Steinberg::FObject)
};

static int xcbt_embed_vst3_set_x(xcbt_embed *pe, xcbt x){
  _xcbt_embed_vst3 *ev = (_xcbt_embed_vst3 *)pe;
  if(ev && !ev->x)
  {
    ev->x = x;
    return 1;
  }
  return 0;
}

static int xcbt_embed_vst3_set_timer(xcbt_embed *pe, int msec){
  _xcbt_embed_vst3 *ev = (_xcbt_embed_vst3 *)pe;
  if(!ev)
    return 0;
  if(ev->tHandlerSet){
    ev->runLoop->unregisterTimer(ev->tHandler);
    ev->tHandlerSet = false;
  }
  if(msec > 0){
    if(ev->runLoop->registerTimer(ev->tHandler, msec) == Steinberg::kResultOk)
    {
      ev->tHandlerSet = true;
    }
    return 1;
  } else {
    return 1;
  }
  return ev->tHandlerSet == true;
}

static int xcbt_embed_vst3_watch(xcbt_embed *pe, int fd){
  _xcbt_embed_vst3 *ev = (_xcbt_embed_vst3 *)pe;
  int i;
  if(!ev)
    return 0;
  if(fd < 0)
  {
    if(ev->eHandlerSet)
    {
      ev->runLoop->unregisterEventHandler(ev->eHandler);
      ev->eHandlerSet = false;
    }
    return 1;
  }
  if(ev->eHandlerSet)
  {
    return 0; // some fd is already set, bug since we do not support several at the moment
  }
  if(ev->runLoop->registerEventHandler(ev->eHandler, fd) == Steinberg::kResultOk)
  {
    ev->eHandlerSet = true;
  }
  return ev->eHandlerSet == true;
}

static void xcbt_embed_vst3_dtor(xcbt_embed *pe){
  _xcbt_embed_vst3 *ev = (_xcbt_embed_vst3 *)pe;
  if (ev)
  {
    xcbt_embed_vst3_watch(pe, -1);
    xcbt_embed_vst3_set_timer(pe, -1);
    // printf("Releasing eHandeler %u\n", ev->eHandler->getRefCount()); // was checking refCounter is 1...
    // printf("Releasing tHandler %u\n", ev->tHandler->getRefCount());
    ev->eHandler->release();
    ev->tHandler->release();
    delete ev;
  }
}

xcbt_embed *IPlugVST3_EmbedFactory(Steinberg::FUnknown *frame)
{
  _xcbt_embed_vst3 *ev = new _xcbt_embed_vst3();
  ev->embed.dtor = xcbt_embed_vst3_dtor;
  ev->embed.set_x = xcbt_embed_vst3_set_x;
  ev->embed.set_timer = xcbt_embed_vst3_set_timer;
  ev->embed.watch = xcbt_embed_vst3_watch;

  Steinberg::FUnknownPtr<Steinberg::Linux::IRunLoop> runLoop(frame);
  ev->runLoop = runLoop;
  if(!ev->runLoop)
  {
    delete ev;
    return nullptr;
  }
  ev->x = nullptr;
  ev->eHandlerSet = false;
  ev->tHandlerSet = false;
  ev->eHandler = new EventHandler();
  ev->eHandler->ev = ev;
  ev->tHandler = new TimerHandler();
  ev->tHandler->ev = ev;
  
  return &ev->embed;
}


END_IPLUG_NAMESPACE
