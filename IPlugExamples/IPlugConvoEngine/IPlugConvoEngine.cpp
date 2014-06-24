/*

IPlug convoengine example
(c) Theo Niessink 2010-2013
<http://www.taletn.com/>


This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software in a
   product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.


A simple IPlug plug-in effect that shows how to use WDL's fast convolution
engine.

 
WDL-OL version... slightly modified
 
*/


#include "IPlugConvoEngine.h"
#include "IPlug_include_in_plug_src.h"
#include "IAutoGUI.h"

#ifdef OS_OSX
// need xcode/gcc to treat this file as C++ for the Mac RTAS build
#include "../../WDL/fft.c"
#endif

enum EParams
{
	kDry,
	kWet,
	kNumParams
};


IPlugConvoEngine::IPlugConvoEngine(IPlugInstanceInfo instanceInfo):
	IPLUG_CTOR(kNumParams, 0, instanceInfo),
	#ifdef _USE_R8BRAIN
	mResampler(NULL),
	#endif
	mSampleRate(0)
{
	TRACE;

	GetParam(kDry)->InitDouble("Dry", 0., 0., 1., 0.001);
	GetParam(kWet)->InitDouble("Wet", 1., 0., 1., 0.001);
  
  IGraphics* pGraphics = MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT);
  IText textProps(12, &COLOR_BLACK, "Verdana", IText::kStyleNormal, IText::kAlignNear, 0, IText::kQualityNonAntiAliased);
	GenerateKnobGUI(pGraphics, this, &textProps, &COLOR_WHITE, &COLOR_BLACK, 60, 70);
  AttachGraphics(pGraphics);
}


IPlugConvoEngine::~IPlugConvoEngine()
{
	#ifdef _USE_R8BRAIN
		if (mResampler) delete mResampler;
	#endif
}


void IPlugConvoEngine::OnParamChange(int paramIdx)
{
	IMutexLock lock(this);

	switch (paramIdx)
	{
		case kDry:
			mDry = GetParam(kDry)->Value();
			break;

		case kWet:
			mWet = GetParam(kWet)->Value();
			break;
	}
}


template <class I, class O>
void IPlugConvoEngine::Resample(const I* src, int src_len, double src_srate, O* dest, int dest_len, double dest_srate)
{
	if (dest_len == src_len)
	{
		// Copy
		for (int i = 0; i < dest_len; ++i) *dest++ = (O)*src++;
		return;
	}

	// Resample using WDL's resampler.
	#if defined(_USE_WDL_RESAMPLER)
	mResampler.SetRates(src_srate, dest_srate);
	double scale = src_srate / dest_srate;
	while (dest_len > 0)
	{
		WDL_ResampleSample* p;
		int n = mResampler.ResamplePrepare(mBlockLength, 1, &p), m = n;
		if (n > src_len) n = src_len;
		for (int i = 0; i < n; ++i) *p++ = (WDL_ResampleSample)*src++;
		if (n < m) memset(p, 0, (m - n) * sizeof(WDL_ResampleSample));
		src_len -= n;

		WDL_ResampleSample buf[mBlockLength];
		n = mResampler.ResampleOut(buf, m, m, 1);
		if (n > dest_len) n = dest_len;
		p = buf;
		for (int i = 0; i < n; ++i) *dest++ = (O)(scale * *p++);
		dest_len -= n;
	}
	mResampler.Reset();

	// Resample using r8brain-free-src.
	#elif defined(_USE_R8BRAIN)
	double scale = src_srate / dest_srate;
	while (dest_len > 0)
	{
		double buf[mBlockLength], *p = buf;
		int n = mBlockLength;
		if (n > src_len) n = src_len;
		for (int i = 0; i < n; ++i) *p++ = (double)*src++;
		if (n < mBlockLength) memset(p, 0, (mBlockLength - n) * sizeof(double));
		src_len -= n;

		n = mResampler->process(buf, mBlockLength, p);
		if (n > dest_len) n = dest_len;
		for (int i = 0; i < n; ++i) *dest++ = (O)(scale * *p++);
		dest_len -= n;
	}
	mResampler->clear();

	// Resample using linear interpolation.
	#else
	double pos = 0.;
	double delta = src_srate / dest_srate;
	for (int i = 0; i < dest_len; ++i)
	{
		int idx = int(pos);
		if (idx < src_len)
		{
			double frac = pos - floor(pos);
			double interp = (1. - frac) * src[idx];
			if (++idx < src_len) interp += frac * src[idx];
			pos += delta;
			*dest++ = (O)(delta * interp);
		}
		else
		{
			*dest++ = 0;
		}
	}
	#endif
}


