/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugVST3_RunLoop.h"

BEGIN_IPLUG_NAMESPACE

struct VST3Timer : Steinberg::Linux::ITimerHandler, public Steinberg::FObject
{
  std::function<void()> callback;

  void PLUGIN_API onTimer() override
  {
    callback();
  }

  DELEGATE_REFCOUNT (Steinberg::FObject)
  DEFINE_INTERFACES
    DEF_INTERFACE (Steinberg::Linux::ITimerHandler)
  END_DEFINE_INTERFACES (Steinberg::FObject)
};

struct EventHandler : Steinberg::Linux::IEventHandler, public Steinberg::FObject
{
  IPlugVST3_RunLoop* ev;

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
  IPlugVST3_RunLoop* ev;

  void PLUGIN_API onTimer () override
  {
    ev->runLoop->unregisterTimer(this);
    ev->tHandlerSet = false;
    xcbt_process(ev->x);
  }

  DELEGATE_REFCOUNT (Steinberg::FObject)
  DEFINE_INTERFACES
    DEF_INTERFACE (Steinberg::Linux::ITimerHandler)
  END_DEFINE_INTERFACES (Steinberg::FObject)
};

IPlugVST3_RunLoop* IPlugVST3_RunLoop::Create(Steinberg::FUnknown *frame)
{
  auto ev = new IPlugVST3_RunLoop();
  ev->dtor = Self::xt_dtor;
  ev->set_x = Self::xt_set_x;
  ev->set_timer = Self::xt_set_timer;
  ev->watch = Self::xt_watch;

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
  
  return ev;
}

void IPlugVST3_RunLoop::Destory(IPlugVST3_RunLoop* self)
{
  Self::xt_dtor((xcbt_embed*) self);
}

VST3Timer* IPlugVST3_RunLoop::CreateTimer(std::function<void()> callback, int msec)
{
  auto tm = new VST3Timer();
  tm->callback = callback;
  if (runLoop->registerTimer(tm, msec) == Steinberg::kResultOk)
  {
    mTimers.Add(tm);
    return tm;
  }
  else
  {
    delete tm;
  }
}

void IPlugVST3_RunLoop::DestroyTimer(VST3Timer* timer)
{
  runLoop->unregisterTimer(timer);
  mTimers.Delete(mTimers.FindR(timer));
  delete timer;
}

void IPlugVST3_RunLoop::xt_dtor(xcbt_embed* pe)
{
  Self* ev = (Self*) pe;

  if (ev)
  {
    Self::xt_watch(pe, -1);
    Self::xt_set_timer(pe, -1);
    // printf("Releasing eHandeler %u\n", ev->eHandler->getRefCount()); // was checking refCounter is 1...
    // printf("Releasing tHandler %u\n", ev->tHandler->getRefCount());
    ev->eHandler->release();
    ev->tHandler->release();

    int i = 0;
    while ((i = ev->mTimers.GetSize()) > 0)
    {
      ev->DestroyTimer(ev->mTimers.Get(i - 1));
    }

    delete ev;
  }
}

int IPlugVST3_RunLoop::xt_set_x(xcbt_embed* pe, xcbt x)
{
  Self* ev = (Self*) pe;

  if(ev && !ev->x)
  {
    ev->x = x;
    return 1;
  }
  return 0;
}

int IPlugVST3_RunLoop::xt_set_timer(xcbt_embed* pe, int msec)
{
  Self* ev = (Self*) pe;

  if(!ev)
    return 0;

  if(ev->tHandlerSet)
  {
    ev->runLoop->unregisterTimer(ev->tHandler);
    ev->tHandlerSet = false;
  }
  
  if(msec > 0)
  {
    if(ev->runLoop->registerTimer(ev->tHandler, msec) == Steinberg::kResultOk)
    {
      ev->tHandlerSet = true;
    }
    return 1;
  }
  else
  {
    return 1;
  }

  return ev->tHandlerSet == true;
}

int IPlugVST3_RunLoop::xt_watch(xcbt_embed* pe, int fd)
{
  Self* ev = (Self*) pe;
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

END_IPLUG_NAMESPACE
