#ifndef _WDL_TIMING2_H_
#define _WDL_TIMING2_H_

#if defined(_DEBUG) || defined(WDL_TIMING_ENABLE_FOR_RELEASE)
#include "time_precise.h"
#endif

class wdl_timing_accumulator
{
public:

#if defined(_DEBUG) || defined(WDL_TIMING_ENABLE_FOR_RELEASE)

  wdl_timing_accumulator(const char *name=NULL)
  {
    m_cnt=0;
    m_tot=m_cur=0.0;
    m_name = name ? name : "[unnamed]";
  }
  ~wdl_timing_accumulator()
  {
    if (m_cnt)
    {
      const double usec=1000000.0;
      wdl_log_force("timing %s: %.0f calls, %.0f usec/call, %.0f usec total\n",
        m_name, (double)m_cnt, m_tot*usec/(double)m_cnt, m_tot*usec);
    }
  }

  void Begin()
  {
    m_cur = time_precise();
  }
  void End()
  {
    WDL_ASSERT(m_cur != 0.0);
    m_tot += time_precise()-m_cur;
    m_cur=0.0;
    m_cnt++;
  }

private:
  WDL_INT64 m_cnt;
  double m_tot, m_cur;
  const char *m_name;

#else

  wdl_timing_accumulator(const char *name=NULL) {}
  ~wdl_timing_accumulator() {}
  void Begin() {}
  void End() {}

#endif
};

#endif
