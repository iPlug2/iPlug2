#include <cmath>
#include <cstdio>
#include <ctime>
#include <cassert>

#include "wdlendian.h"
#include "wdl_base64.h"

#include "IPlugProcessor.h"

#ifdef OS_WIN
#define strtok_r strtok_s
#endif

template<typename sampleType>
IPlugProcessor<sampleType>::IPlugProcessor(IPlugConfig c, EAPI plugAPI)
  : mLatency(c.latency)
  , mIsInstrument(c.plugIsInstrument)
  , mDoesMIDI(c.plugDoesMidi)
{
  int totalNInChans, totalNOutChans;

  ParseChannelIOStr(c.channelIOStr, mIOConfigs, totalNInChans, totalNOutChans, mMaxNInBuses, mMaxNOutBuses);

  mInData.Resize(totalNInChans);
  mOutData.Resize(totalNOutChans);
  
  sampleType** ppInData = mInData.Get();

  for (int i = 0; i < totalNInChans; ++i, ++ppInData)
  {
    IChannelData<>* pInChannel = new IChannelData<>;
    pInChannel->mConnected = false;
    pInChannel->mData = ppInData;
    mInChannels.Add(pInChannel);
  }

  sampleType** ppOutData = mOutData.Get();

  for (int i = 0; i < totalNOutChans; ++i, ++ppOutData)
  {
    IChannelData<>* pOutChannel = new IChannelData<>;
    pOutChannel->mConnected = false;
    pOutChannel->mData = ppOutData;
    pOutChannel->mIncomingData = nullptr;
    mOutChannels.Add(pOutChannel);
  }
}

template<typename sampleType>
IPlugProcessor<sampleType>::~IPlugProcessor()
{
  TRACE;

  mInChannels.Empty(true);
  mOutChannels.Empty(true);
  mIOConfigs.Empty(true);
 
  if (mLatencyDelay)
    DELETE_NULL(mLatencyDelay);
}

