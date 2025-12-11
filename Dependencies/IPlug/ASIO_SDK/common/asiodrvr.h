//------------------------------------------------------------------------
// Project     : ASIO SDK
//
// Category    : Interfaces
// Filename    : common/asiodrvr.h
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

#ifndef _asiodrvr_
#define _asiodrvr_

// cpu and os system we are running on
#include "asiosys.h"
// basic "C" interface
#include "asio.h"

class AsioDriver;
extern AsioDriver *getDriver();		// for generic constructor 

#if WINDOWS
#include <windows.h>
#include "combase.h"
#include "iasiodrv.h"
class AsioDriver : public IASIO ,public CUnknown
{
public:
	AsioDriver(LPUNKNOWN pUnk, HRESULT *phr);

	DECLARE_IUNKNOWN
	// Factory method
	static CUnknown *CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);
	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE NonDelegatingQueryInterface(REFIID riid,void **ppvObject);

#else

class AsioDriver
{
public:
	AsioDriver();
#endif
	virtual ~AsioDriver();

	virtual ASIOBool init(void* sysRef);
	virtual void getDriverName(char *name);	// max 32 bytes incl. terminating zero
	virtual long getDriverVersion();
	virtual void getErrorMessage(char *string);	// max 124 bytes incl.

	virtual ASIOError start();
	virtual ASIOError stop();

	virtual ASIOError getChannels(long *numInputChannels, long *numOutputChannels);
	virtual ASIOError getLatencies(long *inputLatency, long *outputLatency);
	virtual ASIOError getBufferSize(long *minSize, long *maxSize,
		long *preferredSize, long *granularity);

	virtual ASIOError canSampleRate(ASIOSampleRate sampleRate);
	virtual ASIOError getSampleRate(ASIOSampleRate *sampleRate);
	virtual ASIOError setSampleRate(ASIOSampleRate sampleRate);
	virtual ASIOError getClockSources(ASIOClockSource *clocks, long *numSources);
	virtual ASIOError setClockSource(long reference);

	virtual ASIOError getSamplePosition(ASIOSamples *sPos, ASIOTimeStamp *tStamp);
	virtual ASIOError getChannelInfo(ASIOChannelInfo *info);

	virtual ASIOError createBuffers(ASIOBufferInfo *bufferInfos, long numChannels,
		long bufferSize, ASIOCallbacks *callbacks);
	virtual ASIOError disposeBuffers();

	virtual ASIOError controlPanel();
	virtual ASIOError future(long selector, void *opt);
	virtual ASIOError outputReady();
};
#endif
