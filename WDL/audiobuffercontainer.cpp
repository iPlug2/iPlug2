#include "audiobuffercontainer.h"
#include "queue.h"
#include <assert.h>

void ChannelPinMapper::SetNPins(int nPins)
{
  if (nPins<0) nPins=0;
  else if (nPins>CHANNELPINMAPPER_MAXPINS) nPins=CHANNELPINMAPPER_MAXPINS;
  int i;
  for (i = m_nPins; i < nPins; ++i) {
    ClearPin(i);
    if (i < m_nCh) {
      SetPin(i, i, true);
    }
  }
  m_nPins = nPins;
}

void ChannelPinMapper::SetNChannels(int nCh, bool auto_passthru)
{
  if (auto_passthru) for (int i = m_nCh; i < nCh && i < m_nPins; ++i) {
    SetPin(i, i, true);
  }
  m_nCh = nCh;
}

void ChannelPinMapper::Init(const PinMapPin * pMapping, int nPins)
{
  if (nPins<0) nPins=0;
  else if (nPins>CHANNELPINMAPPER_MAXPINS) nPins=CHANNELPINMAPPER_MAXPINS;
  memcpy(m_mapping, pMapping, nPins*sizeof(PinMapPin));
  memset(m_mapping+nPins, 0, (CHANNELPINMAPPER_MAXPINS-nPins)*sizeof(PinMapPin));
  m_nPins = m_nCh = nPins;
}

#define BITMASK64(bitIdx) (((WDL_UINT64)1)<<(bitIdx))
 
void ChannelPinMapper::ClearPin(int pinIdx)
{
  if (pinIdx >=0 && pinIdx < CHANNELPINMAPPER_MAXPINS) m_mapping[pinIdx].clear();
}

void ChannelPinMapper::SetPin(int pinIdx, int chIdx, bool on)
{
  if (pinIdx >=0 && pinIdx < CHANNELPINMAPPER_MAXPINS)
  {
    if (on) 
    {
      m_mapping[pinIdx].set_chan(chIdx);
    }
    else 
    {
      m_mapping[pinIdx].clear_chan(chIdx);
    }
  }
}

bool ChannelPinMapper::TogglePin(int pinIdx, int chIdx) 
{
  bool on = GetPin(pinIdx, chIdx);
  on = !on;
  SetPin(pinIdx, chIdx, on); 
  return on;
}

bool ChannelPinMapper::GetPin(int pinIdx, int chIdx) const
{
  if (pinIdx >= 0 && pinIdx < CHANNELPINMAPPER_MAXPINS)
  {
    return m_mapping[pinIdx].has_chan(chIdx);
  }
  return false;
}

bool ChannelPinMapper::IsStraightPassthrough() const
{
  if (m_nCh != m_nPins) return false;
  PinMapPin tmp;
  tmp.clear();
  for (int i = 0; i < m_nPins; ++i)
  {
    tmp.set_chan(i);
    if (!tmp.equal_to(m_mapping[i])) return false;
    tmp.clear_chan(i);
  }
  return true;
}

#define PINMAPPER_MAGIC 1000

const char *ChannelPinMapper::SaveStateNew(int* pLen)
{
  m_cfgret.Clear();
  int magic = PINMAPPER_MAGIC;
  WDL_Queue__AddToLE(&m_cfgret, &magic);
  WDL_Queue__AddToLE(&m_cfgret, &m_nCh);
  WDL_Queue__AddToLE(&m_cfgret, &m_nPins);
  const int num64 = wdl_max(1,(wdl_min(m_nCh,CHANNELPINMAPPER_MAXPINS) + 63)/64);
  for (int y = 0; y < num64; y ++)
  {
    for (int x = 0; x < m_nPins; x ++)
    {
      const WDL_UINT64 v = m_mapping[x].get_64(y);
      WDL_Queue__AddToLE(&m_cfgret, &v);
    }
  }
  *pLen = m_cfgret.GetSize();
  return (const char*)m_cfgret.Get();
}