template<typename sampleType>
void IPlugProcessor<sampleType>::ProcessBlock(sampleType** inputs, sampleType** outputs, int nFrames)
{
  int i, nIn = mInChannels.GetSize(), nOut = mOutChannels.GetSize();
  int j = 0;
  for (i = 0; i < nOut; ++i)
  {
    if (i < nIn)
    {
      memcpy(outputs[i], inputs[i], nFrames * sizeof(double));
      j++;
    }
  }
  // zero remaining outs
  for (/* same j */; j < nOut; ++j)
  {
    memset(outputs[j], 0, nFrames * sizeof(double));
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::ProcessMidiMsg(const IMidiMsg& msg)
{
  SendMidiMsg(msg);
}

template<typename sampleType>
bool IPlugProcessor<sampleType>::SendMidiMsgs(WDL_TypedBuf<IMidiMsg>& msgs)
{
  bool rc = true;
  int n = msgs.GetSize();
  
  for (int i = 0; i < n; ++i)
    rc &= SendMidiMsg(msgs.Get()[i]);
  
  return rc;
}

template<typename sampleType>
double IPlugProcessor<sampleType>::GetSamplesPerBeat() const
{
  const double tempo = GetTempo();
  
  if (tempo > 0.0)
    return GetSampleRate() * 60.0 / tempo;
  
  return 0.0;
}

template<typename sampleType>
int IPlugProcessor<sampleType>::MaxNChannelsForBus(ERoute direction, int busIdx) const
{
  if(HasWildcardBus(direction))
    return -1;
  
  const int maxNBuses = MaxNBuses(direction);
  int maxChansOnBuses[maxNBuses];
  memset(maxChansOnBuses, 0, maxNBuses * sizeof(int));
  
  //find the maximum channel count for each input bus
  for (auto configIdx = 0; configIdx < NIOConfigs(); configIdx++)
  {
    IOConfig* pIOConfig = mIOConfigs.Get(configIdx);
    
    for (auto bus = 0; bus < maxNBuses; bus++)
      maxChansOnBuses[bus] = std::max(pIOConfig->NChansOnBusSAFE(direction, bus), maxChansOnBuses[bus]);
  }
  
  return maxChansOnBuses[busIdx];
}

template<typename sampleType>
bool IPlugProcessor<sampleType>::IsInChannelConnected(int chIdx) const
{
  return (chIdx < mInChannels.GetSize() && mInChannels.Get(chIdx)->mConnected);
}

template<typename sampleType>
bool IPlugProcessor<sampleType>::IsOutChannelConnected(int chIdx) const
{
  return (chIdx < mOutChannels.GetSize() && mOutChannels.Get(chIdx)->mConnected);
}

template<typename sampleType>
int IPlugProcessor<sampleType>::NInChansConnected() const
{
  int count = 0;
  
  for (int i = 0; i<mInChannels.GetSize(); i++) {
    count += (int) IsInChannelConnected(i);
  }
  
  return count;
}

template<typename sampleType>
int IPlugProcessor<sampleType>::NOutChansConnected() const
{
  int count = 0;
  
  for (int i = 0; i<mOutChannels.GetSize(); i++) {
    count += (int) IsOutChannelConnected(i);
  }
  
  return count;
}

template<typename sampleType>
bool IPlugProcessor<sampleType>::LegalIO(int NInputChans, int NOutputChans) const
{
  bool legal = false;
  
  for (auto i = 0; i < NIOConfigs() && !legal; ++i)
  {
    IOConfig* pIO = mIOConfigs.Get(i);
    legal = ((NInputChans < 0 || NInputChans == pIO->GetTotalNChannels(ERoute::kInput)) && (NOutputChans < 0 || NOutputChans == pIO->GetTotalNChannels(ERoute::kOutput)));
  }
  
  Trace(TRACELOC, "%d:%d:%s", NInputChans, NOutputChans, (legal ? "legal" : "illegal"));
  return legal;
}

template<typename sampleType>
void IPlugProcessor<sampleType>::LimitToStereoIO()
{
  int NInputChans = NInChannels(), NOutputChans = NOutChannels();
  
  if (NInputChans > 2)
    SetInputChannelConnections(2, NInputChans - 2, false);
  
  if (NOutputChans > 2)
    SetOutputChannelConnections(2, NOutputChans - 2, true);
}

template<typename sampleType>
void IPlugProcessor<sampleType>::SetInputLabel(int idx, const char* pLabel)
{
  if (idx >= 0 && idx < NInChannels())
  {
    mInChannels.Get(idx)->mLabel.Set(pLabel);
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::SetOutputLabel(int idx, const char* pLabel)
{
  if (idx >= 0 && idx < NOutChannels())
  {
    mOutChannels.Get(idx)->mLabel.Set(pLabel);
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::SetLatency(int samples)
{
  mLatency = samples;
  
  if (mLatencyDelay)
  {
    mLatencyDelay->SetDelayTime(mLatency);
  }
}

//static
template<typename sampleType>
int IPlugProcessor<sampleType>::ParseChannelIOStr(const char* IOStr, WDL_PtrList<IOConfig>& channelIOList, int& totalNInChans, int& totalNOutChans, int& totalNInBuses, int& totalNOutBuses)
{
  bool foundAWildcard = false;
  int IOConfigIndex = 0;
  
  DBGMSG("BEGIN IPLUG CHANNEL IO PARSER --------------------------------------------------\n");
  // lamda function to iterate through the period separated buses and check that none have 0 channel count
  auto ParseBusToken = [&foundAWildcard, &IOConfigIndex](ERoute busDir, char* pBusStr, char* pBusStrEnd, int& NBuses, int& NChans, IOConfig* pConfig)
  {
    while (pBusStr != NULL)
    {
      auto NChanOnBus = 0;
      
      if(strcmp(pBusStr, "*") == 0)
      {
        foundAWildcard = true;
        NChanOnBus = -MAX_BUS_CHANS; // we put a negative number in here which will be picked up in the api classes in order to deal with NxN or NxM routings
      }
      else if (sscanf(pBusStr, "%d", &NChanOnBus) == 1)
        ; //don't do anything
      else
      {
        DBGMSG("Error: something wrong in the %s part of this io string: %s.\n", RoutingDirStrs[busDir], pBusStr);
        assert(0);
      }
      NChans += NChanOnBus;
      
      pBusStr = strtok_r(NULL, ".", &pBusStrEnd);
      
      if(NChanOnBus)
      {
        pConfig->AddBusInfo(busDir, NChanOnBus, RoutingDirStrs[busDir]);
        NBuses++;
      }
      else if(NBuses > 0)
      {
        DBGMSG("Error: with multiple %s buses you can't define one with no channels!\n", RoutingDirStrs[busDir]);
        assert(NChanOnBus > 0);
      }
    }
  };
  
  totalNInChans = 0; totalNOutChans = 0;
  totalNInBuses = 0; totalNOutBuses = 0;
  
  char* pChannelIOStr = strdup(IOStr);
  
  char* pIOStrEnd;
  char* pIOStr = strtok_r(pChannelIOStr, " ", &pIOStrEnd); // a single IO string
  
  WDL_PtrList<WDL_String> IOStrlist;
  
  // iterate through the space separated IO configs
  while (pIOStr != NULL)
  {
    IOConfig* pConfig = new IOConfig();
    
    int NInChans = 0, NOutChans = 0;
    int NInBuses = 0, NOutBuses = 0;
    
    char* pIStr = strtok(pIOStr, "-"); // Input buses part of string
    char* pOStr = strtok(NULL, "-");   // Output buses part of string
    
    WDL_String* thisIOStr  = new WDL_String();
    thisIOStr->SetFormatted(10, "%s-%s", pIStr, pOStr);
    
    for (auto str = 0; str < IOStrlist.GetSize(); str++)
    {
      if(strcmp(IOStrlist.Get(str)->Get(), thisIOStr->Get()) == 0)
      {
        DBGMSG("Error: Duplicate IO string. %s\n", thisIOStr->Get());
        assert(0);
      }
    }
    
    IOStrlist.Add(thisIOStr);
    
    char* pIBusStrEnd;
    char* pIBusStr = strtok_r(pIStr, ".", &pIBusStrEnd); // a single input bus
    
    ParseBusToken(kInput, pIBusStr, pIBusStrEnd, NInBuses, NInChans, pConfig);
    
    char* pOBusStrEnd;
    char* pOBusStr = strtok_r(pOStr, ".", &pOBusStrEnd);
    
    ParseBusToken(kOutput, pOBusStr, pOBusStrEnd, NOutBuses, NOutChans, pConfig);
    
    if(foundAWildcard == true && IOConfigIndex > 0)
    {
      DBGMSG("Error: You can only have a single IO config when using wild cards.\n");
      assert(0);
    }
    
    DBGMSG("Channel I/O #%i - %s\n", IOConfigIndex + 1, thisIOStr->Get());
    DBGMSG("               - input bus count: %i, output bus count %i\n", NInBuses, NOutBuses);
    for (auto i = 0; i < NInBuses; i++)
      DBGMSG("               - channel count on input bus %i: %i\n", i + 1, pConfig->NChansOnBusSAFE(kInput, i));
    for (auto i = 0; i < NOutBuses; i++)
      DBGMSG("               - channel count on output bus %i: %i\n", i + 1, pConfig->NChansOnBusSAFE(kOutput, i));
    DBGMSG("               - input channel count across all buses: %i, output channel count across all buses %i\n\n", NInChans, NOutChans);
    
    totalNInChans = std::max(totalNInChans, NInChans);
    totalNOutChans = std::max(totalNOutChans, NOutChans);
    totalNInBuses = std::max(totalNInBuses, NInBuses);
    totalNOutBuses = std::max(totalNOutBuses, NOutBuses);
    
    channelIOList.Add(pConfig);
    
    IOConfigIndex++;
    
    pIOStr = strtok_r(NULL, " ", &pIOStrEnd); // go to next io string
  }
  
  free(pChannelIOStr);
  IOStrlist.Empty(true);
  DBGMSG("%i I/O configs detected\n", IOConfigIndex);
  DBGMSG("Total # in chans: %i, Total # out chans: %i \n\n", totalNInChans, totalNOutChans);
  DBGMSG("END IPLUG CHANNEL IO PARSER --------------------------------------------------\n");
  
  return IOConfigIndex;
}

#pragma mark -

template<typename sampleType>
void IPlugProcessor<sampleType>::SetInputChannelConnections(int idx, int n, bool connected)
{
  int iEnd = std::min(idx + n, mInChannels.GetSize());
  
  for (int i = idx; i < iEnd; ++i)
  {
    IChannelData<>* pInChannel = mInChannels.Get(i);
    pInChannel->mConnected = connected;
    
    if (!connected)
    {
      *(pInChannel->mData) = pInChannel->mScratchBuf.Get();
    }
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::SetOutputChannelConnections(int idx, int n, bool connected)
{
  int iEnd = std::min(idx + n, mOutChannels.GetSize());
  
  for (int i = idx; i < iEnd; ++i)
  {
    IChannelData<>* pOutChannel = mOutChannels.Get(i);
    pOutChannel->mConnected = connected;
    
    if (!connected)
    {
      *(pOutChannel->mData) = pOutChannel->mScratchBuf.Get();
    }
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::AttachInputBuffers(int idx, int n, PLUG_SAMPLE_DST** ppData, int nFrames)
{
  int iEnd = std::min(idx + n, mInChannels.GetSize());
  
  for (int i = idx; i < iEnd; ++i)
  {
    IChannelData<>* pInChannel = mInChannels.Get(i);
    if (pInChannel->mConnected)
    {
      *(pInChannel->mData) = *(ppData++);
    }
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::AttachInputBuffers(int idx, int n, PLUG_SAMPLE_SRC** ppData, int nFrames)
{
  int iEnd = std::min(idx + n, mInChannels.GetSize());
  for (int i = idx; i < iEnd; ++i)
  {
    IChannelData<>* pInChannel = mInChannels.Get(i);
    if (pInChannel->mConnected)
    {
      PLUG_SAMPLE_DST* pScratch = pInChannel->mScratchBuf.Get();
      CastCopy(pScratch, *(ppData++), nFrames);
      *(pInChannel->mData) = pScratch;
    }
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::AttachOutputBuffers(int idx, int n, PLUG_SAMPLE_DST** ppData)
{
  int iEnd = std::min(idx + n, mOutChannels.GetSize());
  for (int i = idx; i < iEnd; ++i)
  {
    IChannelData<>* pOutChannel = mOutChannels.Get(i);
    if (pOutChannel->mConnected)
    {
      *(pOutChannel->mData) = *(ppData++);
    }
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::AttachOutputBuffers(int idx, int n, PLUG_SAMPLE_SRC** ppData)
{
  int iEnd = std::min(idx + n, mOutChannels.GetSize());
  for (int i = idx; i < iEnd; ++i)
  {
    IChannelData<>* pOutChannel = mOutChannels.Get(i);
    if (pOutChannel->mConnected)
    {
      *(pOutChannel->mData) = pOutChannel->mScratchBuf.Get();
      pOutChannel->mIncomingData = *(ppData++);
    }
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::PassThroughBuffers(PLUG_SAMPLE_DST type, int nFrames)
{
  if (mLatency && mLatencyDelay)
  {
    mLatencyDelay->ProcessBlock(mInData.Get(), mOutData.Get(), nFrames);
  }
  else 
  {
    IPlugProcessor<sampleType>::ProcessBlock(mInData.Get(), mOutData.Get(), nFrames);
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::PassThroughBuffers(PLUG_SAMPLE_SRC type, int nFrames)
{
  // for PLUG_SAMPLE_SRC bit buffers, first run the delay (if mLatency) on the PLUG_SAMPLE_DST IPlug buffers
  PassThroughBuffers(PLUG_SAMPLE_DST(0.), nFrames);
  
  int i, n = NOutChannels();
  IChannelData<>** ppOutChannel = mOutChannels.GetList();
  
  for (i = 0; i < n; ++i, ++ppOutChannel)
  {
    IChannelData<>* pOutChannel = *ppOutChannel;
    if (pOutChannel->mConnected)
    {
      CastCopy(pOutChannel->mIncomingData, *(pOutChannel->mData), nFrames);
    }
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::ProcessBuffers(PLUG_SAMPLE_DST type, int nFrames)
{
  ProcessBlock(mInData.Get(), mOutData.Get(), nFrames);
}

template<typename sampleType>
void IPlugProcessor<sampleType>::ProcessBuffers(PLUG_SAMPLE_SRC type, int nFrames)
{
  ProcessBlock(mInData.Get(), mOutData.Get(), nFrames);
  int i, n = NOutChannels();
  IChannelData<>** ppOutChannel = mOutChannels.GetList();
  
  for (i = 0; i < n; ++i, ++ppOutChannel)
  {
    IChannelData<>* pOutChannel = *ppOutChannel;
    
    if (pOutChannel->mConnected)
    {
      CastCopy(pOutChannel->mIncomingData, *(pOutChannel->mData), nFrames);
    }
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::ProcessBuffersAccumulating(PLUG_SAMPLE_SRC type, int nFrames)
{
  ProcessBlock(mInData.Get(), mOutData.Get(), nFrames);
  int i, n = NOutChannels();
  IChannelData<>** ppOutChannel = mOutChannels.GetList();
  
  for (i = 0; i < n; ++i, ++ppOutChannel)
  {
    IChannelData<>* pOutChannel = *ppOutChannel;
    if (pOutChannel->mConnected)
    {
      PLUG_SAMPLE_SRC* pDest = pOutChannel->mIncomingData;
      PLUG_SAMPLE_DST* pSrc = *(pOutChannel->mData);
      
      for (int j = 0; j < nFrames; ++j, ++pDest, ++pSrc)
      {
        *pDest += (PLUG_SAMPLE_SRC) *pSrc;
      }
    }
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::ZeroScratchBuffers()
{
  int i, nIn = NInChannels(), nOut = NOutChannels();

  for (i = 0; i < nIn; ++i)
  {
    IChannelData<>* pInChannel = mInChannels.Get(i);
    memset(pInChannel->mScratchBuf.Get(), 0, mBlockSize * sizeof(PLUG_SAMPLE_DST));
  }

  for (i = 0; i < nOut; ++i)
  {
    IChannelData<>* pOutChannel = mOutChannels.Get(i);
    memset(pOutChannel->mScratchBuf.Get(), 0, mBlockSize * sizeof(PLUG_SAMPLE_DST));
  }
}

template<typename sampleType>
void IPlugProcessor<sampleType>::SetBlockSize(int blockSize)
{
  if (blockSize != mBlockSize)
  {
    int i, nIn = NInChannels(), nOut = NOutChannels();
    
    for (i = 0; i < nIn; ++i)
    {
      IChannelData<>* pInChannel = mInChannels.Get(i);
      pInChannel->mScratchBuf.Resize(blockSize);
      memset(pInChannel->mScratchBuf.Get(), 0, blockSize * sizeof(PLUG_SAMPLE_DST));
    }
    
    for (i = 0; i < nOut; ++i)
    {
      IChannelData<>* pOutChannel = mOutChannels.Get(i);
      pOutChannel->mScratchBuf.Resize(blockSize);
      memset(pOutChannel->mScratchBuf.Get(), 0, blockSize * sizeof(PLUG_SAMPLE_DST));
    }
    
    mBlockSize = blockSize;
  }
}
