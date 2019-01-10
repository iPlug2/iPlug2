/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

/**
 * @file
 * @brief IPlugProcessor implementation.
 * This file #included in the header file... don't include it in regular sources
 */

#include <cmath>
#include <cstdio>
#include <ctime>
#include <cassert>

//#include "IPlugProcessor.h"

#ifdef OS_WIN
#define strtok_r strtok_s
#endif

template<typename T>
IPlugProcessor<T>::IPlugProcessor(IPlugConfig c, EAPI plugAPI)
  : mLatency(c.latency)
  , mPlugType((EIPlugPluginType) c.plugType)
  , mDoesMIDIIn(c.plugDoesMidiIn)
  , mDoesMIDIOut(c.plugDoesMidiOut)
  , mDoesMPE(c.plugDoesMPE)
{
  int totalNInBuses, totalNOutBuses;
  int totalNInChans, totalNOutChans;

  ParseChannelIOStr(c.channelIOStr, mIOConfigs, totalNInChans, totalNOutChans, totalNInBuses, totalNOutBuses);

  mScratchData[ERoute::kInput].Resize(totalNInChans);
  mScratchData[ERoute::kOutput].Resize(totalNOutChans);

  T** ppInData = mScratchData[ERoute::kInput].Get();

  for (auto i = 0; i < totalNInChans; ++i, ++ppInData)
  {
    IChannelData<>* pInChannel = new IChannelData<>;
    pInChannel->mConnected = false;
    pInChannel->mData = ppInData;
    mChannelData[ERoute::kInput].Add(pInChannel);
  }

  T** ppOutData = mScratchData[ERoute::kOutput].Get();

  for (auto i = 0; i < totalNOutChans; ++i, ++ppOutData)
  {
    IChannelData<>* pOutChannel = new IChannelData<>;
    pOutChannel->mConnected = false;
    pOutChannel->mData = ppOutData;
    pOutChannel->mIncomingData = nullptr;
    mChannelData[ERoute::kOutput].Add(pOutChannel);
  }
}

template<typename T>
IPlugProcessor<T>::~IPlugProcessor()
{
  TRACE;

  mChannelData[ERoute::kInput].Empty(true);
  mChannelData[ERoute::kOutput].Empty(true);
  mIOConfigs.Empty(true);
}

template<typename T>
void IPlugProcessor<T>::ProcessBlock(T** inputs, T** outputs, int nFrames)
{
  int i, nIn = mChannelData[ERoute::kInput].GetSize(), nOut = mChannelData[ERoute::kOutput].GetSize();
  int j = 0;
  for (i = 0; i < nOut; ++i)
  {
    if (i < nIn)
    {
      memcpy(outputs[i], inputs[i], nFrames * sizeof(T));
      j++;
    }
  }
  // zero remaining outs
  for (/* same j */; j < nOut; ++j)
  {
    memset(outputs[j], 0, nFrames * sizeof(T));
  }
}

template<typename T>
void IPlugProcessor<T>::ProcessMidiMsg(const IMidiMsg& msg)
{
  SendMidiMsg(msg);
}

template<typename T>
bool IPlugProcessor<T>::SendMidiMsgs(WDL_TypedBuf<IMidiMsg>& msgs)
{
  bool rc = true;
  int n = msgs.GetSize();

  for (auto i = 0; i < n; ++i)
    rc &= SendMidiMsg(msgs.Get()[i]);

  return rc;
}

template<typename T>
double IPlugProcessor<T>::GetSamplesPerBeat() const
{
  const double tempo = GetTempo();

  if (tempo > 0.0)
    return GetSampleRate() * 60.0 / tempo;

  return 0.0;
}

#pragma mark -

template<typename T>
int IPlugProcessor<T>::MaxNBuses(ERoute direction) const
{
  int maxNBuses = 0;
  //find the maximum channel count for each input or output bus
  for (auto configIdx = 0; configIdx < NIOConfigs(); configIdx++)
  {
    maxNBuses = std::max(mIOConfigs.Get(configIdx)->NBuses(direction), maxNBuses);
  }

  return maxNBuses;
}

template<typename T>
int IPlugProcessor<T>::MaxNChannelsForBus(ERoute direction, int busIdx) const
{
  if(HasWildcardBus(direction))
    return -1;

  const int maxNBuses = MaxNBuses(direction);
  int maxChansOnBuses[maxNBuses];
  memset(maxChansOnBuses, 0, maxNBuses * sizeof(int));

  //find the maximum channel count for each input or output bus
  for (auto configIdx = 0; configIdx < NIOConfigs(); configIdx++)
  {
    IOConfig* pIOConfig = mIOConfigs.Get(configIdx);

    for (auto bus = 0; bus < maxNBuses; bus++)
      maxChansOnBuses[bus] = std::max(pIOConfig->NChansOnBusSAFE(direction, bus), maxChansOnBuses[bus]);
  }

  return maxChansOnBuses[busIdx];
}

