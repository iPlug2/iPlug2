#ifndef __IPlugConvoEngine_H__
#define __IPlugConvoEngine_H__

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

*/


#include "IPlug_include_in_plug_hdr.h"
#include "convoengine.h"

#if defined(_USE_WDL_RESAMPLER)
	#include "resample.h"
#elif defined(_USE_R8BRAIN)
	#include "r8brain/CDSPResampler.h"
	using namespace r8b;
#endif


class IPlugConvoEngine: public IPlug
{
public:
	IPlugConvoEngine(IPlugInstanceInfo instanceInfo);
	~IPlugConvoEngine();

	void OnParamChange(int paramIdx);
	void Reset();
	void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

	// Returns destination length
	inline int ResampleLength(int src_len, double src_srate, double dest_srate) const
	{
		return int(dest_srate / src_srate * (double)src_len + 0.5);
	}

	template <class I, class O> void Resample(const I* src, int src_len, double src_srate, O* dest, int dest_len, double dest_srate);

private:
	static const float mIR[512];

	WDL_ImpulseBuffer mImpulse;
	WDL_ConvolutionEngine_Div mEngine;

	#if defined(_USE_WDL_RESAMPLER) || defined(_USE_R8BRAIN)
	static const int mBlockLength = 64;
	#endif

	#if defined(_USE_WDL_RESAMPLER)
	WDL_Resampler mResampler;
	#elif defined(_USE_R8BRAIN)
	CDSPResampler16IR* mResampler;
	#endif

	double mDry, mWet;

	double mSampleRate;
};


#endif // __IPlugConvoEngine_H__
