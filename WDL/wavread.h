/*
  WDL - wavread.h
  Copyright (C) 2012-2020 Theo Niessink
  <http://www.taletn.com/>

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.


  This file provides a simple class for reading 8, 16 or 24 bit PCM, or 32
  or 64 bit floating point WAV files.

*/


#ifndef _WAVREAD_H_
#define _WAVREAD_H_


#include <stdio.h>
#include <string.h>

#include "pcmfmtcvt.h"
#include "wdlendian.h"
#include "wdlstring.h"
#include "win32_utf8.h"
#include "wdltypes.h"

#if defined(WAVEREADER_MAX_NCH) && WAVEREADER_MAX_NCH > 64
#include "heapbuf.h"
#endif

class WaveReader
{
public:
	WaveReader(): m_fp(NULL), m_size(0), m_nch(0), m_srate(0), m_bps(0) {}

	WaveReader(const char* filename) { Open(filename); }

	int Open(const char* filename)
	{
		m_fn.Set(filename);
		m_size = m_nch = m_srate = m_bps = 0;

		m_fp = fopen(filename, "rb");
		if (!m_fp) return 0;

		unsigned char buf[16];

		static const unsigned int RIFF = 'FFIR', WAVE = 'EVAW';
		if (!(fread(buf, 12, 1, m_fp) && geti32(buf) == RIFF && geti32(buf + 8) == WAVE))
		{
			Close();
			return 0;
		}

		#ifdef WAVEREADER_STRICT
		{
			const unsigned int size = geti32(buf + 4);
			long int pos, len;
			if (!((pos = ftell(m_fp)) >= 0 && !fseek(m_fp, 0, SEEK_END) && (len = ftell(m_fp)) >= 0 && !fseek(m_fp, pos, SEEK_SET) && len == 8 + size))
			{
				Close();
				return 0;
			}
		}
		#endif

		static const unsigned int fmt = ' tmf', data = 'atad';
		for (;;)
		{
			if (!fread(buf, 8, 1, m_fp))
			{
				Close();
				return 0;
			}
			const unsigned int id = geti32(buf);
			unsigned int size = geti32(buf + 4);

			if (id == fmt && size >= 16)
			{
				size -= 16;
				int ok = (int)fread(buf, 16, 1, m_fp);

				const int format = geti16(buf);
				m_nch = geti16(buf + 2);
				m_srate = geti32(buf + 4);
				m_bps = geti16(buf + 14);

				ok &= (format == 1 && (m_bps == 8 || m_bps == 16 || m_bps == 24)) // PCM
				   || (format == 3 && (m_bps == 32 || m_bps == 64)); // Float
				ok &= m_nch > 0;

				#ifdef WAVEREADER_STRICT
				{
					ok &= m_srate > 0;
					const unsigned int blockalign = geti16(buf + 12);
					ok &= blockalign == m_nch * m_bps/8; // block alignment
					ok &= geti32(buf + 8) == blockalign * m_srate; // bytes_per_sec
				}
				#endif

				if (!ok) m_nch = m_srate = m_bps = 0;
			}
			else if (id == data && size > 0 && m_bps)
			{
				const unsigned int blockalign = m_nch * m_bps/8;
				#ifdef WAVEREADER_STRICT
				if (size % blockalign == 0)
				#endif
				{
					m_size = size / blockalign;
					break;
				}
			}

			if (fseek(m_fp, size, SEEK_CUR))
			{
				Close();
				return 0;
			}
		}

		return !!m_fp;
	}

	void Close()
	{
		if (m_fp)
		{
			fclose(m_fp);
			m_fp = NULL;
		}
	}

	~WaveReader()
	{
		Close();
	}

	const char* GetFileName() const { return m_fn.Get(); }

	int Status() const { return !!m_fp; }

	// Length/size in samples.
	unsigned int GetLength() const { return m_size; }
	unsigned int GetSize() const { return m_size * m_nch; }

	size_t ReadRaw(void* buf, size_t len)
	{
		return m_fp ? fread(buf, 1, len, m_fp) : 0;
	}