template<typename T>
int IPlugProcessor<T>::NChannelsConnected(ERoute direction) const
{
  const WDL_PtrList<IChannelData<>>& channelData = mChannelData[direction];

  int count = 0;
  for (auto i = 0; i<channelData.GetSize(); i++)
  {
    count += (int) IsChannelConnected(direction, i);
  }

  return count;
}

template<typename T>
bool IPlugProcessor<T>::LegalIO(int NInputChans, int NOutputChans) const
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

template<typename T>
void IPlugProcessor<T>::LimitToStereoIO()
{
  if (MaxNChannels(ERoute::kInput) > 2)
    SetChannelConnections(ERoute::kInput, 2, MaxNChannels(ERoute::kInput) - 2, false);

  if (MaxNChannels(ERoute::kOutput) > 2)
    SetChannelConnections(ERoute::kOutput, 2, MaxNChannels(ERoute::kOutput) - 2, true);
}

template<typename T>
void IPlugProcessor<T>::SetChannelLabel(ERoute direction, int idx, const char* formatStr, bool zeroBased)
{
  if (idx >= 0 && idx < MaxNChannels(direction))
    mChannelData[direction].Get(idx)->mLabel.SetFormatted(MAX_CHAN_NAME_LEN, formatStr, idx+(!zeroBased));
}

template<typename T>
void IPlugProcessor<T>::SetLatency(int samples)
{
  mLatency = samples;

  if (mLatencyDelay)
    mLatencyDelay->SetDelayTime(mLatency);
}

//static
template<typename T>
int IPlugProcessor<T>::ParseChannelIOStr(const char* IOStr, WDL_PtrList<IOConfig>& channelIOList, int& totalNInChans, int& totalNOutChans, int& totalNInBuses, int& totalNOutBuses)
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

    ParseBusToken(ERoute::kInput, pIBusStr, pIBusStrEnd, NInBuses, NInChans, pConfig);

    char* pOBusStrEnd;
    char* pOBusStr = strtok_r(pOStr, ".", &pOBusStrEnd);

    ParseBusToken(ERoute::kOutput, pOBusStr, pOBusStrEnd, NOutBuses, NOutChans, pConfig);

    if(foundAWildcard == true && IOConfigIndex > 0)
    {
      DBGMSG("Error: You can only have a single IO config when using wild cards.\n");
      assert(0);
    }

    DBGMSG("Channel I/O #%i - %s\n", IOConfigIndex + 1, thisIOStr->Get());
    DBGMSG("               - input bus count: %i, output bus count %i\n", NInBuses, NOutBuses);
    for (auto i = 0; i < NInBuses; i++)
      DBGMSG("               - channel count on input bus %i: %i\n", i + 1, pConfig->NChansOnBusSAFE(ERoute::kInput, i));
    for (auto i = 0; i < NOutBuses; i++)
      DBGMSG("               - channel count on output bus %i: %i\n", i + 1, pConfig->NChansOnBusSAFE(ERoute::kOutput, i));
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

template<typename T>
void IPlugProcessor<T>::SetChannelConnections(ERoute direction, int idx, int n, bool connected)
{
  WDL_PtrList<IChannelData<>>& channelData = mChannelData[direction];

  const auto endIdx = std::min(idx + n, channelData.GetSize());

  for (auto i = idx; i < endIdx; ++i)
  {
    IChannelData<>* pChannel = channelData.Get(i);
    pChannel->mConnected = connected;

    if (!connected)
      *(pChannel->mData) = pChannel->mScratchBuf.Get();
  }
}

template<typename T>
void IPlugProcessor<T>::AttachBuffers(ERoute direction, int idx, int n, PLUG_SAMPLE_DST** ppData, int)
{
  WDL_PtrList<IChannelData<>>& channelData = mChannelData[direction];

  const auto endIdx = std::min(idx + n, channelData.GetSize());

  for (auto i = idx; i < endIdx; ++i)
  {
    IChannelData<>* pChannel = channelData.Get(i);

    if (pChannel->mConnected)
      *(pChannel->mData) = *(ppData++);
  }
}

template<typename T>
void IPlugProcessor<T>::AttachBuffers(ERoute direction, int idx, int n, PLUG_SAMPLE_SRC** ppData, int nFrames)
{
  WDL_PtrList<IChannelData<>>& channelData = mChannelData[direction];

  const auto endIdx = std::min(idx + n, channelData.GetSize());

  for (auto i = idx; i < endIdx; ++i)
  {
    IChannelData<>* pChannel = channelData.Get(i);

    if (pChannel->mConnected)
    {
      if (direction == ERoute::kInput)
      {
        PLUG_SAMPLE_DST* pScratch = pChannel->mScratchBuf.Get();
        CastCopy(pScratch, *(ppData++), nFrames);
        *(pChannel->mData) = pScratch;
      }
      else // output
      {
        *(pChannel->mData) = pChannel->mScratchBuf.Get();
        pChannel->mIncomingData = *(ppData++);
      }
    }
  }
}

