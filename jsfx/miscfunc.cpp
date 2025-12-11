/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * implement various jsfx apis including file reading
 */

#include "../WDL/eel2/ns-eel.h"
#include "../WDL/eel2/ns-eel-int.h"
#include "../WDL/win32_utf8.h"
#include "../WDL/mutex.h"
#include "sfxui.h"
#include "miscfunc.h"
#include <math.h>
#include <stdio.h>

#include "../WDL/win32_utf8.h"

#ifdef JSFX_SUPPORT_WAV_DECODING
  #define MSADPCM_PREAMBLELEN 7 // this is 7 bytes per channel, but the preamble has two samples too
  #include "../WDL/pcmfmtcvt.h"
  #define CHAR_TO_EEL_F(s) (((signed char)(s))/128.0)
#endif

fileHandleRec::fileHandleRec()
{
  m_hb_wpos=0;
  m_hb=0;
  m_mode=FILE_MODE_NORMAL;
  m_itemsleft=m_riff_bps=m_riff_nch=0;
  m_srate=0.0f;
#ifdef JSFX_SUPPORT_WAV_DECODING
  m_riff_adpcm=m_riff_blockalign=0;
  memset(&m_adpcmctx,0,sizeof(m_adpcmctx));
#endif
  m_start_offs=m_start_len=0; // for rewind, later
  m_fp=0;

  m_pcm_src = 0;
  m_pcm_src_rdbuf_rdpos = 0;
  m_pcm_src_absolute_rdpos = 0;
}

fileHandleRec::~fileHandleRec()
{
  if (m_fp) fclose(m_fp);
  delete m_pcm_src;
}


char *fileHandleRec::txt_eval_tok(EEL_F *v, char *token)
{
  char *p=token;
  if (token[0] == '(')
  {
    if (txt_eval_expression(v,token+1,&p)) return p;
  }
  if ((token[0] == '.' || token[0]=='-' || (token[0] >= '0' && token[0] <= '9')))
  {
    *v=(EEL_F)strtod(token,&p);
    if (p != token) return p;
  }
  if ((token[0] == 'b' || token[0] == 'B'))
  {
    *v=(EEL_F)strtoul(token+1,&p,2);
    if (p != token+1) return p;
  }
  if ((token[0] == 'x' || token[0] == 'X'))
  {
    *v=(EEL_F)strtoul(token+1,&p,16);
    if (p != token+1) return p;
  }
  if ((token[0] == 'o' || token[0] == 'O'))
  {
    *v=(EEL_F)strtoul(token+1,&p,8);
    if (p != token+1) return p;
  }

  EEL_F *f=txt_get_value(token,0,&p);
  if (f && p != token)
  {
    *v=*f;
  }

  return p;
}


char *fileHandleRec::txt_eval_op(int *op, char *token)
{
  char *p=token;
  while (*p && !isalnum_safe(*p) && *p != '_')
  {
    if (*p == '+') { *op=1; return p+1; }
    if (*p == '-') { *op=2; return p+1; }
    if (*p == '*') { *op=3; return p+1; }
    if (*p == '/') { *op=4; return p+1; }
    if (*p == '|') { *op=5; return p+1; }
    if (*p == '&') { *op=6; return p+1; }
    if (*p == '^') { *op=7; return p+1; }
    if (*p == ')') { *op=-1; return p+1; }
    p++;
  }
  return token;
}

int fileHandleRec::txt_eval_expression(EEL_F *v, char *line, char **endptr) // return 1 on successful eval
{
  int cnt=0;
  int op=0;
  char *p=line;
  while (*p)
  {
    EEL_F cv=0.0;
    char *lp=p;
    p=txt_eval_tok(&cv,lp);
    if (p == lp) break;

    cnt++;
    if (!op) *v=cv;
    else if (op == 1) *v+=cv;
    else if (op == 2) *v-=cv;
    else if (op == 3) *v *= cv;
    else if (op == 4) *v /= cv;
    else if (op == 5) *v = (EEL_F) (((int)*v)|((int)cv));
    else if (op == 6) *v = (EEL_F) (((int)*v)&((int)cv));
    else if (op == 7) *v = (EEL_F) (((int)*v)^((int)cv));

    lp=p;
    p=txt_eval_op(&op,lp);
    if (p == lp) break;

    if (op == -1) break;

  }

  if (endptr) *endptr=p;

  return cnt;
}

EEL_F *fileHandleRec::txt_get_value(char *name, int create, char **endptr)
{
  if (endptr) *endptr=name;
  int x;
  while (isspace_safe(*name)) name++;
  char *ptr=name;
  while (isalnum_safe(*ptr) || *ptr == '_') ptr++;

  if (!*name) return 0;

  if (endptr) *endptr=ptr;

  for (x = 0; x < m_txtdefs.GetSize(); x ++)
  {
    if (!strnicmp(name,m_txtdefs.Get(x)+sizeof(EEL_F),ptr-name))
      return (EEL_F *)m_txtdefs.Get(x);
  }
  if (!create) return 0;
  char *p=(char *)malloc(ptr-name+1+sizeof(EEL_F));
  if (p)
  {
    strncpy(p+sizeof(EEL_F),name,ptr-name);
    p[sizeof(EEL_F)+ptr-name]=0;
    m_txtdefs.Add(p);
  }
  return (EEL_F *)p;
}



void fileHandleRec::Close()
{
  if (m_hb && m_hb_wpos < m_hb->GetSize()) m_hb->Resize(m_hb_wpos);
  m_hb=0;
  m_hb_wpos=0;

  if (m_fp) fclose(m_fp);
  m_fp=0;
  delete m_pcm_src;
  m_pcm_src = 0;

  m_mode=FILE_MODE_NORMAL;
  m_riff_nch=m_riff_bps=0;
  m_srate=0.0f;
  m_txtdefs.Empty(true,free);
}

void fileHandleRec::Rewind()
{
  m_hb_wpos=0;
  if (m_hb) m_itemsleft=m_start_len;
  if (m_pcm_src)
  {
    m_itemsleft = m_start_len;
    m_pcm_src_absolute_rdpos = 0;
    m_pcm_src_rdbuf_rdpos = 0;
    m_pcm_src_rdbuf.Resize(0, false);
  }
  if (m_fp)
  {
    fseek(m_fp,m_start_offs,SEEK_SET);
    m_itemsleft=m_start_len;
#ifdef JSFX_SUPPORT_WAV_DECODING
    memset(&m_adpcmctx,0,sizeof(m_adpcmctx));
#endif
  }
  m_txtdefs.Empty(true,free);
}

bool fileHandleRec::readPcmSrcFloat(EEL_F *f)
{
  if (!m_pcm_src) return true;

  if (m_pcm_src_rdbuf_rdpos >= m_pcm_src_rdbuf.GetSize())
  {
    m_pcm_src_rdbuf_rdpos = 0;

    const double sr = m_srate;
    const int bsize = 1024;
    const int nch = m_pcm_src->GetNumChannels();
    m_pcm_src_rdbuf.Resize(nch*bsize);
    if (m_pcm_src_rdbuf.GetSize()>=nch*bsize)
    {
      PCM_source_transfer_t block = { m_pcm_src_absolute_rdpos / sr, sr, nch, bsize, m_pcm_src_rdbuf.Get(), };
      m_pcm_src->GetSamples(&block);
      if (block.samples_out < bsize) m_pcm_src_rdbuf.Resize(block.samples_out * nch, false);
      m_pcm_src_absolute_rdpos += block.samples_out;
    }
    else m_pcm_src_rdbuf.Resize(0);

  }
  if (m_pcm_src_rdbuf_rdpos >= m_pcm_src_rdbuf.GetSize()) return true;

  *f = m_pcm_src_rdbuf.Get()[m_pcm_src_rdbuf_rdpos++];
  m_itemsleft--;

  return false;
}

void fileHandleRec::OpenMem(WDL_HeapBuf *hb, int iswrite)
{
  Close();

  m_hb_wpos=0;
  m_hb=hb;
  m_start_offs=0;
  if (iswrite > 0)
  {
    m_mode = FILE_MODE_WRITE;
    m_itemsleft=0;
  }
  else
  {
    m_mode = FILE_MODE_NORMAL;
    m_itemsleft=m_hb->GetSize()/sizeof(float);
  }
  m_start_len=m_itemsleft;
}