	unsigned int ReadFloats(float* samples, unsigned int nsamples)
	{
		if (!m_fp) return 0;

		unsigned int i = 0, n = m_size * m_nch;
		if (nsamples < n) n = nsamples;

		if (m_bps == 8)
		{
			for (; i < n; ++i)
			{
				int a = fgetc(m_fp);
				if (a == EOF) break;
				UINT8_TO_float(*samples, a);
				samples++;
			}
		}
		else if (m_bps == 16)
		{
			for (; i < n; ++i)
			{
				short a;
				if (!fread(&a, 2, 1, m_fp)) break;
				WDL_BSWAP16_IF_BE(a);
				INT16_TO_float(*samples, a);
				samples++;
			}
		}
		else if (m_bps == 24)
		{
			for (; i < n; ++i)
			{
				unsigned char a[3];
				if (!fread(a, 3, 1, m_fp)) break;
				i24_to_float(a, samples);
				samples++;
			}
		}
		else if (m_bps == 32)
		{
		#ifdef WDL_LITTLE_ENDIAN
			i = (unsigned int)fread(samples, 4, n, m_fp);
		#else
			n = (unsigned int)fread(samples, 4, n, m_fp);
			for (; i < n; ++i)
			{
				const unsigned int a = geti32(samples);
				*samples++ = WDL_bswapf_if_be(a);
			}
		#endif
		}
		else if (m_bps == 64)
		{
			for (; i < n; ++i)
			{
				WDL_UINT64 a;
				if (!fread(&a, 8, 1, m_fp)) break;
				*samples++ = (float)WDL_bswapf_if_be(a);
			}
		}

		return i;
	}

	unsigned int ReadDoubles(double* samples, unsigned int nsamples)
	{
		if (!m_fp) return 0;

		unsigned int i = 0, n = m_size * m_nch;
		if (nsamples < n) n = nsamples;

		if (m_bps == 8)
		{
			for (; i < n; ++i)
			{
				int a = fgetc(m_fp);
				if (a == EOF) break;
				UINT8_TO_double(*samples, a);
				samples++;
			}
		}
		else if (m_bps == 16)
		{
			for (; i < n; ++i)
			{
				short a;
				if (!fread(&a, 2, 1, m_fp)) break;
				WDL_BSWAP16_IF_BE(a);
				INT16_TO_double(*samples, a);
				samples++;
			}
		}
		else if (m_bps == 24)
		{
			for (; i < n; ++i)
			{
				unsigned char a[3];
				if (!fread(a, 3, 1, m_fp)) break;
				i24_to_double(a, samples);
				samples++;
			}
		}
		else if (m_bps == 32)
		{
			for (; i < n; ++i)
			{
				unsigned int a;
				if (!fread(&a, 4, 1, m_fp)) break;
				*samples++ = WDL_bswapf_if_be(a);
			}
		}
		else if (m_bps == 64)
		{
		#ifdef WDL_LITTLE_ENDIAN
			i = (unsigned int)fread(samples, 8, n, m_fp);
		#else
			n = (unsigned int)fread(samples, 8, n, m_fp);
			for (; i < n; ++i)
			{
				const WDL_UINT64 a = geti64(samples);
				*samples++ = WDL_bswapf_if_be(a);
			}
		#endif
		}

		return i;
	}

	unsigned int ReadFloatsNI(float** samples, unsigned int offs, unsigned int nsamples, int nchdest = 0)
	{
		if (!m_fp) return 0;

		int nch = m_nch;
		if (nchdest < 1) nchdest = nch;
		else if (nchdest < nch) nch = nchdest;

		unsigned char stackbuf[STACKBUF_MAX_SIZE], *tmpbuf = GetTmpBuf(stackbuf);
		if (!tmpbuf) return 0;

		unsigned int i = 0, n = m_size;
		if (nsamples < n) n = nsamples;

		if (m_bps == 8)
		{
			for (; i < n; ++i)
			{
				if (!fread(tmpbuf, m_nch, 1, m_fp)) return i;
				for (int ch = 0; ch < nch; ++ch)
				{
					UINT8_TO_float(samples[ch][offs + i], tmpbuf[ch]);
				}
			}
		}
		else if (m_bps == 16)
		{
			for (; i < n; ++i)
			{
				if (!fread(tmpbuf, m_nch * 2, 1, m_fp)) return i;
				for (int ch = 0; ch < nch; ++ch)
				{
					short a = geti16(tmpbuf + ch * 2);
					WDL_BSWAP16_IF_BE(a);
					INT16_TO_float(samples[ch][offs + i], a);
				}
			}
		}
		else if (m_bps == 24)
		{
			for (; i < n; ++i)
			{
				if (!fread(tmpbuf, m_nch * 3, 1, m_fp)) return i;
				for (int ch = 0; ch < nch; ++ch)
				{
					i24_to_float(tmpbuf + ch * 3, samples[ch] + offs + i);
				}
			}
		}
		else if (m_bps == 32)
		{
			for (; i < n; ++i)
			{
				if (!fread(tmpbuf, m_nch * 4, 1, m_fp)) return i;
				for (int ch = 0; ch < nch; ++ch)
				{
					const unsigned int a = geti32(tmpbuf + ch * 4);
					samples[ch][offs + i] = WDL_bswapf_if_be(a);
				}
			}
		}
		else if (m_bps == 64)
		{
			for (; i < n; ++i)
			{
				if (!fread(tmpbuf, m_nch * 8, 1, m_fp)) return i;
				for (int ch = 0; ch < nch; ++ch)
				{
					const WDL_UINT64 a = geti64(tmpbuf + ch * 8);
					samples[ch][offs + i] = (float)WDL_bswapf_if_be(a);
				}
			}
		}

		if (nchdest > nch && i)
		{
			for (int ch = nch; ch < nchdest; ++ch)
			{
				memcpy(samples[ch] + offs, samples[ch % nch] + offs, i * 4);
			}
		}

		return i;
	}