template<typename T>
void IPlugProcessor<T>::PassThroughBuffers(PLUG_SAMPLE_DST type, int nFrames)
{
  if (mLatency && mLatencyDelay)
    mLatencyDelay->ProcessBlock(mScratchData[ERoute::kInput].Get(), mScratchData[ERoute::kOutput].Get(), nFrames);
  else
    IPlugProcessor<T>::ProcessBlock(mScratchData[ERoute::kInput].Get(), mScratchData[ERoute::kOutput].Get(), nFrames);
}

template<typename T>
void IPlugProcessor<T>::PassThroughBuffers(PLUG_SAMPLE_SRC type, int nFrames)
{
  // for PLUG_SAMPLE_SRC bit buffers, first run the delay (if mLatency) on the PLUG_SAMPLE_DST IPlug buffers
  PassThroughBuffers(PLUG_SAMPLE_DST(0.), nFrames);

  int i, n = MaxNChannels(ERoute::kOutput);
  IChannelData<>** ppOutChannel = mChannelData[ERoute::kOutput].GetList();

  for (i = 0; i < n; ++i, ++ppOutChannel)
  {
    IChannelData<>* pOutChannel = *ppOutChannel;
    if (pOutChannel->mConnected)
    {
      CastCopy(pOutChannel->mIncomingData, *(pOutChannel->mData), nFrames);
    }
  }
}

template<typename T>
void IPlugProcessor<T>::ProcessBuffers(PLUG_SAMPLE_DST type, int nFrames)
{
  ProcessBlock(mScratchData[ERoute::kInput].Get(), mScratchData[ERoute::kOutput].Get(), nFrames);
}

template<typename T>
void IPlugProcessor<T>::ProcessBuffers(PLUG_SAMPLE_SRC type, int nFrames)
{
  ProcessBlock(mScratchData[ERoute::kInput].Get(), mScratchData[ERoute::kOutput].Get(), nFrames);
  int i, n = MaxNChannels(ERoute::kOutput);
  IChannelData<>** ppOutChannel = mChannelData[ERoute::kOutput].GetList();

  for (i = 0; i < n; ++i, ++ppOutChannel)
  {
    IChannelData<>* pOutChannel = *ppOutChannel;

    if (pOutChannel->mConnected)
    {
      CastCopy(pOutChannel->mIncomingData, *(pOutChannel->mData), nFrames);
    }
  }
}

template<typename T>
void IPlugProcessor<T>::ProcessBuffersAccumulating(int nFrames)
{
  ProcessBlock(mScratchData[ERoute::kInput].Get(), mScratchData[ERoute::kOutput].Get(), nFrames);
  int i, n = MaxNChannels(ERoute::kOutput);
  IChannelData<>** ppOutChannel = mChannelData[ERoute::kOutput].GetList();

  for (i = 0; i < n; ++i, ++ppOutChannel)
  {
    IChannelData<>* pOutChannel = *ppOutChannel;
    if (pOutChannel->mConnected)
    {
      PLUG_SAMPLE_SRC* pDest = pOutChannel->mIncomingData;
      PLUG_SAMPLE_DST* pSrc = *(pOutChannel->mData); // TODO : check this: PLUG_SAMPLE_DST will allways be float, because this is only for VST2 accumulating
      for (int j = 0; j < nFrames; ++j, ++pDest, ++pSrc)
      {
        *pDest += (PLUG_SAMPLE_SRC) *pSrc;
      }
    }
  }
}

template<typename T>
void IPlugProcessor<T>::ZeroScratchBuffers()
{
  int i, nIn = MaxNChannels(ERoute::kInput), nOut = MaxNChannels(ERoute::kOutput);

  for (i = 0; i < nIn; ++i)
  {
    IChannelData<>* pInChannel = mChannelData[ERoute::kInput].Get(i);
    memset(pInChannel->mScratchBuf.Get(), 0, mBlockSize * sizeof(PLUG_SAMPLE_DST));
  }

  for (i = 0; i < nOut; ++i)
  {
    IChannelData<>* pOutChannel = mChannelData[ERoute::kOutput].Get(i);
    memset(pOutChannel->mScratchBuf.Get(), 0, mBlockSize * sizeof(PLUG_SAMPLE_DST));
  }
}

template<typename T>
void IPlugProcessor<T>::SetBlockSize(int blockSize)
{
  if (blockSize != mBlockSize)
  {
    int i, nIn = MaxNChannels(ERoute::kInput), nOut = MaxNChannels(ERoute::kOutput);

    for (i = 0; i < nIn; ++i)
    {
      IChannelData<>* pInChannel = mChannelData[ERoute::kInput].Get(i);
      pInChannel->mScratchBuf.Resize(blockSize);
      memset(pInChannel->mScratchBuf.Get(), 0, blockSize * sizeof(PLUG_SAMPLE_DST));
    }

    for (i = 0; i < nOut; ++i)
    {
      IChannelData<>* pOutChannel = mChannelData[ERoute::kOutput].Get(i);
      pOutChannel->mScratchBuf.Resize(blockSize);
      memset(pOutChannel->mScratchBuf.Get(), 0, blockSize * sizeof(PLUG_SAMPLE_DST));
    }

    mBlockSize = blockSize;
  }
}
