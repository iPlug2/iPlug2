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
    m_depth = 0;
  }
  ~wdl_timing_accumulator()
  {
    Report("final");
  }

  static void _format_big_integer(double usec, char *buf, int buflen)
  {
    char tmp[256];
#ifdef _MSC_VER
    _snprintf(tmp, sizeof(tmp), "%.0f", usec);
#else
    snprintf(tmp, sizeof(tmp), "%.0f", usec);
#endif
    int len = strlen(tmp);
    char *from = tmp, *to = buf;
    if (len*4/3 < buflen)
    {
      while (len--)
      {
        *to++ = *from++;
        if (len && len%3 == 0) *to++ = ',';
      }
      *to = 0;
    }
    else
    {
      lstrcpyn(to, from, buflen);
    }
  }

  void Report(const char *caption=NULL) const
  {
    if (m_cnt)
    {
      const double usec=1000000.0;
      char buf1[256], buf2[256];
      _format_big_integer(m_tot*usec/(double)m_cnt, buf1, sizeof(buf1));
      _format_big_integer(m_tot*usec, buf2, sizeof(buf2));
      wdl_log_force("timing %s%s%s%s: %.0f calls, %s usec/call, %s usec total\n",
        m_name, caption ? " (" : "", caption ? caption : "", caption ? ")" : "",
        (double)m_cnt, buf1, buf2);
    }
  }

  void Begin()
  {
    if (!m_depth++)
      m_cur = time_precise();
  }
  void End()
  {
    WDL_ASSERT(m_depth > 0);
    if (!--m_depth)
    {
      WDL_ASSERT(m_cur != 0.0);
      m_tot += time_precise()-m_cur;
      m_cur=0.0;
      m_cnt++;
    }
  }

private:
  WDL_INT64 m_cnt;
  double m_tot, m_cur;
  const char *m_name;
  int m_depth;
#else

  wdl_timing_accumulator(const char *name=NULL) {}
  ~wdl_timing_accumulator() {}
  void Report(const char *caption=NULL) {}
  void Begin() {}
  void End() {}

#endif
};

class wdl_timing_accumulator_hold
{
  public:
#if defined(_DEBUG) || defined(WDL_TIMING_ENABLE_FOR_RELEASE)
    wdl_timing_accumulator_hold(wdl_timing_accumulator *p) : m_p(p) { if (p) p->Begin(); }
    ~wdl_timing_accumulator_hold() { if (m_p) m_p->End(); }
    wdl_timing_accumulator *m_p;
#else
    wdl_timing_accumulator_hold(wdl_timing_accumulator *p) {}
#endif
};

#endif
