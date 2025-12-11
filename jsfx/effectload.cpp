/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * effect parsing/loading
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <stdarg.h>
#include <fcntl.h>

#include "sfxui.h"
#include "../WDL/dirscan.h"
#include "../WDL/wdlcstring.h"
#include "../WDL/win32_utf8.h"

#ifdef _WIN32
#include "../WDL/win32_utf8.c"
#endif

#include "../WDL/eel2/eel_pproc.h"

FILE *scanForEffect(const char *fn, WDL_FastString *actualfn, const char *config_effectdir);

double flexi_atof(const char *p)
{
  if (!p || !*p) return 0.0;
  char buf[512];
  lstrcpyn_safe(buf, p, sizeof(buf));

  char *n=buf;
  while (*n)
  {
    if (*n == ',') *n='.';
    n++;
  }
  return atof(buf);
}


class sliderLoader
{
public:
  sliderLoader()
  {
    def_v=1.0;
    range_min=0.0;
    range_max=0.0;
    range_scale=0.0;
    range_curve=0.0;
    has_scale=0;
    is_dir=0;
    no_external_shaping = false;
    shape = effectSlider::SHAPE_LINEAR;
  }
  ~sliderLoader()
  {
    valuelist.Empty(true,free);
  }
  WDL_FastString name, varname;
  double def_v;
  double range_min,range_max,range_scale,range_curve;
  int has_scale;
  int is_dir;
  int shape; // effectSlider::SHAPE_*
  bool no_external_shaping;
  WDL_FastString dir_name;
  WDL_FastString def_string;
  WDL_PtrList<char> valuelist;
};


static int recurseFindEffect(const char *root, const char *subdir, const char *find, WDL_FastString *out)
{
  WDL_FastString tmp(root);
  if (*subdir)
  {
    tmp.Append("/");
    tmp.Append(subdir);
  }

  WDL_DirScan dir;

  if (!dir.First(tmp.Get()))
  {
    do
    {
      if (dir.GetCurrentFN()[0] == '.') continue;
      if (dir.GetCurrentIsDirectory())
      {
        WDL_FastString tmp2;
        tmp2.Set(subdir);
        if (*subdir) tmp2.Append("/");
        tmp2.Append(dir.GetCurrentFN());

        if (recurseFindEffect(root,tmp2.Get(),find,out)) return 1;
      }
      else
      {
        if (!stricmp(find,dir.GetCurrentFN()))
        {
          out->Set(subdir);
          if (*subdir) out->Append("/");
          out->Append(dir.GetCurrentFN());
          return 1;
        }
      }
    }
    while (!dir.Next());
  }

  return 0;
}

struct effectLoadState
{
  effectLoadState()
  {
    m_misc_flags = 0;
    m_gfx_hz = 0;
    m_gfx_want_idle=0;
    m_no_meters=false;
    m_want_all_kb=false;
    sample_line_offs=-1;
    init_line_offs=-1;
    slider_line_offs=-1;
    block_line_offs=-1;
    serialize_line_offs=-1;
    gfx_line_offs=-1;
    gfx_reqh=gfx_reqw=0;
    max_ram=0;
    max_ram_prealloc=0;
    has_no_inputs=has_no_outputs=false;
  }
  ~effectLoadState()
  {
    in_pins.Empty(true,free);
    out_pins.Empty(true,free);
    localfilenames.Empty(true,free);
    lSliders.Empty(true);
  }

  WDL_FastString impfnname;
  int sample_line_offs, init_line_offs, slider_line_offs, block_line_offs;
  int serialize_line_offs, gfx_line_offs;
  WDL_FastString desc;
  int gfx_reqh,gfx_reqw;
  int max_ram, max_ram_prealloc;
  int m_gfx_want_idle;
  int m_gfx_hz;
  int m_misc_flags;
  bool has_no_inputs;
  bool has_no_outputs;
  bool m_want_all_kb,m_no_meters;
  WDL_FastString m_gmem_name;
  WDL_PtrList<char> in_pins,out_pins;
  WDL_FastString samplecode, initcode, slidercode, onblockcode;
  WDL_FastString serializecode,gfxcode;
  WDL_PtrList<char> localfilenames;
  WDL_PtrList<sliderLoader> lSliders; // non-present sliders will be NULL
  WDL_TypedBuf<int> pproc_line_inf;

  void load(FILE *fp, char *errbuf, int errbuf_sz, WDL_PtrList<char> *includestack,
            WDL_PtrList<effectLoadState> *importStates, const char *config_effectdir, EEL2_PreProcessor &pre_proc,
            WDL_PtrList<effectConfigItem> *config_items
            );
};

int get_slider_from_name(const char *str, char endc, int *outlen)
{
  if (strnicmp(str,"slider",6)) return 0;
  const char *p = str + 6;
  if (*p < '1' || *p > '9') return 0;
  while (*p >= '0' && *p <= '9') p++;
  if (outlen) *outlen = (int) (p - str);
  if (endc == ' ' ? !isspace_safe(*p) : (*p != endc)) return 0;

  const int rv = atoi(str + 6);
  if (rv < 1 || rv > NUM_SLIDERS) return 0;
  return rv;
}

