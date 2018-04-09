#include "IPlugTimer.h"

WDL_PtrList<Timer_impl> Timer_impl::sTimers;

//static
Timer* Timer::Create(ITimerCallback& callback, uint32_t intervalMs)
{
  return new Timer_impl(callback, intervalMs);
}