int fileHandleRec::Open(const char *fn, int iswrite)
{
  m_hb_wpos=0;
  m_hb=0;
  m_mode=FILE_MODE_NORMAL;
  m_riff_nch=m_riff_bps=0;
  m_srate=0.0f;
  if (m_fp) fclose(m_fp);
  m_fp = 0;
  delete m_pcm_src;
  m_pcm_src = 0;

  const size_t l=strlen(fn);
  const bool isTextExt = l > 4 && !stricmp(fn + l - 4, ".txt");
  const bool isRawExt = l > 4 && !stricmp(fn + l - 4, ".raw");

  if (!isTextExt && !isRawExt && !iswrite)
  {

    m_pcm_src = _PCM_Source_CreateFromFile ? _PCM_Source_CreateFromFile(fn) : NULL;
    if (m_pcm_src && m_pcm_src->GetNumChannels()>0 && m_pcm_src->GetSampleRate() > 1.0)
    {
      m_pcm_src_rdbuf_rdpos = 0;
      m_pcm_src_rdbuf.Resize(0);

      m_pcm_src_absolute_rdpos = 0;

      const double sr = m_pcm_src->GetSampleRate();
      m_srate = (float)sr;
      m_riff_bps = m_pcm_src->GetBitsPerSample();
      if (m_riff_bps<8) m_riff_bps = 32;
      m_riff_nch = m_pcm_src->GetNumChannels();

      m_itemsleft = m_start_len = ((WDL_INT64)(sr * m_pcm_src->GetLength() + 0.5)) * m_riff_nch;
    }
    else
    {
      delete m_pcm_src;
      m_pcm_src=NULL;
    }

    if (m_pcm_src) return 1;
  }

  m_fp=fopenUTF8(fn,iswrite>0?"wb":"rb");

  if (!m_fp) return 0;
  if (iswrite>0)
  {
    m_mode=FILE_MODE_WRITE;
    return 1;
  }

  fseek(m_fp,0,SEEK_END);
  m_itemsleft=ftell(m_fp)/sizeof(float);
  fseek(m_fp,0,SEEK_SET);

  if (iswrite<0)
  {
    m_start_offs=0;
    m_start_len=m_itemsleft;
    return 1;
  }


  if (isTextExt)
  {
    m_mode=FILE_MODE_TEXT;
  }
#ifdef JSFX_SUPPORT_WAV_DECODING
  else if (l > 4 && !stricmp(fn+l-4,".wav"))
  {
    // parse RIFF
    unsigned char buf[8];
    if (fread(buf,1,8,m_fp)==8 && !memcmp(buf,"RIFF",4))
    {
      unsigned int fmt_bps=0,fmt_nch=0,fmt_srate=0;
      if (fread(buf,1,4,m_fp)==4 && !memcmp(buf,"WAVE",4)) for (;;)
      {
        if (fread(buf,1,8,m_fp)!=8) goto bailout;
        unsigned int fmt_len = buf[4] | (buf[5]<<8) | (buf[6]<<16) | (buf[7]<<24);
        if (!memcmp(buf,"fmt ",4))
        {
          if (fmt_len < 16)
          {
            goto bailout;
          }
          unsigned char ebuf[16];
          if (fread(ebuf,1,16,m_fp) != 16)
          {
            goto bailout;
          }
          m_riff_adpcm=0;
          if (ebuf[0] == 2 && ebuf[1] == 0)
          {
            m_riff_adpcm=1;
          }
          else if (ebuf[0] != 1 || ebuf[1] != 0)
          {
            goto bailout; // not PCM
          }
          fmt_nch=ebuf[2];
          if (ebuf[3] != 0 || (fmt_nch != 1 && fmt_nch != 2))
          {
            goto bailout;
          }
          fmt_srate=ebuf[4]|(ebuf[5]<<8)|(ebuf[6]<<16)|(ebuf[7]<<24);
          m_riff_blockalign = ebuf[12]|(ebuf[13]<<8);
          fmt_bps = ebuf[14]|(ebuf[15]<<8);

          if (m_riff_adpcm)
          {
            if (fmt_bps != 4) goto bailout;
          }
          else
          {
            if (fmt_bps != 8 && fmt_bps != 16 && fmt_bps != 24) goto bailout;
          }

          fseek(m_fp,((fmt_len+1)&~1)-16,SEEK_CUR);
          break;
        }
        else fseek(m_fp,(fmt_len+1)&~1,SEEK_CUR);
      }
      // we have our fmt, now let's look for a data chunk ( we should start over but meh )
      for (;;)
      {
        if (fread(buf,1,8,m_fp)!=8) goto bailout;
        unsigned int data_len = buf[4] | (buf[5]<<8) | (buf[6]<<16) | (buf[7]<<24);
        if (!memcmp(buf,"data",4))
        {
          if (m_riff_adpcm)
          {
            m_itemsleft=data_len;
            if (m_riff_blockalign)
            {
              m_itemsleft -=
                ((data_len+m_riff_blockalign-1)/m_riff_blockalign)*(MSADPCM_PREAMBLELEN-1)*fmt_nch;
            }
            m_itemsleft*=2;
          }
          else
          {
            m_itemsleft=data_len/(fmt_bps>=8?fmt_bps/8:1);
          }
          memset(&m_adpcmctx,0,sizeof(m_adpcmctx));

          m_riff_nch=fmt_nch;
          m_riff_bps=fmt_bps;
          m_srate=(float)fmt_srate;
          m_start_offs=ftell(m_fp);
          m_start_len=m_itemsleft;
          return 1;
        }
        fseek(m_fp,((data_len+1)&~1),SEEK_CUR);
      }
    }
bailout:
    fseek(m_fp,0,SEEK_SET);
  }
#endif
  m_start_offs=ftell(m_fp);

  m_start_len=m_itemsleft;
  return 1;

}


#ifdef JSFX_SUPPORT_WAV_DECODING

static int adpcm_getwordsigned(int *s, fileHandleRec *epctx)
{
  unsigned char c[2];
  if (fread(c,1,2,epctx->m_fp)!=2) return 1;
  *s = c[0] + (c[1]<<8);
  if (*s & 0x8000) *s -= 0x10000;
  return 0;
}

static int decodeADPCM(EEL_F *outptr, fileHandleRec *epctx)
{
  fileHandleRec::adpcm_ctx *adctx = &epctx->m_adpcmctx;

  static int cotab[16] = { 256, 512, 0, 192, 240, 460,  392, 0,
                           0,  -256, 0, 64,  0,  -208, -232, 0 };
  static int adtab[16] = { 230, 230, 230, 230, 307, 409, 512, 614,
                           768, 614, 512, 409, 307, 230, 230, 230 };


  if (adctx->state<0) return 0;
  if (!epctx->m_itemsleft) return 0;

  if (!adctx->state) // read preamble
  {
    unsigned char c=fgetc(epctx->m_fp);
    if (c > 6 || feof(epctx->m_fp)) { adctx->state=-1; return 0; }
    adctx->cf1[0]=cotab[c];
    adctx->cf2[0]=cotab[8+c];
    if (epctx->m_riff_nch>1)
    {
      c=fgetc(epctx->m_fp);
      if (c > 6 || feof(epctx->m_fp)) { adctx->state=-1; return 0; }
      adctx->cf1[1]=cotab[c];
      adctx->cf2[1]=cotab[8+c];
    }

    if (adpcm_getwordsigned(adctx->deltas,epctx) || ( epctx->m_riff_nch>1 && adpcm_getwordsigned(adctx->deltas+1,epctx))) { adctx->state=-1; return 0; }
    if (adpcm_getwordsigned(adctx->spl1,epctx) || ( epctx->m_riff_nch>1 && adpcm_getwordsigned(adctx->spl1+1,epctx))) { adctx->state=-1; return 0; }
    if (adpcm_getwordsigned(adctx->spl2,epctx) || ( epctx->m_riff_nch>1 && adpcm_getwordsigned(adctx->spl2+1,epctx))) { adctx->state=-1; return 0; }

    // ok at this point we have nch*2 samples queued up already!
    // so we'll set adctx->state to ns+nch*2

    // samples in block
    adctx->statetmp = (epctx->m_riff_blockalign-MSADPCM_PREAMBLELEN*epctx->m_riff_nch)*2;

    // samples plus our initial samples
    adctx->state=adctx->statetmp+epctx->m_riff_nch*2;

    adctx->needbyte=1;

    INT16_TO_double(*outptr,adctx->spl2[0]);
  }
  else if (adctx->state>adctx->statetmp)
  {
    int diff=adctx->state-adctx->statetmp;
    if (epctx->m_riff_nch == 1 || diff == 2) INT16_TO_double(*outptr,adctx->spl1[0]);
    else if (diff == 3) INT16_TO_double(*outptr,adctx->spl2[1]);
    else INT16_TO_double(*outptr,adctx->spl1[1]);
  }
  else // decode a sample
  {
    int ch=adctx->state&(epctx->m_riff_nch-1);
    int nib;
    if (adctx->needbyte)
    {
      adctx->lastbyte=fgetc(epctx->m_fp);
      if (feof(epctx->m_fp)) { adctx->state=-1; return 0; }

      nib=adctx->lastbyte>>4;
      adctx->needbyte=0;
    }
    else
    {
      nib=adctx->lastbyte&0xf;
      adctx->needbyte=1;
    }


    int sn=nib;
    if (sn & 8) sn -= 16;

    int pred = ( ((adctx->spl1[ch] * adctx->cf1[ch]) + (adctx->spl2[ch] * adctx->cf2[ch])) / 256) + (sn * adctx->deltas[ch]);

    adctx->spl2[ch] = adctx->spl1[ch];

    if (pred < -32768) pred=-32768;
    else if (pred > 32767) pred=32767;

    adctx->spl1[ch] = pred;
    INT16_TO_double(*outptr,pred);

    int i= (adtab[nib] * adctx->deltas[ch]) / 256;
    if (i <= 16) adctx->deltas[ch]=16;
    else adctx->deltas[ch] = i;
  }

  adctx->state--;
  epctx->m_itemsleft--;

  return 1;
}