void effectLoadState::load(FILE *effect_fp, char *errbuf, int errbuf_sz, WDL_PtrList<char> *includestack,
                           WDL_PtrList<effectLoadState> *importStates, const char *config_effectdir,
                           EEL2_PreProcessor &pre_proc,  WDL_PtrList<effectConfigItem> *config_items)
{
  int linemode=-1;
  int linepos=0;
  WDL_FastString * const dests[] = {
    &initcode,
    &slidercode,
    &onblockcode,
    &samplecode,
    &serializecode,
    &gfxcode,
  };

  if (!effect_fp) return;
  WDL_FastString file_str;
  while (file_str.GetLength() < (128<<20)) // 128MB should be enough for anybody
  {
    char buf[4096];
    if (!fgets(buf,sizeof(buf),effect_fp)) break;
    file_str.Append(buf);
  }

  if (config_items)
  {
    for (int x = 0; x < config_items->GetSize(); x ++) config_items->Get(x)->prev_val = 0.0;
    // scan for config items before preprocessing

    const char *read_ptr = file_str.Get();
    while (*read_ptr)
    {
      while (*read_ptr == '\r' || *read_ptr == '\n')
      {
        if (*read_ptr == '\n') linepos++;
        read_ptr++;
      }
      if (!*read_ptr || *read_ptr == '@') break;
      const char * const buf = read_ptr;
      while (*read_ptr && *read_ptr != '\n' && *read_ptr != '\r') read_ptr++;
      if (!strncmp(buf,"config:",7))
      {
        const char *endptr = read_ptr, *tok, *p = buf + 7;
        int toklen;
        int pos = 0, state=0;

        effectConfigItem *rec = NULL;

        while (NULL != (tok=nseel_simple_tokenizer(&p,endptr,&toklen,NULL)))
        {
          if (!toklen) break;
          if (WDL_NOT_NORMALLY(pos && !rec)) break;
          if (!pos)
          {
            for (int x = 0; x < config_items->GetSize(); x++)
            {
              effectConfigItem *r = config_items->Get(x);
              if (r->symbol.GetLength() == toklen && !memcmp(r->symbol.Get(),tok,toklen))
              {
                if (r->prev_val != 0.0)
                {
                  snprintf(errbuf,errbuf_sz,"%d: config: duplicate symbol '%s'",linepos+1,r->symbol.Get());
                  return;
                }

                rec = r;
                rec->values.Empty(true);
                rec->prev_val = 1.0;
                break;
              }
            }
            if (!rec) rec = new effectConfigItem(tok,toklen);
          }
          else if (pos == 1)
          {
            if (tok[0] == '\"')
            {
              tok++;
              if (--toklen>0 && tok[toklen-1]=='\"') toklen--;
            }
            rec->desc.Set(tok,toklen);
            const char *p = strstr(rec->desc.Get(), "-preserve-config");
            if (p)
            {
              rec->preserve_config = true;
              rec->desc.DeleteSub((int)(p-rec->desc.Get()),16);
            }
            while (rec->desc.GetLength()>0 && rec->desc.Get()[rec->desc.GetLength()-1] == ' ')
              rec->desc.SetLen(rec->desc.GetLength()-1);
          }
          else if (pos == 2)
          {
            rec->def_val = atof(tok);
            if (rec->prev_val == 2.0) rec->cur_val = rec->def_val;
            if (rec->def_val==0.0 && tok[0] != '0') break;
          }
          else
          {
            if (state == 1) state = *tok == '=' ? 2 : 0;
            else if (state == 2)
            {
              if (!rec->values.GetSize()) break;
              if (tok[0] == '\"')
              {
                tok++;
                if (--toklen>0 && tok[toklen-1]=='\"') toklen--;
              }
              rec->values.Get(rec->values.GetSize()-1)->name.Set(tok,toklen);
              state = 3;
            }
            else if (state == 3) state=0;

            if (state == 0)
            {
              const double v = atof(tok);
              if (v==0.0 && tok[0] != '0')
              {
                rec->values.Empty(true);
                break;
              }
              effectConfigItem::rec *r = new effectConfigItem::rec;
              r->v = v;
              rec->values.Add(r);
              state = 1;
            }
          }
          pos++;
        }
        if (!rec || rec->values.GetSize()<2)
        {
          if (rec && config_items->Find(rec)<0) delete rec;
          snprintf(errbuf,errbuf_sz,"%d:usage: config: symbol_name description default_value val1[=\"desc\"] val2[=\"desc\"] [...]",linepos+1);
          return;
        }
        if (rec && config_items->Find(rec)<0) config_items->Add(rec);
      }
    }

    for (int x = 0; x < config_items->GetSize(); x ++)
    {
      effectConfigItem *item = config_items->Get(x);
      if (item->prev_val == 0.0) config_items->Delete(x--,true);
      else pre_proc.define(item->symbol.Get(),item->cur_val);
    }
    linepos = 0;
  }

  if (strstr(file_str.Get(),EEL2_PREPROCESS_OPEN_TOKEN))
  {
    char *tmp = strdup(file_str.Get());
    if (tmp)
    {
      file_str.Set("");
      const char *err = pre_proc.preprocess(tmp, &file_str);
      free(tmp);
      if (err)
      {
        lstrcpyn_safe(errbuf,err,errbuf_sz);
        return;
      }
    }
  }

  char *read_ptr = (char *)file_str.Get();
  while (*read_ptr)
  {
    char * const buf = read_ptr;

    while (*read_ptr && *read_ptr != '\n') read_ptr++;
    if (read_ptr > buf && read_ptr[-1] == '\r') read_ptr[-1] = 0;
    if (*read_ptr) *read_ptr++ = 0;

    linepos++;
    int which_slider, boffs;
    if (linemode == -1 && (which_slider=get_slider_from_name(buf,':',&boffs))>0)
    {
      which_slider--; // make which_slider 0 based
      boffs++; // skip :

      while (lSliders.GetSize()<=which_slider) lSliders.Add(NULL);
      sliderLoader *slid = lSliders.Get(which_slider);
      if (!slid) lSliders.Set(which_slider, slid = new sliderLoader);

      if (buf[boffs] == '/')
      {
        char *p=buf+1+boffs;
        while (*p && *p != ':') p++;
        if (*p)
        {
          slid->dir_name.Set(buf+1+boffs,p-(buf+1+boffs));
          const char *tmp = slid->dir_name.Get();
          if (tmp && tmp[0] && tmp[strlen(tmp)-1]!='/') slid->dir_name.Append("/");
          char *np=++p;
          while (*np && *np != ':') np++;
          if (*np)
          {
            slid->def_string.Set(p,np-p);
            np++;
            while (*np == ' ') np++;
            slid->name.Set(np);

            slid->is_dir=1;
          }
        }
        if (!slid->is_dir && !errbuf[0]) snprintf(errbuf,errbuf_sz,"warning: got 'sliderX:' with a path and incomplete parameters");
      }
      else
      {
        // scan for = before ,<, if so, use as varname
        char *p=buf+boffs;
        while (*p && *p != ',' && *p != '<' && *p != '=') p++;
        if (*p == '=')
        {
          p++;
          const char *rd = buf+boffs;
          while (*rd == ' ') rd++;
          if ((p-1) > rd) slid->varname.Set(rd, (int)((p-1)-rd));
          slid->def_v=strtod(p,NULL);
        }
        else
        {
          slid->def_v=strtod(buf+boffs,NULL);
        }
        while (*p && *p != ',' && *p != '<') p++;
        if (*p == '<')
        {
          slid->range_min = strtod(++p,NULL);
          while (*p && *p != ',' && *p != '>') p++;
          if (*p == ',')
          {
            slid->range_max = strtod(++p,NULL);
          }
          while (*p && *p != ',' && *p != '>') p++;
          if (*p == ',')
          {
            slid->range_scale = strtod(++p,NULL);
            slid->has_scale=1;
            while (*p && *p != '>' && *p != '{')
            {
              int shape = !strnicmp(p,":log",4) ? effectSlider::SHAPE_LOG_PLAIN :
                          !strnicmp(p,":sqr",4) ? effectSlider::SHAPE_POW :
                          effectSlider::SHAPE_LINEAR;
              if (shape != effectSlider::SHAPE_LINEAR)
              {
                p += 4;
                if (*p == '!')
                {
                  slid->no_external_shaping = true;
                  p++;
                }
                if (*p == '=')
                {
                  p++;
                  if (shape == effectSlider::SHAPE_LOG_PLAIN)
                  {
                    const double v = (strtod(p,NULL) - slid->range_min) / (slid->range_max - slid->range_min);
                    if (v > 0.0 && v != 0.5 && v < 1.0)
                    {
                      shape = effectSlider::SHAPE_LOG_MIDPOINT; // log with parameter
                      slid->range_curve = 2.0 * log(fabs((v-1.0) / (v)));
                    }
                    else
                      shape = effectSlider::SHAPE_LINEAR;
                  }
                  else
                    slid->range_curve = strtod(p,NULL);
                }
                slid->shape = shape;
              }
              else
                p++;
            }
            if (*p == '{')
            {
              p++;
              while (*p && *p != '>' && *p != '}')
              {
                char *np=p;
                while (*np && *np != '>' && *np != '}' && *np != ',') np++;
                if (np > p)
                {
                  char *tmp=(char *)malloc(np-p + 1);
                  memcpy(tmp,p,np-p);
                  tmp[np-p]=0;
                  slid->valuelist.Add(tmp);
                }
                p=np;
                if (*p == ',') p++;
              }

            }
          }
          while (*p && *p != '>') p++;
        }
        if (*p)
        {
          p++;
          while (*p == ' ' || *p == ',') p++;
          slid->name.Set(p);
        }
      }
    }
    else if (linemode == -1 && !strncmp(buf,"in_pin:",7))
    {
      char *p=buf+7;
      while (*p == ' ') p++;
      has_no_inputs = (p[0] && !in_pins.GetSize() && (!strcmp(p, "-") || !stricmp(p, "none")));
      in_pins.Add(strdup(p)); // add the name in any case, if has_no_inputs by the end, clear the "none" name
    }
    else if (linemode == -1 && !strncmp(buf,"out_pin:",8))
    {
      char *p=buf+8;
      while (*p == ' ') p++;
      has_no_outputs = (p[0] && !out_pins.GetSize() && (!strcmp(p, "-") || !stricmp(p, "none")));
      out_pins.Add(strdup(p));  // add the name in any case, if has_no_outputs by the end, clear the "none" name
    }
    else if (linemode == -1 && !strncmp(buf,"import",6) && (buf[6] == ' ' || buf[6] == '\t' || buf[6] == 0))
    {
      char *p=buf+6;
      while (*p == ' ') p++;

      char *ep = p;
      while (*ep) ep++;
      while (ep > p && (ep[-1] == '\t' || ep[-1] == ' ')) *--ep = 0;

      if (!*p)
      {
        snprintf(errbuf,errbuf_sz,"import: must specify filename");
      }
      else
      {
        WDL_FastString s;
        FILE *fp=NULL;
        {
          char *ptr = p;
          while (!fp && *ptr)
          {
            // first try same path as loading effect
            if (includestack->GetSize() && includestack->Get(includestack->GetSize()-1)[0])
            {
              s.Set(includestack->Get(includestack->GetSize()-1));
              const char *sp=s.Get()+s.GetLength();
              while (sp>=s.Get() && *sp != '\\' && *sp != '/') sp--;
              s.SetLen(sp + 1 - s.Get());
              if (s.GetLength())
              {
                s.Append(ptr);
                fp=fopenUTF8(s.Get(),"rb");
              }
            }

            // scan past any / or \\, and try again
            if (!fp)
            {
              while (*ptr && *ptr != '\\' && *ptr != '/') ptr++;
              if (*ptr) ptr++;
            }
          }
        }
        // otherwise scan logic
        if (!fp) fp = scanForEffect(p,&s,config_effectdir);

        if (!fp)
        {
          snprintf(errbuf,errbuf_sz,"import: can't find file '%s'",p);
        }
        else
        {
          int x;
          for(x=0;x<includestack->GetSize() && stricmp(includestack->Get(x),s.Get());x++);
          if (x >= includestack->GetSize()) // prevent recursion/excessive reimporting
          {
            includestack->Add(strdup(s.Get()));

            effectLoadState *n=new effectLoadState;
            n->impfnname.Set(p);

            WDL_TypedBuf<int> bk = pre_proc.m_line_tab; // save copies of our line table
            pre_proc.m_line_tab.Resize(0);
            n->load(fp,errbuf,errbuf_sz,includestack,importStates,config_effectdir,pre_proc, NULL);
            n->pproc_line_inf = pre_proc.m_line_tab; // save the line table

            pre_proc.m_line_tab = bk;

            if (n->init_line_offs < 0 &&
                n->sample_line_offs < 0 &&
                n->block_line_offs < 0 &&
                n->gfx_line_offs < 0 &&
                n->serialize_line_offs < 0 &&
                n->slider_line_offs < 0)
            {
              snprintf(errbuf,errbuf_sz,"import: '%s' has no @init or other code",p);
              delete n;
            }
            else
            {
              importStates->Add(n);
            }
          }
          fclose(fp);
        }
      }
    }
    else if (linemode == -1 && !strncmp(buf,"desc:",5))
    {
      const char *p=buf+5;
      while (*p == ' ') p++;
      desc.Set(p);
    }
    else if (linemode == -1 && !strncmp(buf,"options:",8))
    {
      const char *p=buf+8;
      const char *tok;
      const char *endptr = p+strlen(p);
      int toklen;

      while (NULL != (tok=nseel_simple_tokenizer(&p,endptr,&toklen,NULL)))
      {
reprocess:
        if ((toklen == 6 && !strnicmp(tok,"maxmem",6)) ||
            (toklen == 8 && !strnicmp(tok,"prealloc",8)))
        {
          const bool ispre = toklen == 8;
          tok = nseel_simple_tokenizer(&p,endptr,&toklen,NULL);
          if (!tok) break;

          if (*tok != '=') goto reprocess;
          tok = nseel_simple_tokenizer(&p,endptr,&toklen,NULL);
          if (!tok) break;

          char tokbuf[512];
          if (++toklen > sizeof(tokbuf)) toklen = sizeof(tokbuf);
          lstrcpyn_safe(tokbuf,tok,toklen);
          if (ispre)
          {
            if (*tokbuf == '*')
            {
              max_ram_prealloc = -1;
            }
            else
            {
              max_ram_prealloc = atoi(tokbuf);
              if (max_ram_prealloc<0) max_ram_prealloc=0;
            }
          }
          else max_ram = atoi(tokbuf);
          continue;
        }

        if (toklen == 4 && !strnicmp(tok,"gmem",4))
        {
          tok = nseel_simple_tokenizer(&p,endptr,&toklen,NULL);
          if (!tok) break;

          if (*tok != '=') goto reprocess;
          tok = nseel_simple_tokenizer(&p,endptr,&toklen,NULL);
          if (tok)
          {
            m_gmem_name.Set(tok,toklen);
            continue;
          }
          m_gmem_name.Set("");
          break;
        }

        if (toklen == 6 && !strnicmp(tok,"gfx_hz",6))
        {
          tok = nseel_simple_tokenizer(&p,endptr,&toklen,NULL);
          if (!tok) break;

          if (*tok != '=') goto reprocess;
          tok = nseel_simple_tokenizer(&p,endptr,&toklen,NULL);
          if (!tok) break;

          m_gfx_hz = atoi(tok);
          continue;
        }

        if (toklen == 11 && !strnicmp(tok,"want_all_kb",11)) m_want_all_kb=true;
        if (toklen == 8 && !strnicmp(tok,"no_meter",8)) m_no_meters=true;
        if (toklen == 8 && !strnicmp(tok,"gfx_idle",8)) m_gfx_want_idle=1;
        if (toklen == 13 && !strnicmp(tok,"gfx_idle_only",13)) m_gfx_want_idle=2;
      }
    }
    else if (linemode == -1 && !strncmp(buf,"filename:",9))
    {
      //localfilenames
      char *p=buf+9;
      if (*p < '0' || *p > '9')
      {
        if (!errbuf[0]) snprintf(errbuf,errbuf_sz,"warning: malformed filename: line");
      }
      else
      {
        int a=atoi(p);
        if (a != localfilenames.GetSize())
          if (!errbuf[0]) snprintf(errbuf,errbuf_sz,"warning: filename: parameter %d out of sequence",a);
        while (*p && *p != ',') p++;
        if (!*p)
        {
          if (!errbuf[0]) snprintf(errbuf,errbuf_sz,"warning: filename: requires ,filename");
        }
        else
        {
          localfilenames.Add(strdup(++p));
        }
      }

    }
    else
    {
      if (buf[0] == '@')
      {
        if (!strncmp(buf+1,"init",4) && (buf[1+4] == 0 || isspace_safe(buf[1+4]))) linemode=0;
        else if (!strncmp(buf+1,"slider",6) && (buf[1+6] == 0 || isspace_safe(buf[1+6]))) linemode=1;
        else if (!strncmp(buf+1,"block",5) && (buf[1+5] == 0 || isspace_safe(buf[1+5]))) linemode=2;
        else if (!strncmp(buf+1,"sample",6) && (buf[1+6] == 0 || isspace_safe(buf[1+6]))) linemode=3;
        else if (!strncmp(buf+1,"serialize",9) && (buf[1+9] == 0 || isspace_safe(buf[1+9]))) linemode=4;
        else if (!strncmp(buf+1,"gfx",3) && (buf[1+3] == 0 || isspace_safe(buf[1+3])))
        {
          char *p=buf+4;
          while (*p == ' ') p++;
          if (*p) gfx_reqw=atoi(p);
          while (*p && *p != ' ') p++;
          while (*p == ' ') p++;
          if (*p) gfx_reqh=atoi(p);
          linemode=5;
        }
        else if (!strncmp(buf+1,"template ",9)) linemode = -1; // legacy removeme someday
        else
        {
          if (!errbuf[0]) snprintf(errbuf,errbuf_sz,"Unknown directive '%s'",buf);
          linemode=-1;
          // error
        }
        if (linemode >= 0)
        {
          if (linemode == 0 && init_line_offs < 0) init_line_offs = linepos;
          if (linemode == 1 && slider_line_offs < 0) slider_line_offs = linepos;
          if (linemode == 2 && block_line_offs < 0) block_line_offs = linepos;
          if (linemode == 3 && sample_line_offs < 0) sample_line_offs = linepos;
          if (linemode == 4 && serialize_line_offs < 0) serialize_line_offs = linepos;
          if (linemode == 5 && gfx_line_offs < 0) gfx_line_offs = linepos;
        }
      }
      else if (linemode >= 0 && linemode <= 5)
      {
        const char *str = buf;
        // legacy fixups
        if (linemode == 0 && !strcmp(str,"  lAngleMid = 90. + (0.5. - (0.5*mSpeakerLayout))*lAngleInc;")) // ATK
                                   str = "  lAngleMid = 90. + (0.5 - (0.5*mSpeakerLayout))*lAngleInc;";
        else if (linemode == 2 && !strcmp(str, "  mouse_x<15+60*cc_menu ? (1=1) :")) //vmorph
                                         str = "  mouse_x<15+60*cc_menu ? (1  ) :";
        else if (linemode == 2 && !strcmp(buf,"load_file == 1 || global_pdc = 0 ?")) // jonas drumreaplacer
                                        str = "load_file == 1 || global_pdc == 0?";
        else if (linemode == 3 && !strcmp(buf, "  wait_for_higher = max_peak = 0 = th_close = 0; ")) // drumreaplacer
                                         str = "  wait_for_higher = max_peak =     th_close = 0; ";

        dests[linemode]->Append(str);
        dests[linemode]->Append("\n");
      }
    }

    if (linemode == -1)
    {
      const char *p = strstr(buf,"tags:");
      if (p)
      {
        p+=5;
        const char *endptr = p+strlen(p), *tok;
        int toklen;
        while (NULL != (tok=nseel_simple_tokenizer(&p,endptr,&toklen,NULL)))
        {
          // make a list of tags?
          if (toklen == 10 && !strnicmp(tok,"instrument",10))
            m_misc_flags|=1;
        }
      }
    }
  }
}
FILE *scanForEffect(const char *fn, WDL_FastString *actualfn, const char *config_effectdir)
{

  actualfn->Set(config_effectdir);
  actualfn->Append("/");
  actualfn->Append(fn);

  FILE *fp = NULL;

  if (!strstr(fn,".."))
  {
  // make sure we're opening a file
#ifndef _WIN32
    struct stat statbuf;
    if (!stat(actualfn->Get(),&statbuf) && (statbuf.st_mode & S_IFMT) != S_IFDIR)
#endif
    {
      fp=fopenUTF8(actualfn->Get(),"r");
    }
  }
  if (!fp) // scan for the last part of fn
  {
    const char *fnp=fn;
    while (*fnp) fnp++;
    while (fnp >= fn && *fnp != '/' && *fnp != '\\') fnp--;
    fnp++;

    WDL_FastString sub;
    if (recurseFindEffect(config_effectdir,"",fnp,&sub))
    {
      actualfn->Set(config_effectdir);
      actualfn->Append("/");
      actualfn->Append(sub.Get());

      fp=fopenUTF8(actualfn->Get(),"r");
    }
  }

  return fp;
}