	unsigned int ReadDoublesNI(double** samples, unsigned int offs, unsigned int nsamples, int nchdest = 0)
	{
		if (!m_fp) return 0;

		int nch = m_nch;
		if (nchdest < 1) nchdest = nch;
		else if (nchdest < nch) nch = nchdest;

		unsigned char stackbuf[STACKBUF_MAX_SIZE], *tmpbuf = GetTmpBuf(stackbuf);
		if (!tmpbuf) return 0;

		unsigned int i = 0, n = m_size;
		if (nsamples < n) n = nsamples;

		if (m_bps == 8)
		{
			for (; i < n; ++i)
			{
				if (!fread(tmpbuf, m_nch, 1, m_fp)) return i;
				for (int ch = 0; ch < nch; ++ch)
				{
					UINT8_TO_double(samples[ch][offs + i], tmpbuf[ch]);
				}
			}
		}
		else if (m_bps == 16)
		{
			for (; i < n; ++i)
			{
				if (!fread(tmpbuf, m_nch * 2, 1, m_fp)) return i;
				for (int ch = 0; ch < nch; ++ch)
				{
					short a = geti16(tmpbuf + ch * 2);
					WDL_BSWAP16_IF_BE(a);
					INT16_TO_double(samples[ch][offs + i], a);
				}
			}
		}
		else if (m_bps == 24)
		{
			for (; i < n; ++i)
			{
				if (!fread(tmpbuf, m_nch * 3, 1, m_fp)) return i;
				for (int ch = 0; ch < nch; ++ch)
				{
					i24_to_double(tmpbuf + ch * 3, samples[ch] + offs + i);
				}
			}
		}
		else if (m_bps == 32)
		{
			for (; i < n; ++i)
			{
				if (!fread(tmpbuf, m_nch * 4, 1, m_fp)) return i;
				for (int ch = 0; ch < nch; ++ch)
				{
					const unsigned int a = geti32(tmpbuf + ch * 4);
					samples[ch][offs + i] = WDL_bswapf_if_be(a);
				}
			}
		}
		else if (m_bps == 64)
		{
			for (; i < n; ++i)
			{
				if (!fread(tmpbuf, m_nch * 8, 1, m_fp)) return i;
				for (int ch = 0; ch < nch; ++ch)
				{
					const WDL_UINT64 a = geti64(tmpbuf + ch * 8);
					samples[ch][offs + i] = WDL_bswapf_if_be(a);
				}
			}
		}

		if (nchdest > nch && i)
		{
			for (int ch = nch; ch < nchdest; ++ch)
			{
				memcpy(samples[ch] + offs, samples[ch % nch] + offs, i * 8);
			}
		}

		return i;
	}

	int get_nch() const { return m_nch; }
	int get_srate() const { return m_srate; }
	int get_bps() const { return m_bps; }

private:
	static unsigned short geti16(const void* ptr)
	{
		return WDL_bswap16_if_be(*(unsigned short*)ptr);
	}

	static unsigned int geti32(const void* ptr)
	{
		return WDL_bswap32_if_be(*(unsigned int*)ptr);
	}

	static WDL_UINT64 geti64(const void* ptr)
	{
		return WDL_bswap64_if_be(*(WDL_UINT64*)ptr);
	}

	unsigned char* GetTmpBuf(unsigned char* stackbuf)
	{
	#if defined(WAVEREADER_MAX_NCH) && WAVEREADER_MAX_NCH > 64
		return m_nch <= STACKBUF_MAX_NCH ? stackbuf : m_nch <= WAVEREADER_MAX_NCH ? (unsigned char*)m_heapbuf.ResizeOK(m_nch * 8, false) : NULL;
	#else
		return m_nch <= STACKBUF_MAX_NCH ? stackbuf : NULL;
	#endif
	}

	static const int STACKBUF_MAX_NCH =
#if !defined(WAVEREADER_MAX_NCH) || WAVEREADER_MAX_NCH > 64
	64;
#else
	WAVEREADER_MAX_NCH;
#endif
	static const int STACKBUF_MAX_SIZE = STACKBUF_MAX_NCH * 8;

#if defined(WAVEREADER_MAX_NCH) && WAVEREADER_MAX_NCH > 64
	WDL_HeapBuf m_heapbuf;
#endif

	WDL_String m_fn;
	FILE* m_fp;
	unsigned int m_size;
	int m_bps, m_nch, m_srate;
};


#endif // _WAVREAD_H_
