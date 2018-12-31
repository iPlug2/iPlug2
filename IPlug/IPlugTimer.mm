
#import <Foundation/Foundation.h>
#include "mutex.h"
#include "IPlugTimer.h"
#include <sys/time.h>
#include <unistd.h>

#define WM_TIMER 0x0113

void Sleep(int ms)
{
  usleep(ms?ms*1000:100);
}

DWORD GetTickCount()
{
  struct timeval tm={0,};
  gettimeofday(&tm,NULL);
  return (DWORD) (tm.tv_sec*1000 + tm.tv_usec/1000);
}

@interface SWELL_DataHold : NSObject
{
  void *m_data;
}
-(id) initWithVal:(void *)val;
-(void *) getValue;
@end

@interface SWELL_TimerFuncTarget : NSObject
{
  TIMERPROC m_cb;
  HWND m_hwnd;
  UINT_PTR m_timerid;
}
-(id) initWithId:(UINT_PTR)tid hwnd:(HWND)h callback:(TIMERPROC)cb;
-(void)SWELL_Timer:(id)sender;
@end

@implementation SWELL_TimerFuncTarget

-(id) initWithId:(UINT_PTR)tid hwnd:(HWND)h callback:(TIMERPROC)cb
{
  if ((self = [super init]))
  {
    m_hwnd=h;
    m_cb=cb;
    m_timerid = tid;
  }
  return self;
}
-(void)SWELL_Timer:(id)sender
{
//  if (g_swell_only_timerhwnd && m_hwnd != g_swell_only_timerhwnd) return;
  
  m_cb(m_hwnd,WM_TIMER,m_timerid,GetTickCount());
}
@end

@implementation SWELL_DataHold
-(id) initWithVal:(void *)val
{
  if ((self = [super init]))
  {
    m_data=val;
  }
  return self;
}
-(void *) getValue
{
  return m_data;
}
@end

// timer stuff
typedef struct TimerInfoRec
{
  UINT_PTR timerid;
  HWND hwnd;
  NSTimer *timer;
  struct TimerInfoRec *_next;
} TimerInfoRec;
static TimerInfoRec *m_timer_list;
static WDL_Mutex m_timermutex;

UINT_PTR SetTimer(HWND hwnd, UINT_PTR timerid, UINT rate, TIMERPROC tProc)
{
  if (!hwnd && !tProc) return 0; // must have either callback or hwnd
  
  if (hwnd && !timerid) return 0;

  WDL_MutexLock lock(&m_timermutex);
  TimerInfoRec *rec=NULL;
  if (hwnd||timerid)
  {
    rec = m_timer_list;
    while (rec)
    {
      if (rec->timerid == timerid && rec->hwnd == hwnd) // works for both kinds
        break;
      rec=rec->_next;
    }
  }
  
  bool recAdd=false;
  if (!rec) 
  {
    rec=(TimerInfoRec*)malloc(sizeof(TimerInfoRec));
    recAdd=true;
  }
  else 
  {
    [rec->timer invalidate];
    rec->timer=0;
  }
  
  rec->timerid=timerid;
  rec->hwnd=hwnd;
  
  if (!hwnd || tProc)
  {
    // set timer to this unique ptr
    if (!hwnd) timerid = rec->timerid = (UINT_PTR)rec;
    
    SWELL_TimerFuncTarget *t = [[SWELL_TimerFuncTarget alloc] initWithId:timerid hwnd:hwnd callback:tProc];
    rec->timer = [NSTimer scheduledTimerWithTimeInterval:(wdl_max(rate,1)*0.001) target:t selector:@selector(SWELL_Timer:)
                                                userInfo:t repeats:YES];
    [t release];
    
  }
  else
  {
    SWELL_DataHold *t=[[SWELL_DataHold alloc] initWithVal:(void *)timerid];
    rec->timer = [NSTimer scheduledTimerWithTimeInterval:(wdl_max(rate,1)*0.001) target:(id)hwnd selector:@selector(SWELL_Timer:)
                                                userInfo:t repeats:YES];
    
    [t release];
  }
  [[NSRunLoop currentRunLoop] addTimer:rec->timer forMode:(NSString*)kCFRunLoopCommonModes];
  
  if (recAdd)
  {
    rec->_next=m_timer_list;
    m_timer_list=rec;
  }
  
  return timerid;
}

BOOL KillTimer(HWND hwnd, UINT_PTR timerid)
{
  if (!hwnd && !timerid) return FALSE;
  
  WDL_MutexLock lock(&m_timermutex);

  BOOL rv=FALSE;
  
  // don't allow removing all global timers
  if (timerid!=~(UINT_PTR)0 || hwnd)
  {
    TimerInfoRec *rec = m_timer_list, *lrec=NULL;
    while (rec)
    {
      
      if (rec->hwnd == hwnd && (timerid==~(UINT_PTR)0 || rec->timerid == timerid))
      {
        TimerInfoRec *nrec = rec->_next;
        
        // remove self from list
        if (lrec) lrec->_next = nrec;
        else m_timer_list = nrec;
        
        [rec->timer invalidate];
        free(rec);
        
        rv=TRUE;
        if (timerid!=~(UINT_PTR)0) break;
        
        rec=nrec;
      }
      else
      {
        lrec=rec;
        rec=rec->_next;
      }
    }
  }
  return rv;
}
