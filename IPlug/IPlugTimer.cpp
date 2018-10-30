/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */


#include "IPlugTimer.h"

WDL_PtrList<Timer_impl> Timer_impl::sTimers;

//static
Timer* Timer::Create(ITimerFunction func, uint32_t intervalMs)
{
  return new Timer_impl(func, intervalMs);
}