#endif // JSFX_SUPPORT_WAV_DECODING

static bool readTextFileVar(fileHandleRec *tfh, EEL_F *var) // returns true on eof
{
  for (;;)
  {
    char buf[1024];
    int pos=0;
    int ign=0;

    while (pos < (int)sizeof(buf)-1)
    {
      int c=fgetc(tfh->m_fp);
      if (c == EOF)  break;
      if (c == '\r' || c== '\n')
      {
        if (!pos)
        {
          ign=0;
          continue; // skip over any initial newlines
        }
        break; // done
      }
      if (!pos && (c == ' ' || c == '\t')) continue; // skip over any initial spaces/tabs

      if (c == ';' || c == '#') ign=1;

      if (!ign)
      {
        if (c == ',')
        {
          if (pos) break; // , delimits new string if in the middle, otherwise skip it
        }
        else buf[pos++]=c;
      }
    }

    buf[pos]=0;
    if (!buf[0]) break;
    char *p=buf;

    char *eqptr=0;
    {
      char *pp=p;
      while (*pp)
      {
        char c=*pp;
        if (c == '=' && !eqptr) eqptr=pp;
        pp++;
      }
    }
    if (!*p) continue;

    EEL_F v=0.0;
    if (eqptr)
    {
      if (tfh->txt_eval_expression(&v,eqptr+1))
      {
        *eqptr=0;
        EEL_F *a=tfh->txt_get_value(p,1);
        if (a) *a=v;
      }
    }
    else
    {
      if (tfh->txt_eval_expression(&v,p))
      {
        *var=v;
        return false;
      }
    }
  }
  return true;
}

static int serializeInternal(SX_Instance *epctx, int a, EEL_F *var, WDL_FastString *strOut)
{
  if (a < 0 || a > MAX_EFFECT_FILES) return 0;

  fileHandleRec *tfh=epctx->m_filehandles+a;

  if (tfh->m_hb)
  {
    if (tfh->m_mode==fileHandleRec::FILE_MODE_WRITE) // writing to memory
    {
      if (strOut)
      {
        const int l = strOut->GetLength();
        const int pad_len = (l+3)&~3;
        if (tfh->m_hb_wpos+pad_len+4 >= tfh->m_hb->GetSize()) tfh->m_hb->Resize(tfh->m_hb_wpos+pad_len+65536);
        char *p=(char*)tfh->m_hb->Get()+tfh->m_hb_wpos;
        memcpy(p,&l,4);
        memcpy(p+4,strOut->Get(),l);
        if (l < pad_len) memset(p+4+l, 0, pad_len-l);
        REAPER_MAKELEINTMEM(p);
        tfh->m_hb_wpos += 4+pad_len;
        return l;
      }
      else
      {
        float f=(float) *var;
        REAPER_MAKELEINTMEM(&f);
        if (tfh->m_hb_wpos+4 >= tfh->m_hb->GetSize()) tfh->m_hb->Resize(tfh->m_hb_wpos+65536);
        memcpy((char*)tfh->m_hb->Get()+tfh->m_hb_wpos,&f,4);
        tfh->m_hb_wpos+=4;
        return 1;
      }
    }

    // reading from memory
    if (tfh->m_hb_wpos+4 <= tfh->m_hb->GetSize())
    {
      if (strOut)
      {
        const char *rdptr = ((const char*)tfh->m_hb->Get()+tfh->m_hb_wpos);
        int l = *(const int *)rdptr;
        REAPER_MAKELEINTMEM(&l);

        const int bytes_left = tfh->m_hb->GetSize() - tfh->m_hb_wpos - 4;
        if (l< 0 || l > bytes_left)
        {
          // error! force EOF
          tfh->m_itemsleft=0;
          tfh->m_hb_wpos = tfh->m_hb->GetSize() ;

          strOut->Set("");
          return 0;
        }
        strOut->SetRaw(rdptr+4,l);

        const int pad_l = (l+3)&~3;
        const int items_used = 1 + pad_l/4;
        tfh->m_hb_wpos += pad_l + 4;

        if (items_used >= tfh->m_itemsleft) tfh->m_itemsleft=0;
        else tfh->m_itemsleft -= items_used;

        return l;
      }
      else
      {
        float f=*(const float *) ((const char*)tfh->m_hb->Get()+tfh->m_hb_wpos);
        REAPER_MAKELEINTMEM(&f);

        *var=(EEL_F)f;

        tfh->m_hb_wpos+=4;
        tfh->m_itemsleft--;
        return 1;
      }
    }
    return 0; // insufficient data in heapbuf
  }

  if (epctx->m_filehandles[a].m_pcm_src)
  {
    if (strOut)
    {
      EEL_F v=0.0;
      tfh->readPcmSrcFloat(&v);
      strOut->SetFormatted(64,"%.10f",v);
    }
    else
    {
      tfh->readPcmSrcFloat(var);
    }
    return 1;
  }

  if (!epctx->m_filehandles[a].m_fp) return 0;

  // file mode!

  if (tfh->m_mode==fileHandleRec::FILE_MODE_WRITE)
  {
    //write
    if (strOut)
    {
      if (strOut->GetLength() > 0) return fwrite(strOut->Get(),1,strOut->GetLength(),tfh->m_fp);
      return 0;
    }
    else
    {
      float f=(float) *var;
      REAPER_MAKELEINTMEM(&f);
      return fwrite(&f,sizeof(float),1,tfh->m_fp);
    }
  }

  if (tfh->m_mode==fileHandleRec::FILE_MODE_TEXT || (!tfh->m_riff_bps && strOut))
  {
    if (strOut)
    {
      char buf[8192];
      buf[0]=0;
      if (fgets(buf,sizeof(buf),tfh->m_fp))
        strOut->Set(buf);
      return strlen(buf);
    }
    readTextFileVar(tfh,var);
    return 1;
  }
  if (!tfh->m_riff_bps)
  {
    // strOut is NULL, var is non-NULL
    float f;
    if (fread(&f,1,sizeof(float),tfh->m_fp) == sizeof(float))
    {
      REAPER_MAKELEINTMEM(&f);
      if (var) *var = (EEL_F) f;
      tfh->m_itemsleft--;
    }
  }
#ifdef JSFX_SUPPORT_WAV_DECODING
  else if (tfh->m_riff_bps==4 && tfh->m_riff_adpcm)
  {
    EEL_F v=0.0;
    decodeADPCM(&v,tfh);
    if (var) *var=v;
    else strOut->SetFormatted(64,"%.10f",v);
  }
  else if (tfh->m_riff_bps==8)
  {
    unsigned char s;
    if (fread(&s,1,sizeof(char),tfh->m_fp) == sizeof(char))
    {
      if (var) *var = CHAR_TO_EEL_F(s);
      else strOut->SetFormatted(64,"%.10f",CHAR_TO_EEL_F(s));
      tfh->m_itemsleft--;
    }
  }
  else if (tfh->m_riff_bps==16)
  {
    short s;
    if (fread(&s,1,sizeof(short),tfh->m_fp) == sizeof(short))
    {
#ifdef __ppc__
      {
        char *p = (char *)&s,tmp;
        tmp=p[0]; p[0]=p[1]; p[1]=tmp;
      }
#endif
      EEL_F v;
      INT16_TO_double(v,s);
      if (var) *var=v;
      else strOut->SetFormatted(64,"%.10f",v);
      tfh->m_itemsleft--;
    }
  }
  else if (tfh->m_riff_bps==24)
  {
    unsigned char i[3];
    if (fread(&i,1,3,tfh->m_fp) == 3)
    {
      EEL_F v;
      i24_to_double(i,&v);
      if (var) *var=v;
      else strOut->SetFormatted(64,"%.10f",v);
      tfh->m_itemsleft--;
    }
  }
#endif // JSFX_SUPPORT_WAV_DECODING
  return 0;
}