static double getVersionForScript()
{
  if (!GetAppVersion) return 0.0;
  const char *p = GetAppVersion();
  double d = atof(p);
  while (*p && *p != '+') p++;
  while (*p == '+' || (*p >= 'a' && *p <= 'z')) p++;
  if (!*p) return d;
  double sc = 0.0001;
  while (*p >= '0' && *p <= '9')
  {
    d+=sc * (*p++-'0');
    sc *= 0.1;
  }
  return d;
}

bool SX_Instance::LoadEffect(const char *fn, int resetFlags, WDL_PtrList<void> *hwnd_for_destroy)
{
  if (!fn[0]) return false;

  WDL_FastString actualfn;
  FILE *fp = scanForEffect(fn,&actualfn,m_effectdir.Get());
  if (!fp)
  {
    snprintf(m_last_error,sizeof(m_last_error),"File not found loading effect '%s'",fn);
    return false;
  }
  remove_idle();

  m_last_error[0]=0;

  int x;
  effectLoadState ls;
  WDL_PtrList<effectLoadState> impstates;
  WDL_PtrList<char> st;
  EEL2_PreProcessor pre_proc;
  WDL_FastString actualpath = actualfn;
  actualpath.remove_filepart();
  pre_proc.m_include_paths.Add(actualpath.Get());

  pre_proc.define("reaper_version",getVersionForScript());
  pre_proc.define("max_nch",MAX_NCH);

  if (resetFlags & LOADEFFECT_RESETFLAG_CONFIGITEMS) m_config_items.Empty(true);

  st.Add(strdup(actualfn.Get()));

  ls.load(fp,m_last_error,sizeof(m_last_error),&st,&impstates,m_effectdir.Get(),pre_proc, &m_config_items);
  st.Empty(true,free);

  if (fp) fclose(fp);

  if (ls.has_no_inputs) ls.in_pins.Empty(true,free);
  if (ls.has_no_outputs) ls.out_pins.Empty(true,free);

  if (resetFlags & LOADEFFECT_RESETFLAG_VARS)
  {
    resetVarsToStock(true, !!(resetFlags & LOADEFFECT_RESETFLAG_SLIDERS));
    m_slidercodehash = m_initcodehash = 0;
  }

  m_in_pinnames.Empty(true,free);
  m_out_pinnames.Empty(true,free);
  for (x = 0; x < ls.in_pins.GetSize(); x ++)
    m_in_pinnames.Add(ls.in_pins.Get(x));
  for (x = 0; x < ls.out_pins.GetSize(); x ++)
    m_out_pinnames.Add(ls.out_pins.Get(x));
  ls.in_pins.Empty(); // prevent these names from being freed, effect owns them
  ls.out_pins.Empty();

  m_has_no_inputs = ls.has_no_inputs || (m_out_pinnames.GetSize() && !m_in_pinnames.GetSize());
  m_has_no_outputs = ls.has_no_outputs || (m_in_pinnames.GetSize() && !m_out_pinnames.GetSize());

  m_full_actual_fn_used.Set(actualfn.Get());
  m_gfx_reqh=ls.gfx_reqh;
  m_gfx_reqw=ls.gfx_reqw;

  if (fn != m_fn.Get())
    m_fn.Set(fn);

  NSEEL_VM_clear_var_refcnts(m_vm);

  if (ls.max_ram>0)
    NSEEL_VM_setramsize(m_vm,ls.max_ram);
  if (ls.max_ram_prealloc!=0)
    NSEEL_VM_preallocram(m_vm,ls.max_ram_prealloc);

  set_gmem_name(ls.m_gmem_name.Get());
  m_want_all_kb = ls.m_want_all_kb;
  m_no_meters = ls.m_no_meters;
  m_gfx_want_idle = ls.m_gfx_want_idle;
  m_gfx_hz = ls.m_gfx_hz;

  WDL_UINT64 hashInit = 12345678;

  clear_code(resetFlags & LOADEFFECT_RESETFLAG_VARS);
  for (x=0;x<impstates.GetSize();x++)
  {
    effectLoadState *imps=impstates.Get(x);
    if (imps)
    {
      if (imps->initcode.Get()[0])
      {
        const char *err=add_imported_code(imps->initcode.Get(),imps->init_line_offs,&hashInit);
        if (err)
        {
          if (!m_last_error[0])
          {
            if (imps->pproc_line_inf.GetSize())
            {
              pre_proc.m_line_tab = imps->pproc_line_inf;
              err = pre_proc.translate_error_line(err);
            }
            snprintf(m_last_error,sizeof(m_last_error),"import %s:%s",imps->impfnname.Get(),err);
          }
        }
      }

      // other sections, use as defaults if the script itself doesn't implement
      if (ls.slider_line_offs<0 && !ls.slidercode.Get()[0] && imps->slidercode.Get()[0])
      {
        ls.slider_line_offs = imps->slider_line_offs;
        ls.slidercode = imps->slidercode;
      }
      if (ls.block_line_offs<0 && !ls.onblockcode.Get()[0] && imps->onblockcode.Get()[0])
      {
        ls.block_line_offs = imps->block_line_offs;
        ls.onblockcode = imps->onblockcode;
      }
      if (ls.serialize_line_offs<0 && !ls.serializecode.Get()[0] && imps->serializecode.Get()[0])
      {
        ls.serialize_line_offs = imps->serialize_line_offs;
        ls.serializecode = imps->serializecode;
      }
      if (ls.gfx_line_offs<0 && !ls.gfxcode.Get()[0] && imps->gfxcode.Get()[0])
      {
        ls.gfx_line_offs = imps->gfx_line_offs;
        ls.gfxcode = imps->gfxcode;
      }
      if (ls.sample_line_offs<0 && !ls.samplecode.Get()[0] && imps->samplecode.Get()[0])
      {
        ls.sample_line_offs = imps->sample_line_offs;
        ls.samplecode = imps->samplecode;
      }
    }
  }

  const char *err=set_init_code(ls.initcode.Get(),ls.init_line_offs,hashInit);
  if (err)
  {
    if (!m_last_error[0]) snprintf(m_last_error,sizeof(m_last_error),"@init:%s",pre_proc.translate_error_line(err));
  }
  err=set_slider_code(ls.slidercode.Get(),ls.slider_line_offs);
  if (err)
  {
    if (!m_last_error[0]) snprintf(m_last_error,sizeof(m_last_error),"@slider:%s",pre_proc.translate_error_line(err));
  }
  err=set_onblock_code(ls.onblockcode.Get(),ls.block_line_offs);
  if (err)
  {
    if (!m_last_error[0]) snprintf(m_last_error,sizeof(m_last_error),"@block:%s",pre_proc.translate_error_line(err));
  }
  err=set_sample_code(ls.samplecode.Get(),ls.sample_line_offs);
  if (err)
  {
    if (!m_last_error[0]) snprintf(m_last_error,sizeof(m_last_error),"@sample:%s",pre_proc.translate_error_line(err));
  }
  err=set_serialize_code(ls.serializecode.Get(),ls.serialize_line_offs);
  if (err)
  {
    if (!m_last_error[0]) snprintf(m_last_error,sizeof(m_last_error),"@serialize:%s",pre_proc.translate_error_line(err));
  }
  err=set_gfx_code(ls.gfxcode.Get(),ls.gfx_line_offs);
  if (err)
  {
    if (!m_last_error[0]) snprintf(m_last_error,sizeof(m_last_error),"@gfx:%s",pre_proc.translate_error_line(err));
  }

  if (!m_last_error[0])
  {
    NSEEL_VM_remove_unused_vars(m_vm);
  }
  impstates.Empty(true);

  m_description.Set(ls.desc.Get());
  if (actualfn.Get()[0] && m_effectdir.GetLength() && !strnicmp(actualfn.Get(),m_effectdir.Get(),m_effectdir.GetLength()))
  {
    const char *p = actualfn.Get()+m_effectdir.GetLength();
    while (*p == '/' || *p == '\\') p++;
    m_fn.Set(p);
  }

  // remove any sliders that will be unused
  for (x = m_sliders.GetSize()-1; x>=0; x --)
  {
    sliderLoader *src = ls.lSliders.Get(x);
    if (src && src->name.GetLength()) continue;

    effectSlider *slid = m_sliders.Get(x);

    if (x > ls.lSliders.GetSize()) m_sliders.Delete(x);
    else m_sliders.Set(x,NULL);

    if (slid)
    {
      if (slid->ui.combo) hwnd_for_destroy->Add(slid->ui.combo);
      if (slid->ui.label) hwnd_for_destroy->Add(slid->ui.label);
      if (slid->ui.slider) hwnd_for_destroy->Add(slid->ui.slider);
      if (slid->ui.edit) hwnd_for_destroy->Add(slid->ui.edit);
      delete slid;
    }
  }

  for (x = 0; x < ls.lSliders.GetSize(); x ++)
  {
    if (m_sliders.GetSize() <= x) m_sliders.Add(NULL);

    sliderLoader *src = ls.lSliders.Get(x);
    if (!src || !src->name.GetLength()) continue;

    effectSlider *slid = m_sliders.Get(x);
    if (!slid) m_sliders.Set(x,slid = new effectSlider);

    EEL_F *new_p;
    if (src->varname.GetLength())
    {
      new_p = NSEEL_VM_regvar(m_vm,src->varname.Get());
    }
    else
    {
      char buf[64];
      snprintf(buf,sizeof(buf),"slider%d",x+1);
      new_p = NSEEL_VM_regvar(m_vm,buf);
    }

    if (new_p && new_p != slid->slider)
    {
      if (slid->slider) *new_p = slid->slider[0];
      slid->slider = new_p;
    }

    if ((resetFlags & LOADEFFECT_RESETFLAG_SLIDERS) && WDL_NORMALLY(slid->slider != NULL))
    {
      slid->slider[0] = src->def_v;
    }
    slid->default_val = src->def_v;
    slid->range_min = src->range_min;
    slid->range_max = src->range_max;

    const char *nm=src->name.Get();
    slid->show = nm[0] != '-';
    if (nm[0] == '-') ++nm;
    slid->name.Set(nm);

    if (src->is_dir)
    {
      slid->is_file_slider=1;
      slid->FileDirectory.Set(src->dir_name.Get());
    }
    else
    {
      slid->sliderValueXlateList.Empty(true,free);
      slid->sliderValueXlateList = src->valuelist;
      src->valuelist.Empty(); // slid->sliderValueXlateList takes ownership
    }

    const double drange = fabs(src->range_max-src->range_min);
    if (!src->has_scale && drange > 1e-20)
    {
      slid->range_scale = pow(10.0,(int)log10(drange) - 2);
    }
    else slid->range_scale = src->range_scale;

    // precalculate slider UI scalings
    double ni = 30000.0;

    if (!slid->sliderValueXlateList.GetSize() &&
        !slid->is_file_slider &&
        src->shape!=effectSlider::SHAPE_LINEAR &&
        (src->shape != effectSlider::SHAPE_LOG_PLAIN || wdl_min(src->range_min,src->range_max) >= 0.00001) &&
        slid->range_max != slid->range_min)
    {
      slid->external_shaping = !src->no_external_shaping;
      slid->ui_is_shape = src->shape;

      switch (slid->ui_is_shape)
      {
        case effectSlider::SHAPE_LOG_PLAIN:
          slid->ui_shape_parm = 0.0;
        break;
        case effectSlider::SHAPE_POW:
          slid->ui_shape_parm = src->range_curve > 0.0 ? src->range_curve : 2.0;
        break;
        case effectSlider::SHAPE_LOG_MIDPOINT:
          slid->ui_shape_parm = src->range_curve;
          slid->ui_shape_parm2 = (exp(slid->ui_shape_parm) - 1) / (slid->range_max-slid->range_min);
        break;
      }

      const double lcstart = slid->_sc(slid->range_min), lcend = slid->_sc(slid->range_max);
      slid->ui_lcstart = lcstart;
      slid->ui_lcend = lcend;

      if (slid->range_scale > 0.0)
      {
        double sc = slid->range_scale;
        if (slid->range_min > slid->range_max) sc = -sc;

        // calculate max steps such that the smallest step is not larger than range_scale
        double dn = slid->_sc(slid->range_min + sc) - lcstart;
        double dn2 = lcend - slid->_sc(slid->range_max - sc);
        if (dn2 > 0.0 && dn2 < dn) dn = dn2;
        if (dn > 0.0)
          ni = (lcend - lcstart) / dn;
      }
    }
    else
    {
      slid->external_shaping = false;
      slid->ui_is_shape = effectSlider::SHAPE_LINEAR;
      if (slid->range_scale > 0.0)
        ni=drange/slid->range_scale;
    }

    if (ni >= 30000.0) slid->ui_step_count = 30000;
    else if (ni <= 1.0) slid->ui_step_count = 1;
    else slid->ui_step_count = (int) floor(ni + 0.5);
  }

  m_localfilenames.Empty(true,free);
  m_localfilenames = ls.localfilenames;
  ls.localfilenames.Empty(); // m_locafilenames takes ownership

  if (resetFlags & LOADEFFECT_RESETFLAG_VARS)
  {
    for (x = 1; x <= MAX_EFFECT_FILES; x ++) m_filehandles[x].Close();
  }

  bm_cnt=0; // reset any benchmarks

  m_misc_flags = ls.m_misc_flags;

  update_var_names();

  if (m_gfx_want_idle) add_idle();

  rescan_file_sliders();

  return true;
}
