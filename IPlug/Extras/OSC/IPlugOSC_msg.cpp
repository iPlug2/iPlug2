/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifdef _WIN32
#include <windows.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "IPlugOSC_msg.h"

using namespace iplug;

static int pad4(int len)
{
  return (len+4)&~3;
}

static int _strlen(const char* p, int maxlen)
{
  int i=0;
  while (i < maxlen && *p) 
  {
    ++p;
    ++i;
  }
  return i;
}

OscMessageRead::OscMessageRead(char* buf, int len)
{
  m_msgok=false;

  if (!buf || len < 1) return;
  if (len > MAX_OSC_MSG_LEN) len=MAX_OSC_MSG_LEN;
 
  m_msg_ptr=buf;
  int n=pad4(_strlen(m_msg_ptr, len));

  if (n == len)
  {
    // this is not technically allowed in the OSC spec,
    // but some implementations omit the type tag with no arguments
    m_msg_end=buf+n;
    m_type_ptr=buf+n;
    m_type_end=buf+n;
    m_arg_ptr=buf+n;
    m_arg_end=buf+n;
    m_msgok=true;
  }
  else if (n < len)
  {
    m_msg_end=buf+n;
    m_type_ptr=buf+n;    
    n += pad4(_strlen(m_type_ptr, len-n));

    if (n <= len)
    {
      m_type_end=buf+n;      
      m_arg_ptr=buf+n;

      const char* t=m_type_ptr;
      if (*t == ',')
      {
        ++m_type_ptr;
        while (++t < m_type_end && *t && n < len)
        {
          if (*t == 'i')
          {
            OSC_MAKEINTMEM4BE(buf+n);
            n += sizeof(int);
          }
          else if (*t == 'f')
          {
            OSC_MAKEINTMEM4BE(buf+n);
            n += sizeof(float);
          }
          else if (*t == 's')
          {
            n += pad4(_strlen(buf+n, len-n));
          }
          else
          {
            return; // unknown argument type      
          }
        }      

        if (t <= m_type_end && !*t && n <= len)
        {
          m_arg_end=buf+n;
          m_msgok=true;       
        }
      }
    }
  }
}

const char* OscMessageRead::GetMessage() const
{
  return (m_msgok ? m_msg_ptr : "");  
}

int OscMessageRead::GetNumArgs() const
{
  if (!m_msgok) return 0;
  if (m_type_ptr >= m_type_end) return 0;
  return strlen(m_type_ptr);
}

const char* OscMessageRead::PopWord()
{
  if (!m_msgok) return 0;
  if (m_msg_ptr >= m_msg_end) return 0;
  char* p=m_msg_ptr;
  if (*p == '/') ++p;
  char* q=strstr(p, "/");
  if (q)
  {
    *q=0;
    m_msg_ptr=q+1;
  }
  else
  {
    m_msg_ptr=m_msg_end;
  }   
  if (!*p) p=0;
  return p;
}

const void *OscMessageRead::GetIndexedArg(int idx, char *typeOut) const
{
  if (!m_msgok || idx<0) return 0;
  const char *ptr = m_type_ptr;
  const char *valptr = m_arg_ptr;
  if (ptr >= m_type_end || valptr >= m_arg_end) return 0;

  while (idx>0)
  {
    if (*ptr == 'i') valptr += sizeof(int);
    else if (*ptr == 'f') valptr += sizeof(float);
    else if (*ptr == 's') valptr += pad4(strlen(valptr));
    else return 0;

    idx--;
    ptr++;
    if (ptr >= m_type_end || valptr >= m_arg_end) return 0;
  }

  *typeOut = *ptr;

  const char *endptr = valptr; 
  if (*ptr == 'i') endptr += sizeof(int);
  else if (*ptr == 'f') endptr += sizeof(float);
  else if (*ptr == 's') endptr += pad4(strlen(valptr));
  else return 0;

  if (endptr > m_arg_end) return 0;

  return valptr;
}

const int* OscMessageRead::PopIntArg(bool peek)
{
  if (!m_msgok) return 0;
  if (m_type_ptr >= m_type_end) return 0;
  if (*m_type_ptr != 'i') return 0;
  if (m_arg_ptr+sizeof(int) > m_arg_end) return 0;

  int* p=(int*)m_arg_ptr;
  if (!peek)
  {
    ++m_type_ptr;
    m_arg_ptr += sizeof(int);
  }

  return p;
}

const float* OscMessageRead::PopFloatArg(bool peek)
{
  if (!m_msgok) return 0;
  if (m_type_ptr >= m_type_end) return 0;
  if (*m_type_ptr != 'f') return 0;
  if (m_arg_ptr+sizeof(float) > m_arg_end) return 0;

  float* p=(float*)m_arg_ptr;

  if (!peek)
  {
    ++m_type_ptr;
    m_arg_ptr += sizeof(float);
  }

  return p;
}