void IPlugConvoEngine::Reset()
{
	TRACE; IMutexLock lock(this);

	// Detect a change in sample rate.
	if (GetSampleRate() != mSampleRate)
	{
		mSampleRate = GetSampleRate();

		const int irLength = sizeof(mIR) / sizeof(mIR[0]);
		const double irSampleRate = 44100.;
		mImpulse.SetNumChannels(1);

		#if defined(_USE_WDL_RESAMPLER)
			mResampler.SetMode(false, 0, true); // Sinc, default size
			mResampler.SetFeedMode(true); // Input driven
		#elif defined(_USE_R8BRAIN)
			if (mResampler) delete mResampler;
			mResampler = new CDSPResampler16IR(irSampleRate, mSampleRate, mBlockLength);
		#endif

		// Resample the impulse response.
		int len = mImpulse.SetLength(ResampleLength(irLength, irSampleRate, mSampleRate));
		if (len) Resample(mIR, irLength, irSampleRate, mImpulse.impulses[0].Get(), len, mSampleRate);

		// Tie the impulse response to the convolution engine.
		mEngine.SetImpulse(&mImpulse);
	}
}


void IPlugConvoEngine::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
	// Send input samples to the convolution engine.
	#if WDL_FFT_REALSIZE == 8
		mEngine.Add(inputs, nFrames, 1);
	#else
	{
		// Convert the input samples from doubles to WDL_FFT_REALs.
		double* in = inputs[0];
		// Use outputs[0] as a temporary buffer.
		WDL_FFT_REAL* tmp = (WDL_FFT_REAL*)outputs[0];
		for (int i = 0; i < nFrames; ++i) *tmp++ = (WDL_FFT_REAL)*in++;
		mEngine.Add((WDL_FFT_REAL**)outputs, nFrames, 1);
	}
	#endif

	double* in = inputs[0];
	double *out_l = outputs[0];
	double *out_r = outputs[1];

	int nAvail = MIN(mEngine.Avail(nFrames), nFrames);

	// If not enough samples are available yet, then only output the dry
	// signal.
	for (int i = 0; i < nFrames - nAvail; ++i) *out_l++ = *out_r++ = mDry * *in++;

	// Output samples from the convolution engine.
	if (nAvail > 0)
	{
		// Apply the dry/wet mix (and convert from WDL_FFT_REALs back to
		// doubles).
		WDL_FFT_REAL* convo = mEngine.Get()[0];
		for (int i = 0; i < nAvail; ++i) *out_l++ = *out_r++ = mDry * *in++ + mWet * *convo++;

		// Remove the sample block from the convolution engine's buffer.
		mEngine.Advance(nAvail);
	}
}


// Impulse response (extracted from ir.wav using the convertor code below)

const float IPlugConvoEngine::mIR[] =
{
	#include "ir.h"
};

/*

#include <stdio.h>
#include "../../../wdlendian.h"

int main(int argc, char* argv[])
{
	FILE* f = fopen(argv[1], "rb");
	fseek(f, 12, SEEK_SET);

	unsigned int i, n;
	for (;;)
	{
		fread(&i, 1, 4, f);
		fread(&n, 1, 4, f);
		n = WDL_bswap32_if_be(n);
		if (i == WDL_bswap32_if_le('data')) break;
		fseek(f, n, SEEK_CUR);
	}

	while (n > 0)
	{
		n -= fread(&i, 1, 4, f);
		printf("%.8ef,\n", (float)WDL_bswapf_if_be(i));
	}

	fclose(f);
	return 0;
}

*/
