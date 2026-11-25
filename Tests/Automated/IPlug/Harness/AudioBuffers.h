/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include <vector>

/** Owns nChans x nFrames samples and exposes the T** that hosts hand to plug-ins */
template <typename T>
class AudioBuffers
{
public:
  AudioBuffers(int nChans, int nFrames, T fill = T(0))
  : mData(static_cast<size_t>(nChans), std::vector<T>(static_cast<size_t>(nFrames), fill))
  {
    for (auto& chan : mData)
      mPtrs.push_back(chan.data());
  }

  AudioBuffers(const AudioBuffers&) = delete;
  AudioBuffers& operator=(const AudioBuffers&) = delete;

  T** Get() { return mPtrs.data(); }
  std::vector<T>& operator[](int chan) { return mData[static_cast<size_t>(chan)]; }
  int NChans() const { return static_cast<int>(mData.size()); }
  int NFrames() const { return mData.empty() ? 0 : static_cast<int>(mData[0].size()); }

private:
  std::vector<std::vector<T>> mData;
  std::vector<T*> mPtrs;
};