const char* OscMessageRead::PopStringArg(bool peek)
{
  if (!m_msgok) return 0;
  if (m_type_ptr >= m_type_end) return 0;
  if (*m_type_ptr != 's') return 0;
  if (m_arg_ptr+strlen(m_arg_ptr) > m_arg_end) return 0;

  const char* p=(const char*)m_arg_ptr;
  if (!peek)
  {
    ++m_type_ptr;
    int n=pad4(strlen(p));
    m_arg_ptr += n;
  }

  return p;
}

void OscMessageRead::DebugDump(const char* label, char* dump, int dumplen) 
{
  // dump message even if it is invalid
  if (!label) label="";
  strcpy(dump, label);
  strcat(dump, m_msg_ptr);

  int n=pad4(strlen(m_msg_ptr));

  const char* t=m_msg_ptr+n;
  if (*t == ',')
  {    
    sprintf(dump+strlen(dump), " [%s]", t+1);

    n += pad4(strlen(t));

    const char* a=m_msg_ptr+n;
    while (*t++)
    {  
      if (*t == 'i')
      {
        sprintf(dump+strlen(dump), " %d", *(int*)a);
        a += sizeof(int);
      }
      else if (*t == 'f')
      {
        sprintf(dump+strlen(dump), " %f", *(float*)a);
        a += sizeof(float);
      }
      else if (*t == 's')
      {
        sprintf(dump+strlen(dump), " \"%s\"", a);
        a += pad4(strlen(a));        
      }
      else
      {
        sprintf(dump+strlen(dump), " %c:(unknown argument type)", *t);
        break;
      }
    }
  }
}



OscMessageWrite::OscMessageWrite()
{
  m_msg[0]=0;
  m_types[0]=0;
  m_args[0]=0;

  m_msg_ptr=m_msg;
  m_type_ptr=m_types;
  m_arg_ptr=m_args;
}

bool OscMessageWrite::PushWord(const char* word)
{
  int len=strlen(word);
  if (m_msg_ptr+len+1 >= m_msg+sizeof(m_msg)) return false;

  strcpy(m_msg_ptr, word);
  m_msg_ptr += len;
  return true;
}

bool OscMessageWrite::PushInt(int val)
{
  char buf[64];
  sprintf(buf, "%d", val);
  return PushWord(buf);
}

bool OscMessageWrite::PushIntArg(int val)
{
  if (m_type_ptr+1 > m_types+sizeof(m_types)) return false;
  if (m_arg_ptr+sizeof(int) > m_args+sizeof(m_args)) return false;

  *m_type_ptr++='i'; 
  *m_type_ptr=0;

  *(int*)m_arg_ptr=val;
  OSC_MAKEINTMEM4BE(m_arg_ptr);
  m_arg_ptr += sizeof(int);
  
  return true;
}

bool OscMessageWrite::PushFloatArg(float val)
{
  if (m_type_ptr+1 > m_types+sizeof(m_types)) return false;
  if (m_arg_ptr+sizeof(float) > m_args+sizeof(m_args)) return false;

  *m_type_ptr++='f';
  *m_type_ptr=0;

  *(float*)m_arg_ptr=val;
  OSC_MAKEINTMEM4BE(m_arg_ptr);
  m_arg_ptr += sizeof(float);
  
  return true;
}

bool OscMessageWrite::PushStringArg(const char* val)
{
  int len=strlen(val);
  int padlen=pad4(len);

  if (m_type_ptr+1 > m_types+sizeof(m_types)) return false;
  if (m_arg_ptr+padlen > m_args+sizeof(m_args)) return false;

  *m_type_ptr++='s';
  *m_type_ptr=0;

  strcpy(m_arg_ptr, val);
  memset(m_arg_ptr+len, 0, padlen-len);
  m_arg_ptr += padlen;

  return true;
}

const char* OscMessageWrite::GetBuffer(int* len)
{
  int msglen=m_msg_ptr-m_msg;
  int msgpadlen=pad4(msglen);

  int typelen=m_type_ptr-m_types+1; // add the comma
  int typepadlen=pad4(typelen);

  int arglen=m_arg_ptr-m_args; // already padded

  if (msgpadlen+typepadlen+arglen > sizeof(m_msg)) 
  {
    if (len) *len=0;
    return "";
  }

  char* p=m_msg;
  memset(p+msglen, 0, msgpadlen-msglen);
  p += msgpadlen;
  
  *p=',';
  strcpy(p+1, m_types);
  memset(p+typelen, 0, typepadlen-typelen);
  p += typepadlen;

  memcpy(p, m_args, arglen);
  p += arglen;

  if (len) *len=p-m_msg;
  return m_msg;
}

void OscMessageWrite::DebugDump(const char* label, char* dump, int dumplen)
{
  int len=0;
  const char* p=GetBuffer(&len);
  if (p && len && len <= MAX_OSC_MSG_LEN)
  {
    char buf[MAX_OSC_MSG_LEN];
    memcpy(buf, p, len);
    OscMessageRead rmsg(buf, len);
    rmsg.DebugDump(label, dump, dumplen);    
  }
}