EEL_F * NSEEL_CGEN_CALL _serialize_var(SX_Instance *epctx, EEL_F *wf, EEL_F *var)
{
  WDL_MutexLock __lock(&epctx->m_string_sermutex);
  serializeInternal(epctx,(int) *wf, var, NULL);
  return var;
}

EEL_F NSEEL_CGEN_CALL _serialize_string(SX_Instance *epctx, EEL_F *wf, EEL_F *var)
{
  WDL_MutexLock __lock(&epctx->m_string_sermutex);

  WDL_FastString *fs=NULL;
  epctx->GetStringForIndex(*var,&fs,true);
  if (!fs) return 0.0;

  return (EEL_F) serializeInternal(epctx,(int) *wf, NULL, fs);
}

EEL_F NSEEL_CGEN_CALL _strcpy_fromslider(SX_Instance *epctx, EEL_F *dest, EEL_F *src)
{
  WDL_MutexLock __lock(&epctx->m_string_sermutex);

  WDL_FastString *fs=NULL;
  epctx->GetStringForIndex(*dest,&fs,true);
  if (fs)
  {
    int x = epctx->find_slider_by_value_ptr(src);
    if (x < 0) x = ((int) *src) - 1;

    fs->Set("");

    effectSlider *slid = epctx->m_sliders.Get(x);
    if (slid)
    {
      if (slid->is_file_slider)
      {
        if (slid->currentFile)
        {
          fs->Append(slid->FileDirectory.Get());
          fs->Append(slid->currentFile);
        }
      }
      else if (slid->sliderValueXlateList.GetSize() && slid->slider)
      {
        int a=(int) (slid->slider[0]+0.5);
        const char *p= slid->sliderValueXlateList.Get(a);
        if (p) fs->Append(p);
      }
    }
  }
  return *dest;
}

EEL_F NSEEL_CGEN_CALL _serialize_mem(SX_Instance *epctx, EEL_F *wf, EEL_F *_offs, EEL_F *_len)
{
  EEL_F **blocks = ((compileContext*)epctx->m_vm)->ram_state->blocks;

  const int file_idx=(int)(*wf);
  if (file_idx < 0 || file_idx > MAX_EFFECT_FILES) return 0.0;
  fileHandleRec *tfh=epctx->m_filehandles+file_idx;

  WDL_MutexLock __lock(&epctx->m_string_sermutex);

  if (!tfh->m_fp && !tfh->m_hb && !tfh->m_pcm_src) return 0.0;

  int outcnt=0;
  int ilen = (int)(*_len+0.0001);
  int ioffs = (int)(*_offs+0.0001);

  if (ioffs<0)
  {
    ilen += ioffs;
    ioffs=0;
  }

  if (ioffs >= NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK) return 0.0;
  if (ioffs+ilen > NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK) ilen = NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK - ioffs;

  while (ilen > 0)
  {
    EEL_F *ptr=__NSEEL_RAMAlloc(blocks,ioffs);
    if (!ptr || ptr==&nseel_ramalloc_onfail) break;

    int lcnt=NSEEL_RAM_ITEMSPERBLOCK-(ioffs&(NSEEL_RAM_ITEMSPERBLOCK-1));
    if (lcnt > ilen) lcnt=ilen;

    ilen -= lcnt;
    ioffs += lcnt;

    while (lcnt--)
    {
      if (tfh->m_hb)
      {
        if (tfh->m_mode==fileHandleRec::FILE_MODE_WRITE)
        {
          float f=(float) *ptr++;
          outcnt++;
          REAPER_MAKELEINTMEM(&f);
          if (tfh->m_hb_wpos+4 >= tfh->m_hb->GetSize()) tfh->m_hb->Resize(tfh->m_hb_wpos+65536);
          memcpy((char*)tfh->m_hb->Get()+tfh->m_hb_wpos,&f,4);
          tfh->m_hb_wpos+=4;
        }
        else
        {
          if (tfh->m_hb_wpos+4 <= tfh->m_hb->GetSize())
          {
            float f=*(float *) ((char*)tfh->m_hb->Get()+tfh->m_hb_wpos);
            REAPER_MAKELEINTMEM(&f);

            *ptr++=(EEL_F)f;
            outcnt++;
            tfh->m_hb_wpos+=4;
            tfh->m_itemsleft--;
          }
          else break;
        }
      }
      else if (tfh->m_pcm_src)
      {
        if (tfh->readPcmSrcFloat(ptr)) break;
        ptr++;
        outcnt++;
      }
      else if (tfh->m_fp)
      {
        if (tfh->m_mode==fileHandleRec::FILE_MODE_WRITE)
        {
          float a = (float) *ptr++;
          REAPER_MAKELEINTMEM(&a);
          if (fwrite(&a,1,sizeof(float),tfh->m_fp) == sizeof(float)) outcnt++;
          else break;
        }
        else
        {
          if (tfh->m_mode==fileHandleRec::FILE_MODE_TEXT)
          {
            if (readTextFileVar(tfh,ptr)) break;
            ptr++;
            outcnt++;
          }
          else if (!tfh->m_riff_bps)
          {
            float f;
            if (fread(&f,1,sizeof(float),tfh->m_fp) == sizeof(float))
            {
              REAPER_MAKELEINTMEM(&f);
              *ptr++ = (EEL_F) f;
              tfh->m_itemsleft--;
              outcnt++;
            }
            else break;
          }
#ifdef JSFX_SUPPORT_WAV_DECODING
          else if (tfh->m_riff_bps==4 && tfh->m_riff_adpcm)
          {
            if (decodeADPCM(ptr,tfh))
            {
              ptr++;
              outcnt++;
            }
            else break;
          }
          else if (tfh->m_riff_bps==8)
          {
            unsigned char s;
            if (fread(&s,1,sizeof(char),tfh->m_fp) == sizeof(char))
            {
              *ptr++ = CHAR_TO_EEL_F(s);
              tfh->m_itemsleft--;
              outcnt++;
            }
            else break;
          }
          else if (tfh->m_riff_bps==16)
          {
            short s;
            if (fread(&s,1,sizeof(short),tfh->m_fp) == sizeof(short))
            {
              EEL_F d;
              #ifdef __ppc__
              {
              char *p = (char *)&s,tmp;
              tmp=p[0]; p[0]=p[1]; p[1]=tmp;
              }
              #endif
              INT16_TO_double(d,s);
              *ptr++ = d;
              tfh->m_itemsleft--;
              outcnt++;
            }
            else break;
          }
          else if (tfh->m_riff_bps==24)
          {
            unsigned char i[3];
            if (fread(&i,1,3,tfh->m_fp) == 3)
            {
              i24_to_double(i,ptr++);
              tfh->m_itemsleft--;
              outcnt++;
            }
          }
#endif // JSFX_SUPPORT_WAV_DECODING
        } // reading
      } // m_fp
    } // lcnt
  } // ilen
  return (EEL_F) outcnt;
}


EEL_F NSEEL_CGEN_CALL _serialize_text(SX_Instance *epctx, EEL_F *wf)
{
  int a=(int)(*wf);
  if (a < 1 || a > MAX_EFFECT_FILES) return 0.0;

  if (epctx->m_filehandles[a].m_fp && epctx->m_filehandles[a].m_mode==fileHandleRec::FILE_MODE_TEXT) return 1.0;
  return 0.0;
}



