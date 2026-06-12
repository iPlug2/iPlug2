#ifndef _WDL_TIME_SERIES_H_
#define _WDL_TIME_SERIES_H_

#include "heapbuf.h"

class WDL_TimeSeries
{
  WDL_TimeSeries(int size)
  {
    m_size=wdl_max(size,1);
    m_ema_wt=0.0;
    Reset();
  }
  void Reset()
  {
    m_series.Resize(0, false);
    m_pos=0;
    m_ema=0.0;
    m_ema_init=false;
  }

  int GetSize() const { return m_series.GetSize(); }

  void AddData(double v)
  {
    if (m_series.GetSize() < m_size) m_series.Add(v);
    else m_series.Get()[m_pos]=v;
    m_pos = (m_pos+1)%m_size;
    if (m_ema_wt > 0.0)
    {
      if (!m_ema_init)
      {
        m_ema=v;
        m_ema_init=true;
      }
      else
      {
        m_ema=m_ema*(1.0-m_ema_wt)+v*m_ema_wt;
      }
    }
  }

  double Last() const
  {
    const int sz=m_series.GetSize();
    if (!sz) return 0.0;
    const double *p=m_series.Get();
    return (m_pos ? p[m_pos-1] : p[sz-1]);
  }
  double Mean() const
  {
    const int sz=m_series.GetSize();
    if (!sz) return 0.0;
    double mean=0.0;
    const double *p=m_series.Get();
    for (int i=0; i < sz; ++i) mean += p[i];
    return mean/(double)sz;
  }

  double StdDev() const
  {
    const int sz=m_series.GetSize();
    if (!sz) return 0.0;
    const double mean=Mean();
    double mean2=0.0;
    const double *p=m_series.Get();
    for (int i=0; i < sz; ++i) mean2 += (p[i]-mean)*(p[i]-mean);
    return sqrt(mean2/(double)sz);
  }

  double ZScore(double v) const
  {
    const int sz=m_series.GetSize();
    if (!sz) return 0.0;
    double stdev=StdDev();
    if (stdev == 0.0) return 0.0;
    double last=Last();
    return fabs(v-last)/stdev;
  }

  void InitExpMovAvg(int hl)
  {
    if (hl < 1) hl=1;
    m_ema_wt=exp(-1.0/(double)hl);
  }
  double ExpMovAvg() const { return m_ema; }

  int m_size;
  int m_pos;
  double m_ema, m_ema_wt;
  bool m_ema_init;
  WDL_TypedBuf<double> m_series;
};


#endif