bool ChannelPinMapper::LoadState(const char* buf, int len)
{
  WDL_Queue chunk;
  chunk.Add(buf, len);
  int* pMagic = WDL_Queue__GetTFromLE(&chunk, (int*)0);
  if (!pMagic || *pMagic != PINMAPPER_MAGIC) return false;
  int* pNCh = WDL_Queue__GetTFromLE(&chunk, (int*) 0);
  int* pNPins = WDL_Queue__GetTFromLE(&chunk, (int*) 0);
  if (!pNCh || !pNPins) return false;
  const int src_pins = *pNPins;
  SetNPins(src_pins);
  SetNChannels(*pNCh);
  const int num64 = wdl_max(1,(wdl_min(m_nCh,CHANNELPINMAPPER_MAXPINS)+63)/64);
  const int maplen = src_pins * sizeof(WDL_UINT64);
  for (int y = 0; y < num64; y ++)
  {
    if (chunk.Available() < maplen) return y>0;
    const WDL_UINT64 *pMap = (const WDL_UINT64 *)WDL_Queue__GetDataFromLE(&chunk, maplen, sizeof(WDL_UINT64));
    const int sz = wdl_min(m_nPins,src_pins);
    for (int x = 0; x < sz; x ++)
    {
      m_mapping[x].set_64(pMap[x], y);
    }
  }

  return true;
}


AudioBufferContainer::AudioBufferContainer()
{
  m_nCh = 0;
  m_nFrames = 0;
  m_fmt = FMT_32FP;
  m_interleaved = true;
  m_hasData = false;
}

// converts interleaved buffer to interleaved buffer, using min(len_in,len_out) and zeroing any extra samples
// isInput means it reads from track channels and writes to plugin pins
// wantZeroExcessOutput=false means that untouched channels will be preserved in buf_out
void PinMapperConvertBuffers(const double *buf, int len_in, int nch_in, 
                             double *buf_out, int len_out, int nch_out,
                             const ChannelPinMapper *pinmap, bool isInput, bool wantZeroExcessOutput) 
{

  if (pinmap->IsStraightPassthrough() || !pinmap->GetNPins())
  {
    int x;
    char *op = (char *)buf_out;
    const char *ip = (const char *)buf;

    const int ip_adv = nch_in * sizeof(double);

    const int clen = wdl_min(nch_in, nch_out) * sizeof(double);
    const int zlen = nch_out > nch_in ? (nch_out - nch_in) * sizeof(double) : 0;

    const int cplen = wdl_min(len_in,len_out);

    for (x=0;x<cplen;x++)
    {
      memcpy(op,ip,clen);
      op += clen;
      if (zlen) 
      {
        if (wantZeroExcessOutput) memset(op,0,zlen);
        op += zlen;
      }
      ip += ip_adv;
    }
    if (x < len_out && wantZeroExcessOutput) memset(op, 0, (len_out-x)*sizeof(double)*nch_out);
  }
  else
  {
    if (wantZeroExcessOutput) memset(buf_out,0,len_out*nch_out*sizeof(double));

    const int npins = wdl_min(pinmap->GetNPins(),isInput ? nch_out : nch_in);
    const int nchan = isInput ? nch_in : nch_out;

    int p;
    PinMapPin clearmask;
    clearmask.clear();
    for (p = 0; p < npins; p ++)
    {
      const PinMapPin &map = pinmap->m_mapping[p];
      for (unsigned int x = 0; map.enum_chans(&x,nchan); x ++)
      {
        int i=len_in;
        const double *ip = buf + (isInput ? x : p);
        const int out_idx = (isInput ? p : x);

        bool want_zero=false;
        if (!wantZeroExcessOutput)
        {
          if (!clearmask.has_chan(out_idx))
          {
            clearmask.set_chan(out_idx);
            want_zero=true;
          }
        }

        double *op = buf_out + out_idx;

        if (want_zero)
        {
          while (i-- > 0) 
          {
            *op = *ip;
            op += nch_out;
            ip += nch_in;
          }
        }
        else
        {
          while (i-- > 0) 
          {
            *op += *ip;
            op += nch_out;
            ip += nch_in;
          }
        }
      }
    }
  }
}