EEL_F * NSEEL_CGEN_CALL _serialize_riff(SX_Instance *epctx, EEL_F *wf, EEL_F *nch, EEL_F *srate)
{
  int a=(int)(*wf);
  if (a < 1 || a > MAX_EFFECT_FILES || !epctx->m_filehandles[a].m_riff_bps)
  {
    *nch=*srate=0;
  }
  else
  {
    fileHandleRec *h = epctx->m_filehandles + a;
    if (*nch == 1920037746.0 /* -'rqsr' */ && *srate > 0.0 && h->m_pcm_src)
    {
      h->m_srate = (float)*srate;
      h->m_itemsleft = h->m_start_len = ((WDL_INT64)(*srate * h->m_pcm_src->GetLength() + 0.5)) * h->m_riff_nch;
    }
    else
    {
      *srate = h->m_srate;
    }

    *nch=h->m_riff_nch;
  }
  return nch;
}

EEL_F * NSEEL_CGEN_CALL _spl_getter(SX_Instance *epctx, EEL_F *ws)
{
  static EEL_F tmp;
  int a=(int)(*ws+0.0001);
  if (!epctx || a<0 || a >= MAX_NCH) return &tmp;
  return &epctx->m_spls[a];
}


EEL_F * NSEEL_CGEN_CALL _slider_getter(SX_Instance *epctx, EEL_F *ws)
{
  static EEL_F tmp;
  effectSlider *slid = epctx ? epctx->m_sliders.Get((int)(*ws+0.0001) - 1) : NULL;
  return slid ? slid->slider : &tmp;
}

EEL_F NSEEL_CGEN_CALL _slider_next_chg(SX_Instance* epctx, EEL_F* ws, EEL_F* nextval)
{
  if (epctx && ws)
  {
    effectSlider *slid = epctx->m_sliders.Get((int)(*ws+0.0001) - 1);
    if (slid)
    {
      int splpos=-1;
      double val=0.0;
      int curpos=slid->parmchg_q_curpos;
      if (curpos < slid->parmchg_q.GetSize())
      {
        val=slid->parmchg_q.Enumerate(curpos, &splpos);
        slid->parmchg_q_curpos++;
      }
      *nextval=(EEL_F)val;
      return (EEL_F)splpos;
    }
  }

  *nextval=0.0;
  return -1.0;
}


EEL_F * NSEEL_CGEN_CALL _midi_send(void *opaque, EEL_F *tm, EEL_F *msg, EEL_F *msg23)
{
  SX_Instance *epctx = (SX_Instance *)opaque;
  if (epctx->midi_sendrecv)
  {
    epctx->midi_sendrecv(epctx->midi_ctxdata,1,tm,msg,msg23, epctx->m_extwantmidibus && *epctx->m_extwantmidibus ? epctx->m_midibus : NULL);
  }
  return msg;
}
EEL_F NSEEL_CGEN_CALL _midi_send2(void *opaque, INT_PTR np, EEL_F **parms)
{
  SX_Instance *epctx = (SX_Instance *)opaque;
  if (epctx->midi_sendrecv)
  {
    EEL_F v = (EEL_F) (((int) parms[2][0]) | (((int) parms[3][0]) <<8 ));
    epctx->midi_sendrecv(epctx->midi_ctxdata,1,parms[0],parms[1],&v, epctx->m_extwantmidibus && *epctx->m_extwantmidibus ? epctx->m_midibus : NULL);
  }
  return parms[1][0];
}

EEL_F NSEEL_CGEN_CALL _midi_send_buf(void *opaque, INT_PTR np, EEL_F **parms)
{
  SX_Instance *epctx = (SX_Instance *)opaque;
  if (epctx->midi_sendrecv)
  {
    int ioffs = (int)(parms[1][0] + 0.00001);
    int ilen = (int)(parms[2][0] + 0.00001);

    EEL_F **blocks = ((compileContext*)epctx->m_vm)->ram_state->blocks;
    EEL_F *ptr1,*ptr2;
    if (blocks && ilen > 0 &&
        (ptr1=__NSEEL_RAMAlloc(blocks,ioffs)) &&
        (ptr2=__NSEEL_RAMAlloc(blocks,ioffs+ilen-1)) &&
        ptr1!=&nseel_ramalloc_onfail && ptr2!=&nseel_ramalloc_onfail)
    {
      unsigned char *buf=NULL;
      bool do_hdr=false;

      const int fbyte = ((int)*ptr1)&0xff;
      if (fbyte == 0xFF)
      {
        // meta message, allow everything through
      }
      else
      {
        if (ilen >= 2 && fbyte == 0xF0 && (((int)*ptr2)&0xff) == 0xF7)
        {
          // already valid sysex!
          // if (blocksHave8BitData(blocks,ioffs+1,ilen-2)) return 0; // sysex must not have any high bits
        }
        else if (ilen <= 3 && (fbyte&0x80))
        {
          // normal midi message, most likely
          // if (blocksHave8BitData(blocks,ioffs+1,ilen-1)) return 0; // message must not have any high bits
        }
        else
        {
          // if we wanted to make sure the user wasn't doing something messy, we could
          // if (blocksHave8BitData(blocks,ioffs,ilen)) return 0; // sysex must not have any high bits
          do_hdr = true;
        }
      }

      if (do_hdr) ilen += 2;

      if (epctx->midi_sendrecv(epctx->midi_ctxdata,0x100,
          parms[0], //timestamp
          (EEL_F *)&ilen, // length
          (EEL_F *)&buf,
          epctx->m_extwantmidibus && *epctx->m_extwantmidibus ? epctx->m_midibus : NULL) > 0 && buf)
      {
        if (do_hdr)
        {
          buf[0]=0xF0;
          buf[ilen - 1] = 0xF7;
          buf++;
          ilen -= 2;
        }
        const int rv=ilen;

        while (ilen > 0)
        {
          //ptr1 is blocks+ioffs
          int lcnt=NSEEL_RAM_ITEMSPERBLOCK-(ioffs&(NSEEL_RAM_ITEMSPERBLOCK-1));
          if (lcnt > ilen) lcnt=ilen;

          if (ptr1 && ptr1 != &nseel_ramalloc_onfail)
          {
            int x;
            for (x=0;x<lcnt;x++) buf[x] = ((int) ptr1[x])&0xff;
          }
          else
          {
            memset(buf,0,lcnt);
          }

          ilen -= lcnt;
          ioffs += lcnt;
          buf += lcnt;

          ptr1=__NSEEL_RAMAlloc(blocks,ioffs); // get next block
        }
        return rv;
      }
    }
  }
  return 0.0;
}

EEL_F NSEEL_CGEN_CALL _midi_recv_buf(void *opaque, INT_PTR np, EEL_F **parms)
{
  SX_Instance *epctx = (SX_Instance *)opaque;
  if (epctx->midi_sendrecv)
  {
    int ioffs = (int)(parms[1][0] + 0.00001);
    int ilen = (int)(parms[2][0] + 0.00001);

    EEL_F **blocks = ((compileContext*)epctx->m_vm)->ram_state->blocks;
    EEL_F *ptr1;
    if (blocks && ilen > 0 &&
        (ptr1=__NSEEL_RAMAlloc(blocks,ioffs)) &&
        ptr1!=&nseel_ramalloc_onfail)
    {
      unsigned char *buf=NULL;
      int used_len=ilen;
      if (epctx->midi_sendrecv(epctx->midi_ctxdata,0x101,
          parms[0], //timestamp
          (EEL_F *)&used_len, // length
          (EEL_F *)&buf,
          epctx->m_extwantmidibus && *epctx->m_extwantmidibus ? epctx->m_midibus : NULL) > 0 && buf)
      {
        if (ilen > used_len) ilen=used_len;
        while (ilen > 0)
        {
          //ptr1 is blocks+ioffs
          int lcnt=NSEEL_RAM_ITEMSPERBLOCK-(ioffs&(NSEEL_RAM_ITEMSPERBLOCK-1));
          if (lcnt > ilen) lcnt=ilen;

          if (ptr1 && ptr1 != &nseel_ramalloc_onfail)
          {
            int x;
            for (x=0;x<lcnt;x++) ptr1[x] = buf[x];
          }

          ilen -= lcnt;
          ioffs += lcnt;
          buf += lcnt;

          ptr1=__NSEEL_RAMAlloc(blocks,ioffs); // get next block
        }
        return used_len;
      }
    }
  }
  return 0.0;
}



