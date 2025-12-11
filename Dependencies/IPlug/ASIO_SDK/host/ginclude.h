//------------------------------------------------------------------------
// Project     : ASIO SDK
//
// Category    : Helpers
// Filename    : host/ginclude.h
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

#ifndef __gInclude__
#define __gInclude__

#if SGI 
	#undef BEOS 
	#undef MAC 
	#undef WINDOWS
	//
	#define ASIO_BIG_ENDIAN 1
	#define ASIO_CPU_MIPS 1
#elif defined(_WIN32) || defined(_WIN64)
	#undef BEOS 
	#undef MAC 
	#undef SGI
	#define WINDOWS 1
	#define ASIO_LITTLE_ENDIAN 1
	#define ASIO_CPU_X86 1
#elif BEOS
	#undef MAC 
	#undef SGI
	#undef WINDOWS
	#define ASIO_LITTLE_ENDIAN 1
	#define ASIO_CPU_X86 1
	//
#else
	#define MAC 1
	#undef BEOS 
	#undef WINDOWS
	#undef SGI
	#define ASIO_BIG_ENDIAN 1
	#define ASIO_CPU_PPC 1
#endif

// always
#define NATIVE_INT64 0
#define IEEE754_64FLOAT 1

#endif	// __gInclude__
