//------------------------------------------------------------------------
// Project     : ASIO SDK
//
// Category    : Helpers
// Filename    : host/ASIOConvertSamples.h
// Created by  : Steinberg, 05/1996
// Description : Steinberg Audio Stream I/O Helpers
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2025, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#ifndef __ASIOConvertSamples__
#define __ASIOConvertSamples__

class ASIOConvertSamples
{
public:
	ASIOConvertSamples();
	~ASIOConvertSamples() {}

	// format converters, input 32 bit integer
	// mono
	void convertMono8(long *source, char *dest, long frames);
	void convertMono8Unsigned(long *source, char *dest, long frames);
	void convertMono16(long *source, short *dest, long frames);
	void convertMono16SmallEndian(long *source, short *dest, long frames);
	void convertMono24(long *source, char *dest, long frames);
	void convertMono24SmallEndian(long *source, char *dest, long frames);

	// stereo interleaved
	void convertStereo8Interleaved(long *left, long *right, char *dest, long frames);
	void convertStereo8InterleavedUnsigned(long *left, long *right, char *dest, long frames);
	void convertStereo16Interleaved(long *left, long *right, short *dest, long frames);
	void convertStereo16InterleavedSmallEndian(long *left, long *right, short *dest, long frames);
	void convertStereo24Interleaved(long *left, long *right, char *dest, long frames);
	void convertStereo24InterleavedSmallEndian(long *left, long *right, char *dest, long frames);

	// stereo split
	void convertStereo8(long *left, long *right, char *dLeft, char *dRight, long frames);
	void convertStereo8Unsigned(long *left, long *right, char *dLeft, char *dRight, long frames);
	void convertStereo16(long *left, long *right, short *dLeft, short *dRight, long frames);
	void convertStereo16SmallEndian(long *left, long *right, short *dLeft, short *dRight, long frames);
	void convertStereo24(long *left, long *right, char *dLeft, char *dRight, long frames);
	void convertStereo24SmallEndian(long *left, long *right, char *dLeft, char *dRight, long frames);

	// integer in place conversions

	void int32msb16to16inPlace(long *in, long frames);
	void int32lsb16to16inPlace(long *in, long frames);
	void int32msb16shiftedTo16inPlace(long *in1, long frames, long shift);
	void int24msbto16inPlace(unsigned char *in, long frames);

	// integer to integer

	void shift32(void* buffer, long shiftAmount, long targetByteWidth,
		bool reverseEndian,	long frames);
	void reverseEndian(void* buffer, long byteWidth, long frames);

	void int32to16inPlace(void* buffer, long frames);
	void int24to16inPlace(void* buffer, long frames);
	void int32to24inPlace(void* buffer, long frames);
	void int16to24inPlace(void* buffer, long frames);
	void int24to32inPlace(void* buffer, long frames);
	void int16to32inPlace(void* buffer, long frames);

	// float to integer
	
	void float32toInt16inPlace(float* buffer, long frames);
	void float32toInt24inPlace(float* buffer, long frames);
	void float32toInt32inPlace(float* buffer, long frames);
};

#endif
