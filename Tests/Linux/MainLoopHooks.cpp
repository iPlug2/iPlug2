#include "IPlugSWELL.h"
#include <alsa/asoundlib.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Wrap only the test executables' SWELL timer calls; production builds have no hooks.
extern UINT_PTR RealSetTimer(HWND, UINT_PTR, UINT, TIMERPROC)
  asm("__real__Z8SetTimerP6HWND__mjPFvS0_jmjE");
extern UINT_PTR TestSetTimer(HWND, UINT_PTR, UINT, TIMERPROC)
  asm("__wrap__Z8SetTimerP6HWND__mjPFvS0_jmjE");

UINT_PTR TestSetTimer(HWND window, UINT_PTR id, UINT rate, TIMERPROC callback)
{
  const char* mode = getenv("IPLUG_LINUX_TEST_MODE");
  if (!window && callback && rate == 16)
  {
    snd_lib_error(__FILE__, __LINE__, __func__, 0, "Linux ALSA diagnostic regression");
    if (mode && strcmp(mode, "timer-fail") == 0)
    {
      fprintf(stderr, "Injected graphics main-loop timer creation failure\n");
      return 0;
    }
  }
  return RealSetTimer(window, id, rate, callback);
}
