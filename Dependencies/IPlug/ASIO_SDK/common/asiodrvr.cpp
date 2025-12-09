//------------------------------------------------------------------------
// Project     : ASIO SDK
//
// Category    : Interfaces
// Filename    : common/asiodrvr.cpp
// Created by  : Steinberg, 05/1996
// Description : Steinberg Audio Stream I/O Helpers
//	c++ superclass to implement asio functionality.
//	From this, you can derive whatever required
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include <string.h>
#include "asiosys.h"
#include "asiodrvr.h"

#if WINDOWS
#error do not use this
AsioDriver::AsioDriver (LPUNKNOWN pUnk, HRESULT *phr) : CUnknown("My AsioDriver", pUnk, phr)
{
}

#else

AsioDriver::AsioDriver()
{
}

#endif

AsioDriver::~AsioDriver()
{
}

ASIOBool AsioDriver::init(void *sysRef)
{
	return ASE_NotPresent;
}

void AsioDriver::getDriverName(char *name)
{
	strcpy(name, "No Driver");
}

long AsioDriver::getDriverVersion()
{
	return 0;
}

void AsioDriver::getErrorMessage(char *string)
{
	strcpy(string, "ASIO Driver Implementation Error!");
}

ASIOError AsioDriver::start()
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::stop()
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::getChannels(long *numInputChannels, long *numOutputChannels)
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::getLatencies(long *inputLatency, long *outputLatency)
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::getBufferSize(long *minSize, long *maxSize,
		long *preferredSize, long *granularity)
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::canSampleRate(ASIOSampleRate sampleRate)
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::getSampleRate(ASIOSampleRate *sampleRate)
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::setSampleRate(ASIOSampleRate sampleRate)
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::getClockSources(ASIOClockSource *clocks, long *numSources)
{
	*numSources = 0;
	return ASE_NotPresent;
}

ASIOError AsioDriver::setClockSource(long reference)
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::getSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp)
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::getChannelInfo(ASIOChannelInfo *info)
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::createBuffers(ASIOBufferInfo *channelInfos, long numChannels,
		long bufferSize, ASIOCallbacks *callbacks)
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::disposeBuffers()
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::controlPanel()
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::future(long selector, void *opt)
{
	return ASE_NotPresent;
}

ASIOError AsioDriver::outputReady()
{
	return ASE_NotPresent;
}