EEL_F * NSEEL_CGEN_CALL _midi_recv(void *opaque, EEL_F *tm, EEL_F *msg, EEL_F *msg23)
{
  SX_Instance *epctx = (SX_Instance *)opaque;
  *msg=0;
  if (epctx->midi_sendrecv)
  {
    epctx->midi_sendrecv(epctx->midi_ctxdata,-1,tm,msg,msg23,epctx->m_extwantmidibus && *epctx->m_extwantmidibus ? epctx->m_midibus : NULL);
  }

  return msg;
}


EEL_F NSEEL_CGEN_CALL _midi_recv2(void *opaque, INT_PTR np, EEL_F **parms)
{
  SX_Instance *epctx = (SX_Instance *)opaque;
  parms[1][0]=0;
  if (epctx->midi_sendrecv)
  {
    EEL_F v=0.0;
    if (epctx->midi_sendrecv(epctx->midi_ctxdata,-1,parms[0],parms[1],&v,epctx->m_extwantmidibus && *epctx->m_extwantmidibus ? epctx->m_midibus : NULL) > 0.0)
    {
      int iv = (int)v;
      parms[2][0] = (EEL_F) (iv&0xff);
      parms[3][0] = (EEL_F) ((iv>>8)&0xff);
    }
  }

  return parms[1][0];
}


EEL_F NSEEL_CGEN_CALL _midi_syx(void *opaque, EEL_F *tm, EEL_F *msg, EEL_F *len)
{
  SX_Instance *epctx = (SX_Instance *)opaque;
  if (!epctx->midi_sendrecv) return 0.0;

  EEL_F **blocks = ((compileContext*)epctx->m_vm)->ram_state->blocks;

  int ioffs = (int)(*msg+0.0001);
  int ilen = (int)(*len+0.0001);

  if (ioffs<0)
  {
    ilen += ioffs;
    ioffs = 0;
  }

  if (ioffs >= NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK) return 0.0;
  if (ioffs+ilen > NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK) ilen = NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK-ioffs;

  EEL_F scnt = 0.0;
  while (ilen > 0)
  {
    EEL_F *ptr=__NSEEL_RAMAlloc(blocks,ioffs);
    if (!ptr || ptr==&nseel_ramalloc_onfail) break;

    int lcnt=NSEEL_RAM_ITEMSPERBLOCK-(ioffs&(NSEEL_RAM_ITEMSPERBLOCK-1));
    if (lcnt > ilen) lcnt=ilen;

    ilen -= lcnt;
    ioffs += lcnt;

    EEL_F fcnt = lcnt;
    scnt += fcnt;
    epctx->midi_sendrecv(epctx->midi_ctxdata,2,tm,ptr,&fcnt,NULL);
  }

  return scnt;  // which should be *len
}

// these are classic jesusonic chain APIs, now unused
EEL_F NSEEL_CGEN_CALL SX_Instance::_bypass_set(SX_Instance *pthis, EEL_F *item, EEL_F *val) { return 0.0; }
EEL_F NSEEL_CGEN_CALL SX_Instance::_bypass_get(SX_Instance *pthis, EEL_F *item) { return 0.0; }
EEL_F NSEEL_CGEN_CALL SX_Instance::_slider_set(SX_Instance *pthis, EEL_F *item, EEL_F *val) { return 0.0; }
EEL_F NSEEL_CGEN_CALL SX_Instance::_slider_get(SX_Instance *pthis, EEL_F *item) { return 0.0; }

static int GetSliderMaskIndex(SX_Instance* ep, EEL_F* item, WDL_UINT64 *mask) // item = either mask or pointer to slider
{
  int idx = ep->find_slider_by_value_ptr(item);
  *mask = idx >= 0 ? 0 : (WDL_UINT64)(WDL_INT64)*item;
  return idx;
}

EEL_F NSEEL_CGEN_CALL _sliderchange(SX_Instance *pthis, EEL_F *item)
{
  pthis->m_slider_anychanged=true;
  WDL_UINT64 mask;
  const int idx = GetSliderMaskIndex(pthis,item,&mask);
  if (fabs(*item + 1.0) < 0.001)
  {
    pthis->gfx_notify_general_statechange();
  }
  // not sure if this return value matters but this is what it has been
  return (EEL_F) (idx>=0 && idx<64 ? (((WDL_UINT64)1)<<idx) : (WDL_UINT64)(WDL_INT64)*item);
}

static EEL_F NSEEL_CGEN_CALL _slider_automate(void *ctx, INT_PTR np, EEL_F **parms)
{
  SX_Instance *pthis=(SX_Instance*)ctx;
  pthis->m_slider_anychanged=true;

  WDL_UINT64 mask;
  const int idx = GetSliderMaskIndex(pthis,parms[0],&mask);
  if (pthis->m_slider_automate)
  {
    int cnt=0;
    for (int i = 0; i < pthis->m_sliders.GetSize(); ++i)
    {
      effectSlider *slid = pthis->m_sliders.Get(i);
      if (!slid) continue;
      if (idx >= 0 ? (i == idx) : (i < 64 && (mask&(((WDL_UINT64)1)<<i))))
      {
        pthis->m_slider_automate(pthis->m_hostctx, cnt, np < 2 || parms[1][0] > 0.5);
      }
      ++cnt;
    }
  }

  // not sure if this return value matters but this is what it has been
  return (EEL_F) (idx>=0 && idx<64 ? (((WDL_UINT64)1)<<idx) : (WDL_UINT64)(WDL_INT64)parms[0][0]);
}


EEL_F NSEEL_CGEN_CALL _slider_show(void *ctx, INT_PTR np, EEL_F **parms)
{
  SX_Instance *p=(SX_Instance*)ctx;
  if (!p || !np) return (EEL_F)0;

  WDL_UINT64 ret=0,mask;
  const int idx = GetSliderMaskIndex(p,parms[0],&mask);

  const int s = np == 1 ? -2 : *(parms[1]) < -0.5 ? -1 : *(parms[1]) >= 0.5 ? 1 : 0;
  for (int i=0; i < p->m_sliders.GetSize(); ++i)
  {
    effectSlider *slid = p->m_sliders.Get(i);
    if (slid && (idx >= 0 ? (i == idx) : (i < 64 && (mask&(((WDL_UINT64)1)<<i)))))
    {
      if (s == -1 || (s >= 0 && slid->show != s))
      {
        slid->show = s < 0 ? !slid->show : s;
        p->m_slider_vischanged=1;
      }
      if (slid->show && i<64) ret |= WDL_UINT64_CONST(1)<<i;
    }
  }
  return (EEL_F)ret;
}


EEL_F NSEEL_CGEN_CALL _serialize_avail(SX_Instance *epctx, EEL_F *wf)
{
  int a=(int)(*wf);
  if (a < 0 || a > MAX_EFFECT_FILES) return 0.0;

  WDL_MutexLock __lock(&epctx->m_string_sermutex);

  if (epctx->m_filehandles[a].m_mode==fileHandleRec::FILE_MODE_WRITE) return -1;

  if (epctx->m_filehandles[a].m_hb)
  {
    return epctx->m_filehandles[a].m_hb_wpos+4 <= epctx->m_filehandles[a].m_hb->GetSize()?1.0:0.0;
  }

  if (epctx->m_filehandles[a].m_fp || epctx->m_filehandles[a].m_pcm_src)
  {
    if (epctx->m_filehandles[a].m_fp && epctx->m_filehandles[a].m_mode==fileHandleRec::FILE_MODE_TEXT)
      return feof(epctx->m_filehandles[a].m_fp)?0.0:1.0;

    return (EEL_F)epctx->m_filehandles[a].m_itemsleft;
  }
  return 0.0;
}

EEL_F NSEEL_CGEN_CALL _serialize_close(SX_Instance *epctx, EEL_F *wf)
{
  int a=(int)(*wf);
  if (a < 1 || a > MAX_EFFECT_FILES) return -1.0;

  WDL_MutexLock __lock(&epctx->m_string_sermutex);
  epctx->m_filehandles[a].Close();

  *wf=-1.0;

  return 0.0;
}


EEL_F * NSEEL_CGEN_CALL  _serialize_rewind(SX_Instance *epctx, EEL_F *wf)
{
  int a=(int)(*wf);
  if (a < 1 || a > MAX_EFFECT_FILES) return wf;

  WDL_MutexLock __lock(&epctx->m_string_sermutex);
  epctx->m_filehandles[a].Rewind();

  return wf;
}

