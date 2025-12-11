/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * file/media handle interface, plus a function to register numerous jsfx-related
 * eel functions, which miscfunc.cpp implements
 */

#ifndef _MISCFUNC_H_
#define _MISCFUNC_H_

#include <stdio.h>

// classic Jesusonic had basic PCM and ADPCM wav support, superceded by REAPER's PCM_source interfaces
// #define JSFX_SUPPORT_WAV_DECODING


#include "../WDL/heapbuf.h"
#include "reaper_plugin.h"

void eel_js_register();

class fileHandleRec
{
public:
  fileHandleRec();
  ~fileHandleRec();
  void Close();
  void OpenMem(WDL_HeapBuf *hb, int iswrite);
  int Open(const char *fn, int iswrite); // returns 1 on success
  void Rewind();

  char *txt_eval_op(int *op, char *token);
  int txt_eval_expression(EEL_F *v, char *line, char **endptr=0);
  char *txt_eval_tok(EEL_F *v, char *token);
  EEL_F *txt_get_value(char *name, int create, char **endptr=0);

  WDL_INT64 m_itemsleft, m_start_offs, m_start_len;

  enum file_mode_type {
    FILE_MODE_NORMAL = 0,
    FILE_MODE_WRITE,
    FILE_MODE_TEXT
  };
  file_mode_type m_mode;

  unsigned int m_riff_bps,m_riff_nch; // bps is 0 if in .raw FP mode
#ifdef JSFX_SUPPORT_WAV_DECODING
  struct adpcm_ctx
  {
    int cf1[2],cf2[2],deltas[2],spl1[2],spl2[2];
    int needbyte;
    unsigned char lastbyte;
    int state,statetmp;
  };

  int m_riff_adpcm,m_riff_blockalign;
  adpcm_ctx m_adpcmctx;
#endif
  float m_srate; // only in PCM_source mode, RIFF or caller-overridden

  FILE *m_fp;
  WDL_HeapBuf *m_hb;
  int m_hb_wpos;

  WDL_PtrList<char> m_txtdefs;

  // code for reading/buffering from PCM_source (may or may not be possible depending on jsfx.dll or reajs)
  PCM_source *m_pcm_src;
  WDL_TypedBuf<ReaSample> m_pcm_src_rdbuf;
  int m_pcm_src_rdbuf_rdpos;

  WDL_INT64 m_pcm_src_absolute_rdpos; // read position in source in samples

  bool readPcmSrcFloat(EEL_F *f);

};

#endif
