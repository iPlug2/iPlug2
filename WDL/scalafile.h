/*
  WDL - scalafile.h
  (c) Theo Niessink 2012
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


  This file provides a class for read/writing Scala scale (.scl) files.
  <http://www.huygens-fokker.org/scala/scl_format.html>


  Example #1:

	// Read scale file

	#include "scalafile.h"

	ScalaScaleFile scl;
	scl.Open("myscale.scl");
	scl.SkipDescr();

	int n = scl.ReadNum();
	for (int i = 0; i < n; ++i)
	{
		double pitch = scl.ReadPitch();
		if (pitch > 0.) printf("%f\n", pitch);
	}

  Example #2:

	// Write (create) scale file

	#include "scalafile.h"
	#include <math.h>

	ScalaScaleFile scl;
	char* filename = "newscale.scl";
	scl.Create(filename);
	scl.WriteFilename(filename);
	scl.WriteDescr("New scale");

	int n = 12;
	scl.WriteNum(n);
	for (int i = 1; i < n; ++i)
	{
		scl.WriteCents((double)(i * 100));
	}
	scl.WriteRatio(2, 1 , "octave");

*/


#ifndef _WDL_SCALA_FILE_H_
#define _WDL_SCALA_FILE_H_


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


class ScalaScaleFile
{
public:
	ScalaScaleFile(): m_fp(NULL) {}

	~ScalaScaleFile()
	{
		if (m_fp) fclose(m_fp);
	}

	bool Open(const char* filename)
	{
		m_fp = fopen(filename, "rb");
		m_buf = EOF;
		return (bool)m_fp;
	}

	int ReadDescr(char* buf, int size)
	{
		int c = SkipComments();
		if (c == EOF) return 0;

		int n = 0;
		while (c != '\n' && c != EOF)
		{
			if (++n < size) *buf++ = c;
			c = GetChar();
		}

		if (size) *buf = '\0';
		return ++n;
	}

	inline int SkipDescr()
	{
		return ReadDescr(NULL, 0);
	}

	int ReadNum()
	{
		const int err = -1;

		char buf[16];
		if (!ReadVal(buf, sizeof(buf))) return err;

		return atoi(buf);
	}

	double ReadPitch()
	{
		const double err = 0.;

		char buf[32];
		if (!ReadVal(buf, sizeof(buf))) return err;

		double pitch;
		if (strchr(buf, '.'))
		{
			double cents = atof(buf);
			pitch = pow(2., cents / 1200.);
		}
		else
		{
			int num = atoi(buf);
			if (num <= 0) return err;
			char* slash = strchr(buf, '/');
			if (slash)
			{
				int denom = atoi(slash + 1);
				if (denom <= 0) return err;
				pitch = (double)num / (double)denom;
			}
			else
			{
				pitch = (double)num;
			}
		}

		return pitch;
	}

	bool Create(const char* filename)
	{
		if (m_fp) fclose(m_fp);
		m_fp = fopen(filename, "w");
		return (bool)m_fp;
	}

	inline int WriteComment(const char* buf)
	{
		return m_fp ? WriteLine(buf) : -1;
	}

	int WriteFilename(const char* filename)
	{
		if (!m_fp) return -1;

		int ret = fputs("! ", m_fp);

		if (ret >= 0)
		{
			const char* p = filename + strlen(filename);
			while (--p >= filename && *p != '\\' && *p != '/');
			ret = fputs(++p, m_fp);
		}

		if (ret >= 0) ret = fputs("\n", m_fp);
		return ret;
	}

	inline int WriteDescr(const char* buf)
	{
		return m_fp ? WriteLine(buf) : -1;
	}

	int WriteNum(int num)
	{
		if (!m_fp) return -1;
		return fprintf(m_fp, "%d\n", num);
	}

	int WriteCents(double cents, const char* comment = NULL)
	{
		if (!m_fp) return -1;
		return WritePitchComment(fprintf(m_fp, "%0.5f", cents), comment);
	}

	int WriteRatio(int num, int denom, const char* comment = NULL)
	{
		if (!m_fp) return -1;
		return WritePitchComment(fprintf(m_fp, "%d/%d", num, denom), comment);
	}

	int WritePitch(double pitch, const char* comment = NULL)
	{
		if (!m_fp) return -1;

		double i;
		int ret;
		if (modf(pitch, &i) == 0.)
			ret = fprintf(m_fp, "%0.0f/1", i);
		else
			ret = fprintf(m_fp, "%0.5f", log(pitch) / log(2.) * 1200.);

		return WritePitchComment(ret, comment);
	}

	int Close()
	{
		if (m_fp)
		{
			if (fclose(m_fp)) return EOF;
			m_fp = NULL;
		}
		return 0;
	}

protected:
	int GetChar()
	{
		int c = m_buf;
		if (c != EOF)
		{
			m_buf = EOF;
			return c;
		}

		c = m_fp ? fgetc(m_fp) : EOF;
		if (c == EOF) return c;

		switch (c)
		{
			case '\r':
				m_buf = fgetc(m_fp);
				if (m_buf == '\n') m_buf = EOF;
				c = '\n';
				break;
			case '\n':
				m_buf = fgetc(m_fp);
				if (m_buf == '\r') m_buf = EOF;
				break;
		}

		return c;
	}

	int SkipComments()
	{
		int c = GetChar();
		while (c == '!')
		{
			do
			{
				c = GetChar();
				if (c == EOF) return c;
			} while (c != '\n');
			c = GetChar();
		}
		return c;
	}

	int ReadVal(char* buf, int size)
	{
		int c = SkipComments();
		if (c == EOF) return 0;

		int n = 0;
		bool ignore = false;
		while (c != '\n' && c != EOF)
		{
			if (!ignore)
			{
				if (c != ' ' && c != '\t')
				{
					if (++n < size) *buf++ = c;
				}
				else if (n)
				{
					ignore = true;
				}
			}
			c = GetChar();
		}
		if (n >= size) return 0;

		*buf = '\0';
		return ++n;
	}

	int WriteLine(const char* buf)
	{
		// if (!m_fp) return -1;

		if (buf)
		{
			int ret = fputs(buf, m_fp);
			if (ret < 0) return ret;
		}

		return fputs("\n", m_fp);
	}

	int WritePitchComment(int ret, const char* comment)
	{
		// if (!m_fp) return -1;

		if (comment && ret >= 0)
		{
			ret = fputs(" ", m_fp);
			if (ret >= 0) ret = fputs(comment, m_fp);
		}

		if (ret >= 0) ret = fputs("\n", m_fp);
		return ret;
	}

	FILE* m_fp;
	int m_buf;
};


#endif // _WDL_SCALA_FILE_H_