EEL_F NSEEL_CGEN_CALL _serialize_open(SX_Instance *epctx, EEL_F *fname)
{
  WDL_MutexLock __lock(&epctx->m_string_sermutex);

  WDL_FastString fn;
  int hasf=0;
  int x = epctx->find_slider_by_value_ptr(fname);
  if (x >= 0)
  {
    effectSlider *slid = epctx->m_sliders.Get(x);
    if (!slid || !slid->is_file_slider || !slid->currentFile) return -1.0;
    fn.Set(epctx->m_datadir.Get());
    fn.Append("/");
    fn.Append(slid->FileDirectory.Get());
    fn.Append(slid->currentFile);
    hasf++;
  }
  else // loop up in effect string table
  {
    if (epctx->GetFilenameForParameter(*fname, &fn,0))
      hasf++;
  }
  if (!hasf) return -1.0;

  for (x = 1; x <= MAX_EFFECT_FILES; x ++)
  {
    if (!epctx->m_filehandles[x].m_fp && !epctx->m_filehandles[x].m_pcm_src) break;
  }

  if (x == MAX_EFFECT_FILES+1 || !epctx->m_filehandles[x].Open(fn.Get(),0))
  {
    return -1.0;
  }

  return (EEL_F) x;
}

static EEL_F NSEEL_CGEN_CALL _get_host_placement(void *ctx, INT_PTR np, EEL_F **parms)
{
  SX_Instance *p=(SX_Instance*)ctx;
  if (p && p->m_hostctx && fxGetPlacement)
  {
    int f=0,cp=0;
    int rv = fxGetPlacement(p->m_hostctx,&cp,&f);
    if (np > 0) parms[0][0] = cp;
    if (np > 1) parms[1][0] = f;
    return rv;
  }
  return -99.0;
}

static EEL_F NSEEL_CGEN_CALL _get_host_numchan(void *ctx, EEL_F *parm)
{
  SX_Instance *p=(SX_Instance*)ctx;
  if (!p || !p->m_hostctx || !fxGetSetHostNumChan)
  {
    return (EEL_F)0;
  }
  return (EEL_F)fxGetSetHostNumChan(p->m_hostctx, NULL);
}

static unsigned int double_to_unsigned_int_mask(double v)
{
  return (unsigned int) (((WDL_INT64)v) & WDL_INT64_CONST(0xFFFFFFFF));
}

static unsigned int get_set_pin_mapping(void *ctx, int isout, int pin, int startchan,
  unsigned int chanmask, unsigned int *set_mapping) // set_mapping from main thread only
{
  if (!ctx || !fxGetSetPinMap2) return 0;
  if (pin < 0 || pin >= REAPER_MAX_CHANNELS) return 0;
  if (startchan < 0 || startchan >= REAPER_MAX_CHANNELS) return 0;

  unsigned int mapping[REAPER_MAX_CHANNELS];
  int sz=fxGetSetPinMap2(ctx, isout, mapping, startchan, NULL);

  if (pin >= sz)
  {
    for (int i=sz; i <= pin; ++i)
    {
      mapping[i] = i>=startchan && i < startchan+32 ? (1<<(i-startchan)) : 0;
    }
    sz=pin+1;
  }

  if (set_mapping)
  {
    mapping[pin] &= ~chanmask;
    mapping[pin] |= *set_mapping;
    fxGetSetPinMap2(ctx, isout, mapping, startchan, &sz);
  }
  return mapping[pin];
}

static EEL_F NSEEL_CGEN_CALL _get_pin_mapping(void *ctx, INT_PTR np, EEL_F **parms)
{
  SX_Instance *p=(SX_Instance*)ctx;
  if (!p || np != 4 || !p->m_hostctx)
  {
    return 0;
  }
  return (EEL_F)get_set_pin_mapping(p->m_hostctx,
    (int)*parms[0], (int)*parms[1], (int)*parms[2], double_to_unsigned_int_mask(*parms[3]), NULL);
}

static EEL_F NSEEL_CGEN_CALL _get_pinmapper_flags(void *ctx, EEL_F *v)
{
  SX_Instance *p=(SX_Instance*)ctx;
  if (!p || !p->m_hostctx || !fxGetSetHostNumChan)
  {
    return (EEL_F)0;
  }
  return (EEL_F)fxGetSetPinmapperFlags(p->m_hostctx, NULL);
}


static EEL_F NSEEL_CGEN_CALL _set_host_numchan(SX_Instance *p, EEL_F *v)
{
  if (!p || !p->m_hostctx || !fxGetSetHostNumChan || GetCurrentThreadId() != p->m_main_thread)
  {
    return (EEL_F)0;
  }
  int numchan=(int)*v;
  return (EEL_F)fxGetSetHostNumChan(p->m_hostctx, &numchan);
}

static EEL_F NSEEL_CGEN_CALL _set_pin_mapping(void *ctx, INT_PTR np, EEL_F **parms)
{
  SX_Instance *p=(SX_Instance*)ctx;
  if (!p || np != 5 || !p->m_hostctx || GetCurrentThreadId() != p->m_main_thread)
  {
    return (EEL_F)0;
  }
  unsigned int mapping=double_to_unsigned_int_mask(*parms[4]);
  return (EEL_F)get_set_pin_mapping(p->m_hostctx,
    (int)*parms[0], (int)*parms[1], (int)*parms[2], double_to_unsigned_int_mask(*parms[3]), &mapping);
}

EEL_F NSEEL_CGEN_CALL _set_pinmapper_flags(SX_Instance *p, EEL_F *v)
{
  if (!p || !p->m_hostctx || !fxGetSetHostNumChan || GetCurrentThreadId() != p->m_main_thread)
  {
    return (EEL_F)0;
  }

  int flags=(int)*v;
  EEL_F ret=(EEL_F)fxGetSetPinmapperFlags(p->m_hostctx, &flags);
  if (fxSetUndoPoint) fxSetUndoPoint(p->m_hostctx);
  return ret;
}

static bool validate_buffers(int baseoffs, int len, int nch, int planar_pitch)
{
  if (planar_pitch == 0)
    return baseoffs >= 0 && baseoffs+len*nch <= NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK;

  for (int x = 0; x < nch; x ++)
  {
    if (baseoffs < 0 || baseoffs+len > NSEEL_RAM_BLOCKS*NSEEL_RAM_ITEMSPERBLOCK) return false;
    baseoffs += planar_pitch;
  }

  return true;
}


static int read_data(NSEEL_VMCTX vm, double *dest, int pos, int amt)
{
  int rsz=0;
  while (amt > 0)
  {
    int avail=0;
    const EEL_F *rd = pos < 0 ? NULL : NSEEL_VM_getramptr(vm,pos,&avail);
    if (!rd || avail<1) return 0;
    if (avail > amt) avail=amt;

    amt -= avail;
    pos += avail;
    while (avail--) dest[rsz++] = *rd++;
  }
  return rsz;
}
  // export_buffer_to_project(buffer, length, channelcount, samplerate, trackidx[, flags for InsertMedia])
EEL_F NSEEL_CGEN_CALL _export_buffer_to_project(void *opaque, INT_PTR np, EEL_F **parms)
{
  SX_Instance *epctx = (SX_Instance *) opaque;
  if (!epctx || GetCurrentThreadId() != epctx->m_main_thread || !epctx->m_in_gfx || np < 5) return -1.0;

  double rv=-1.0;

  const int baseoffs = (int) (parms[0][0]+0.5);
  const int nch = (int) (parms[2][0]+0.5);
  const int len = (int) (parms[1][0]+0.5);
  const int sr = (int) (parms[3][0]+0.5);
  int trackidx = (int) (parms[4][0]+0.5);
  const int flags = np >= 6 ? (int) (parms[5][0]+0.5) : 0;
  const int planar_pitch = np >= 8 ? (int) floor(parms[7][0]+0.5) : 0;

  if (trackidx<0 || trackidx>0x7fff) trackidx=0x7fff;

  if (flags & 0x10000)
  {
    // go to end of project
    if (SetEditCurPos&&GetProjectLength)
    {
      SetEditCurPos(GetProjectLength(NULL),true,false);
    }
  }
  if ((flags & 0x20000) && np > 6 && parms[6][0] > 1.0 && parms[6][0] < 960.0)
  {
    // set tempo / add tempo marker
    if (SetTempoTimeSigMarker&&GetCursorPositionEx&&FindTempoTimeSigMarker&&GetTempoTimeSigMarker)
    {
      const double pos = GetCursorPositionEx(NULL);
      const int idx = FindTempoTimeSigMarker(NULL,pos);

      double lastpos=0.0, lastbpm=1.0;
      bool lastlinear=false;

      if (!GetTempoTimeSigMarker(NULL,idx,&lastpos,NULL,NULL,&lastbpm,NULL,NULL,&lastlinear))
      {
        if (CSurf_OnTempoChange && Master_GetTempo && !GetTempoTimeSigMarker(NULL,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL))
        {
          // no tempo markers
          if (fabs(pos)<0.0001 || fabs(parms[6][0] - Master_GetTempo()) < 0.001)
            CSurf_OnTempoChange(parms[6][0]);
          else
            SetTempoTimeSigMarker(NULL, -1, pos, -1, -1, parms[6][0], 0,0, false);
        }
        else
        {
          SetTempoTimeSigMarker(NULL, -1, pos, -1, -1, parms[6][0], 0,0, false);
        }
      }
      else
      {
        // these don't necessarily handle linear tempos correctly, but meh
        if (pos < lastpos + 0.000000001) // replace
        {
          SetTempoTimeSigMarker(NULL, idx, pos, -1, -1, parms[6][0], 0,0, false);
        }
        else
        {
          if (lastlinear || fabs(lastbpm-parms[6][0]) > 0.0001)
          {
            SetTempoTimeSigMarker(NULL, -1, pos, -1, -1, parms[6][0], 0,0, false);
          }
        }
      }
    }
  }

  if (nch >= 1 && nch <= REAPER_MAX_CHANNELS && len > 0 &&
      Plugin_FindNewFileName && PCM_Sink_Create &&
      sr > 0 && sr < (1<<20) &&
      validate_buffers(baseoffs,len,nch,planar_pitch))
  {
    char tmp[2048];
    tmp[0]=0;
    Plugin_FindNewFileName(NULL,trackidx,"$tracknumber-$track-$year2$month$day_$hour$minute-jsfx","wav",tmp,sizeof(tmp));
    if (tmp[0])
    {
      char cfgbuf[5];
      wdl_mem_store_int(cfgbuf, REAPER_FOURCC('w','a','v','e'));
      cfgbuf[4]=32;
      WDL_TypedBuf<ReaSample> block;
      const int blocksize = 1024;
      ReaSample *buf=block.ResizeOK(blocksize * nch);
      ReaSample *ptrs[REAPER_MAX_CHANNELS];

      if (buf)
      {
        PCM_sink *sink = PCM_Sink_Create(tmp,cfgbuf,5,nch,sr,true);
        if (sink)
        {
          int pos = baseoffs, remaining = len;

          if (planar_pitch != 0)
          {
            for (int x=0;x<nch;x++) ptrs[x] = buf + x * blocksize;
            while (remaining > 0)
            {
              int amt = wdl_min(blocksize,remaining);
              for (int x=0; x<nch && amt > 0; x++)
              {
                int v = read_data(epctx->m_vm,ptrs[x],pos + planar_pitch*x, amt);
                if (v < amt) amt=v;
              }
              if (amt < 1) break;
              sink->WriteDoubles(ptrs,amt,nch,0,1);

              pos += amt;
              remaining -= amt;
            }
          }
          else
          {
            for (int x=0;x<nch;x++) ptrs[x]=buf+x;
            while (remaining > 0)
            {
              const int amt = read_data(epctx->m_vm, buf, pos, wdl_min(blocksize, remaining) * nch) / nch;
              if (amt < 1) break;
              sink->WriteDoubles(ptrs,amt,nch,0,nch);

              pos += amt * nch;
              remaining -= amt;
            }
          }
          delete sink;

          if (InsertMedia)
          {
            InsertMedia(tmp,(trackidx<<16) | 512 | (flags&0x1fc /*only 4-256 allowed through*/));
          }
          rv=0.0;
        }
      }
    }
  }

  return rv;
}


void eel_js_register()
{
  NSEEL_addfunc_retval("file_open",1,NSEEL_PProc_THIS,&_serialize_open);
  NSEEL_addfunc_retval("file_close",1,NSEEL_PProc_THIS,&_serialize_close);
  NSEEL_addfunc_retptr("file_rewind",1,NSEEL_PProc_THIS,&_serialize_rewind);
  NSEEL_addfunc_retptr("file_var",2,NSEEL_PProc_THIS,&_serialize_var);
  NSEEL_addfunc_retval("file_string",2,NSEEL_PProc_THIS,&_serialize_string);
  NSEEL_addfunc_retval("file_mem",3,NSEEL_PProc_THIS,&_serialize_mem);
  NSEEL_addfunc_retptr("file_riff",3,NSEEL_PProc_THIS,&_serialize_riff);
  NSEEL_addfunc_retval("file_text",1,NSEEL_PProc_THIS,&_serialize_text);
  NSEEL_addfunc_retval("file_avail",1,NSEEL_PProc_THIS,&_serialize_avail);
  NSEEL_addfunc_retval("set_bypass",2,NSEEL_PProc_THIS,&SX_Instance::_bypass_set);
  NSEEL_addfunc_retval("get_bypass",1,NSEEL_PProc_THIS,&SX_Instance::_bypass_get);
  NSEEL_addfunc_retval("set_slider",2,NSEEL_PProc_THIS,&SX_Instance::_slider_set);
  NSEEL_addfunc_retval("get_slider",1,NSEEL_PProc_THIS,&SX_Instance::_slider_get);
  NSEEL_addfunc_retval("sliderchange",1,NSEEL_PProc_THIS,&_sliderchange);
  NSEEL_addfunc_varparm("slider_automate",1,NSEEL_PProc_THIS,&_slider_automate);
  NSEEL_addfunc_retval("strcpy_fromslider",2,NSEEL_PProc_THIS,&_strcpy_fromslider);

  NSEEL_addfunc_retptr("midisend",3,NSEEL_PProc_THIS,&_midi_send);
  NSEEL_addfunc_exparms("midisend",4,NSEEL_PProc_THIS,&_midi_send2);
  NSEEL_addfunc_exparms("midisend_buf",3,NSEEL_PProc_THIS,&_midi_send_buf);
  NSEEL_addfunc_exparms("midirecv_buf",3,NSEEL_PProc_THIS,&_midi_recv_buf);
  NSEEL_addfunc_retptr("midirecv",3,NSEEL_PProc_THIS,&_midi_recv);
  NSEEL_addfunc_exparms("midirecv",4,NSEEL_PProc_THIS,&_midi_recv2);
  NSEEL_addfunc_retptr("midisyx",3,NSEEL_PProc_THIS,&_midi_syx);

  NSEEL_addfunc_retptr("spl",1,NSEEL_PProc_THIS,&_spl_getter);
  NSEEL_addfunc_retptr("slider",1,NSEEL_PProc_THIS,&_slider_getter);

  NSEEL_addfunc_retval("slider_next_chg", 2, NSEEL_PProc_THIS, &_slider_next_chg);

  NSEEL_addfunc_varparm("slider_show", 2, NSEEL_PProc_THIS, &_slider_show);

  NSEEL_addfunc_retval("get_host_numchan",1,NSEEL_PProc_THIS,&_get_host_numchan);
  NSEEL_addfunc_exparms("get_pin_mapping",4,NSEEL_PProc_THIS,&_get_pin_mapping);
  NSEEL_addfunc_retval("get_pinmapper_flags",1,NSEEL_PProc_THIS,&_get_pinmapper_flags);

  NSEEL_addfunc_varparm("get_host_placement",1,NSEEL_PProc_THIS,&_get_host_placement);

  NSEEL_addfunc_varparm("export_buffer_to_project",5,NSEEL_PProc_THIS,&_export_buffer_to_project);
  NSEEL_addfunc_retval("set_host_numchan",1,NSEEL_PProc_THIS,&_set_host_numchan);
  NSEEL_addfunc_exparms("set_pin_mapping",5,NSEEL_PProc_THIS,&_set_pin_mapping);
  NSEEL_addfunc_retval("set_pinmapper_flags",1,NSEEL_PProc_THIS,&_set_pinmapper_flags);

}
