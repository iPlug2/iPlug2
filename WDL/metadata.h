#ifndef _WDL_METADATA_H_
#define _WDL_METADATA_H_

#include <math.h>
#include "wdlstring.h"
#include "xmlparse.h"
#include "fileread.h"
#include "filewrite.h"
#include "queue.h"
#include "win32_utf8.h"
#include "wdl_base64.h"
#include "coreaudio_channel_formats.h"


const char *EnumMexKeys(int i, const char **desc=NULL)
{
  // TO_DO_IF_METADATA_UPDATE
  static const char *s_mexkeys[]=
    {"TITLE", "ARTIST", "ALBUM", "TRACKNUMBER", "YEAR", "GENRE", "COMMENT", "DESC", "BPM", "KEY", "DB_CUSTOM"};
  static const char *s_mexdesc[]=
    {"Title", "Artist", "Album", "Track", "Date", "Genre", "Comment", "Description", "BPM", "Key", "Media Explorer Tags"};

  bool ok = i >= 0 && i < sizeof(s_mexkeys)/sizeof(s_mexkeys[0]);
  if (desc) *desc = ok ? s_mexdesc[i] : NULL;
  return ok ? s_mexkeys[i] : NULL;
}


int MetadataToArray(WDL_StringKeyedArray<char*> *metadata, WDL_TypedBuf<const char*> *metadata_arr)
{
  if (!metadata || !metadata_arr) return 0;

  int cnt=0;
  for (int i=0; i < metadata->GetSize(); ++i)
  {
    const char *k, *v=metadata->Enumerate(i, &k);
    if (k && v)
    {
      metadata_arr->Add(k);
      metadata_arr->Add(v);
      ++cnt;
    }
  }
  metadata_arr->Add(NULL);
  return cnt;
}

int ArrayToMetadata(const char **metadata_arr, WDL_StringKeyedArray<char*> *metadata)
{
  if (!metadata_arr || !metadata) return 0;

  int cnt=0;
  for (; metadata_arr[0] && metadata_arr[1]; metadata_arr += 2)
  {
    metadata->AddUnsorted(metadata_arr[0], strdup(metadata_arr[1]));
    ++cnt;
  }
  if (cnt) metadata->Resort();
  return cnt;
}


char *tag_strndup(const char *src, int len)
{
  if (!src || !len) return NULL;
  int n=0;
  while (n < len && src[n]) ++n;
  char *dest=(char*)malloc(n+1);
  if (!dest) return NULL;
  memcpy(dest, src, n);
  dest[n]=0;
  return dest;
}

WDL_UINT64 ParseUInt64(const char *val)
{
  WDL_UINT64 i=0;
  if (val)
  {
    const char *p=val;
    while (*p)
    {
      int d=*p-'0';
      if (d < 0 || d > 9) break;
      i=(10*i)+d;
      ++p;
    }
    if (*p) i=0;
  }
  return i;
}

void InsertMetadataIncrKeyIfNeeded(WDL_StringKeyedArray<char*> *metadata,
  const char *key, const char *val)
{
  if (!metadata->Exists(key))
  {
    metadata->Insert(key, strdup(val));
  }
  else
  {
    for (int i=2; i < 100; ++i)
    {
      char str[2048];
      snprintf(str,sizeof(str), "%s:%d", key, i);
      if (!metadata->Exists(str))
      {
        metadata->Insert(str, strdup(val));
        break;
      }
    }
  }
}

void XMLCompliantAppend(WDL_FastString *str, const char *txt, bool is_value)
{
  if (str && txt) for (;;)
  {
    char c = *txt++;
    switch (c)
    {
      case 0: return;
      case '<': str->Append("&lt;"); break;
      case '>': str->Append("&gt;"); break;
      case '&': str->Append("&amp;"); break;
      case ' ': str->Append(is_value ? " " : "_"); break;
      default: str->Append(&c,1); break;
    }
  }
}

const char *XMLHasOpenTag(WDL_FastString *str, const char *tag) // tag like "<FOO>")
{
  // stupid
  int taglen=strlen(tag);
  const char *open=strstr(str->Get(), tag);
  while (open)
  {
    const char *close=strstr(open+taglen, tag+1);
    if (!close || WDL_NOT_NORMALLY(close[-1] != '/')) break;
    open=strstr(close+taglen-1, tag);
  }
  return open;
}

void UnpackXMLElement(const char *pre, wdl_xml_element *elem,
  WDL_StringKeyedArray<char*> *metadata)
{
  WDL_FastString key;
  if (stricmp(elem->name, "BWFXML"))
  {
    key.SetFormatted(512, "%s:%s", pre, elem->name);
    pre=key.Get();
  }
  if (elem->value.Get()[0])
  {
    const char *k=key.Get();
    if (!strncmp(k, "IXML:ASWG:", 10))
    {
      k += 5;
    }
    else if (!strncmp(k, "IXML:BEXT:", 10))
    {
      // we could rewrite this as follows, but this might overwrite an actual bext chunk so maybe not?
      /*
      if (!strcmp(k+10, "BWF_DESCRIPTION")) k="BWF:Description";
      else if (!strcmp(k+10, "BWF_ORIGINATOR")) k="BWF:Originator";
      else if (!strcmp(k+10, "BWF_ORIGINATOR_REFERENCE")) k="BWF:OriginatorReference";
      else if (!strcmp(k+10, "BWF_ORIGINATION_DATE")) k="BWF:OriginationDate";
      else if (!strcmp(k+10, "BWF_ORIGINATION_TIME")) k="BWF:OriginationTime";
      else if (!strcmp(k+10, "BWF_TIME_REFERENCE")) k="BWF:TimeReference"; // todo parse lo/hi
      else if (!strcmp(k+10, "BWF_VERSION")) k="BWF:Version";
      else if (!strcmp(k+10, "BWF_LOUDNESS_VALUE")) k="BWF:LoudnessValue";
      else if (!strcmp(k+10, "BWF_LOUDNESS_RANGE")) k="BWF:LoudnessRange";
      else if (!strcmp(k+10, "BWF_MAX_TRUE_PEAK_LEVEL")) k="BWF:MaxTruePeakLevel";
      else if (!strcmp(k+10, "BWF_MAX_MOMENTARY_LOUDNESS")) k="BWF:MaxMomentaryLoudness";
      else if (!strcmp(k+10, "BWF_MAX_SHORT_TERM_LOUDNESS")) k="BWF:MaxShortTermLoudness";
      */
    }
    InsertMetadataIncrKeyIfNeeded(metadata, k, elem->value.Get());
  }
  for (int i=0; i < elem->elements.GetSize(); ++i)
  {
    wdl_xml_element *elem2=elem->elements.Get(i);
    UnpackXMLElement(pre, elem2, metadata);
  }
}

bool UnpackIXMLChunk(const char *buf, int buflen,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!buf || !buflen || !metadata) return false;

  while (buflen > 20 && strnicmp(buf, "<BWFXML>", 8))
  {
    ++buf;
    --buflen;
  }
  if (buflen >= 20)
  {
    wdl_xml_parser xml(buf, buflen);
    if (!xml.parse() && xml.element_root)
    {
      UnpackXMLElement("IXML", xml.element_root, metadata);
      return true;
    }
  }

  return false;
}

bool IsXMPMetadata(const char *name, WDL_FastString *key)
{
  if (!name || !name[0] || !key) return false;

  // returns true if this XMP schema is one we know/care about
  if (!strnicmp(name, "xmpDM:", 6) && name[6])
  {
    key->SetFormatted(512, "XMP:dm/%s", name+6);
    return true;
  }
  if (!strnicmp(name, "dc:", 3) && name[3])
  {
    key->SetFormatted(512, "XMP:dc/%s", name+3);
    return true;
  }
  return false;
}

double UnpackXMPTimestamp(wdl_xml_element *elem)
{
  double tval=-1.0;
  int num=0, denom=0;
  for (int i=0; i < elem->attributes.GetSize(); ++i)
  {
    char *attr;
    const char *val=elem->attributes.Enumerate(i, &attr);
    if (!strcmp(attr, "xmpDM:scale") && val && val[0])
    {
      if (sscanf(val, "%d/%d", &num, &denom) != 2) num=denom=0;
    }
    else if (!strcmp(attr, "xmpDM:value") && val && val[0])
    {
      tval=atof(val);
    }
  }
  if (tval >= 0.0 && num > 0 && denom > 0)
  {
    return tval*(double)num/(double)denom;
  }
  return -1.0;
}

const char *GetXMPSubElement(wdl_xml_element *elem, const char *name)
{
  for (int i=0; i < elem->elements.GetSize(); ++i)
  {
    wdl_xml_element *elem2=elem->elements.Get(i);
    if (!strcmp(elem2->name, name)) return elem2->value.Get(); // may be ""
  }
  return NULL; // element does not exist
}

bool IsXMPResourceList(wdl_xml_element *elem)
{
  if (!strcmp(elem->name, "rdf:li"))
  {
    const char *val=elem->get_attribute("rdf:parseType");
    if (val && !strcmp(val, "Resource")) return true;
  }
  return false;
}

// keep in sync with metadata.cpp:XMP_MARKER_RESOLUTION
#define XMP_MARKER_RESOLUTION 1000000

// todo generic PopulateCuesFromMetadata function metadata => list of REAPERCues

void UnpackXMPTrack(wdl_xml_element *elem, WDL_StringKeyedArray<char*> *metadata)
{
  int num_markers=0;
  WDL_FastString key, val;
  for (int i=0; i < elem->elements.GetSize(); ++i)
  {
    wdl_xml_element *elem2=elem->elements.Get(i);
    if (!strcmp(elem2->name, "rdf:Bag"))
    {
      for (int i2=0; i2 < elem2->elements.GetSize(); ++i2)
      {
        wdl_xml_element *elem3=elem2->elements.Get(i2);
        if (IsXMPResourceList(elem3))
        {
          const char *track_type=GetXMPSubElement(elem3, "xmpDM:trackType");
          const char *frame_rate=GetXMPSubElement(elem3, "xmpDM:frameRate");
          if (track_type && frame_rate && !strcmp(track_type, "Cue"))
          {
            double marker_resolution=0;
            if (frame_rate[0] == 'f') marker_resolution=atof(frame_rate+1);
            // todo parse other resolution types
            if (marker_resolution > 0.0)
            {
              double marker_convert=XMP_MARKER_RESOLUTION/marker_resolution;
              for (int i3=0; i3 < elem3->elements.GetSize(); ++i3)
              {
                wdl_xml_element *elem4=elem3->elements.Get(i3);
                if (!strcmp(elem4->name, "xmpDM:markers"))
                {
                  for (int i4=0; i4 < elem4->elements.GetSize(); ++i4)
                  {
                    wdl_xml_element *elem5=elem4->elements.Get(i4);
                    if (!strcmp(elem5->name, "rdf:Seq"))
                    {
                      for (int i5=0; i5 < elem5->elements.GetSize(); ++i5)
                      {
                        wdl_xml_element *elem6=elem5->elements.Get(i4);
                        const char *start_time=GetXMPSubElement(elem6, "xmpDM:startTime");
                        const char *duration=GetXMPSubElement(elem6, "xmpDM:duration");
                        const char *marker_name=GetXMPSubElement(elem6, "xmpDM:name");
                        if (start_time)
                        {
                          double st=atof(start_time)*marker_convert;
                          double et = st + (duration ? atof(duration)*marker_convert : 0.0);
                          key.SetFormatted(512, "XMP:MARK%03d", num_markers++);
                          val.SetFormatted(512, "%.0f:%.0f:", st, et);
                          if (marker_name && marker_name[0]) val.Append(marker_name);
                          else val.AppendFormatted(512, "Marker %d", num_markers);
                          metadata->Insert(key.Get(), strdup(val.Get()));
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void UnpackXMPDescription(const char *curkey, wdl_xml_element *elem,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!strcmp(elem->name, "xmpDM:Tracks"))
  {
    // xmp "tracks" are collections of markers and other related data,
    // we can parse the markers (at least ones we wronte) but otherwise
    // we can ignore this entire block
    UnpackXMPTrack(elem, metadata);
    return;
  }

  if (!strcmp(elem->name, "xmpDM:relativeTimestamp"))
  {
    double tval=UnpackXMPTimestamp(elem);
    if (tval >= 0.0)
    {
      char buf[512];
      snprintf(buf, sizeof(buf), "%.0f", floor(tval*1000.0));
      metadata->Insert("XMP:dm/relativeTimestamp", strdup(buf));
    }
    return;
  }

  WDL_FastString key;
  int i;
  for (i=0; i < elem->attributes.GetSize(); ++i)
  {
    char *attr;
    const char *val=elem->attributes.Enumerate(i, &attr);
    if (IsXMPMetadata(attr, &key) && val && val[0])
    {
      metadata->Insert(key.Get(), strdup(val));
    }
  }

  if (IsXMPMetadata(elem->name, &key)) curkey=key.Get();
  if (curkey && elem->value.Get()[0])
  {
    InsertMetadataIncrKeyIfNeeded(metadata, curkey, elem->value.Get());
  }

  for (i=0; i < elem->elements.GetSize(); ++i)
  {
    wdl_xml_element *elem2=elem->elements.Get(i);
    UnpackXMPDescription(curkey, elem2, metadata);
  }
}

void UnpackXMPElement(wdl_xml_element *elem, WDL_StringKeyedArray<char*> *metadata)
{
  if (!strcmp(elem->name, "rdf:Description"))
  {
    // everything we care about is in this block
    UnpackXMPDescription(NULL, elem, metadata);
    return;
  }

  for (int i=0; i < elem->elements.GetSize(); ++i)
  {
    wdl_xml_element *elem2=elem->elements.Get(i);
    UnpackXMPElement(elem2, metadata);
  }
}

bool UnpackXMPChunk(const char *buf, int buflen,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!buf || !buflen || !metadata) return false;

  wdl_xml_parser xmp(buf, buflen);
  if (!xmp.parse() && xmp.element_root)
  {
    UnpackXMPElement(xmp.element_root, metadata);
    return true;
  }

  return false;
}


// metadata is passed as an assocarray of id=>value strings,
// where id has the form "scheme:identifier"
// example "ID3:TIT2", "INFO:IPRD", "VORBIS:ALBUM", etc
// for user-defined metadata, the form is extended to "scheme:identifier:key",
// example "ID3:TXXX:mykey", "VORBIS:USER:mykey", etc
// id passed to this function is just "identifier:key" part
// NOTE: WDL/lameencdec and WDL/vorbisencdec have copies of this, so edit them if this changes
bool ParseUserDefMetadata(const char *id, const char *val,
  const char **k, const char **v, int *klen, int *vlen)
{
  const char *sep=strchr(id, ':');
  if (sep) // key encoded in id, version >= 6.12
  {
    *k=sep+1;
    *klen=strlen(*k);
    *v=val;
    *vlen=strlen(*v);
    return true;
  }

  sep=strchr(val, '=');
  if (sep) // key encoded in value, version <= 6.11
  {
    *k=val;
    *klen=sep-val;
    *v=sep+1;
    *vlen=strlen(*v);
    return true;
  }

  // no key, version <= 6.11
  *k="User";
  *klen=strlen(*k);
  *v=val;
  *vlen=strlen(*v);
  return false;
}


bool HasScheme(const char *scheme, WDL_StringKeyedArray<char*> *metadata)
{
  if (!scheme || !scheme[0] || !metadata) return false;

  bool ismatch=false;
  int idx=metadata->LowerBound(scheme, &ismatch);
  const char *key=NULL;
  metadata->Enumerate(idx, &key);
  if (key && !strnicmp(key, scheme, strlen(scheme))) return true;
  return false;
}


WDL_INT64 _ATOI64(const char *str)
{
  bool neg=false;
  if (*str == '-') { neg=true; str++; }
  WDL_INT64 v=0;
  while (*str >= '0' && *str <= '9')
  {
    v = (v*10) + (WDL_INT64) (neg ? -(*str - '0') : (*str - '0'));
    str++;
  }
  return v;
}


int PackIXMLChunk(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata, int padtolen)
{
  if (!hb || !metadata) return 0;

  if (!HasScheme("IXML", metadata) &&
    !HasScheme("ASWG", metadata) &&
    !HasScheme("BWF", metadata))
  {
    return 0;
  }

  int olen=hb->GetSize();

  WDL_FastString ixml;
  const char *ixml_open="<?xml version=\"1.0\" encoding=\"UTF-8\"?><BWFXML>";
  const char *need_close=NULL;
  int junklen=0;

  for (int i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (!key || !key[0] || !val || !val[0]) continue;

    const char *sec =
      !strncmp(key, "ASWG:", 5) ? "ASWG" :
      !strncmp(key, "BWF:", 4) ? "BWF" :
      !strncmp(key, "IXML:USER:", 10) ? "USER" :
      !strncmp(key, "IXML:", 5) ? "IXML" :
      NULL;
    if (!sec) continue;

    key += strlen(sec)+1;
    if (!strcmp(sec, "BWF"))
    {
      if (!strcmp(key, "Description")) key="BWF_DESCRIPTION";
      else if (!strcmp(key, "Originator")) key="BWF_ORIGINATOR";
      else if (!strcmp(key, "OriginatorReference")) key="BWF_ORIGINATOR_REFERENCE";
      else if (!strcmp(key, "OriginationDate")) key="BWF_ORIGINATION_DATE";
      else if (!strcmp(key, "OriginationTime")) key="BWF_ORIGINATION_TIME";
      else if (!strcmp(key, "TimeReference")) key="BWF_TIME_REFERENCE";
      else if (!strcmp(key, "Version")) key="BWF_VERSION";
      else if (!strcmp(key, "LoudnessValue")) key="BWF_LOUDNESS_VALUE";
      else if (!strcmp(key, "LoudnessRange")) key="BWF_LOUDNESS_RANGE";
      else if (!strcmp(key, "MaxTruePeakLevel")) key="BWF_MAX_TRUE_PEAK_LEVEL";
      else if (!strcmp(key, "MaxMomentaryLoudness")) key="BWF_MAX_MOMENTARY_LOUDNESS";
      else if (!strcmp(key, "MaxShortTermLoudness")) key="BWF_MAX_SHORT_TERM_LOUDNESS";
      else continue;
    }

    if (!ixml.GetLength()) ixml.Append(ixml_open);

    if (need_close && strcmp(need_close, sec))
    {
      ixml.AppendFormatted(512, "</%s>", need_close);
      need_close=NULL;
    }
    if (!need_close && strcmp(sec, "IXML"))
    {
      ixml.AppendFormatted(512, "<%s>", sec);
      need_close=sec;
    }

    if (!strcmp(key, "BWF_TIME_REFERENCE"))
    {
      WDL_UINT64 pos=_ATOI64(val);
      int hi=pos>>32, lo=(pos&0xFFFFFFFF);
      ixml.AppendFormatted(4096, "<%s_HIGH>%d</%s_HIGH>", key, hi, key);
      ixml.AppendFormatted(4096, "<%s_LOW>%d</%s_LOW>", key, lo, key);
      continue;
    }

    if (!strcmp(sec, "USER"))
    {
      const char *k, *v;
      int klen, vlen;
      ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
      key=k;
      val=v;
    }

    if (!strncmp(val, "#junk#", 6))
    {
      junklen += 11+2*strlen(key)+strlen(val);
      continue;
    }

    ixml.Append("<");
    XMLCompliantAppend(&ixml, key, false);
    ixml.Append(">");
    XMLCompliantAppend(&ixml, val, true);
    ixml.Append("</");
    XMLCompliantAppend(&ixml, key, false);
    ixml.Append(">");
  }
  if (need_close)
  {
    ixml.AppendFormatted(512, "</%s>", need_close);
  }

  if (ixml.GetLength())
  {
    ixml.Append("</BWFXML>");
    int ixmllen=ixml.GetLength();
    int len=ixmllen+1+junklen;
    if (len < padtolen) len=padtolen;
    if (len&1) ++len;
    unsigned char *p=(unsigned char*)hb->ResizeOK(olen+len);
    if (WDL_NORMALLY(p != NULL))
    {
      memcpy(p+olen, ixml.Get(), ixmllen);
      memset(p+olen+ixmllen, 0, len-ixmllen);
    }
  }

  return hb->GetSize()-olen;
}


int PackXMPChunk(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata)
{
  if (!hb || !metadata) return 0;

  if (!HasScheme("XMP", metadata)) return 0;

  int olen=hb->GetSize();

  static const char *xmp_hdr=
    "<?xpacket begin=\"\xEF\xBB\xBF\" id=\"W5M0MpCehiHzreSzNTczkc9d\"?>"
    "<x:xmpmeta xmlns:x=\"adobe:ns:meta/\">"
      "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\">"
        "<rdf:Description"
        " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
        " xmlns:xmpDM=\"http://ns.adobe.com/xmp/1.0/DynamicMedia/\""; // unclosed
  static const char *xmp_ftr=
        "</rdf:Description>"
      "</rdf:RDF>"
    "</x:xmpmeta>"
    "<?xpacket end=\"w\"?>";

  WDL_FastString xmp(xmp_hdr);

  for (int pass=0; pass < 2; ++pass) // attributes, then elements
  {
    if (pass) xmp.Append(">");
    for (int i=0; i < metadata->GetSize(); ++i)
    {
      const char *key;
      const char *val=metadata->Enumerate(i, &key);
      if (!key || !key[0] || !val || !val[0]) continue;

      if (!strncmp(key, "XMP:", 4) && key[4])
      {
        key += 4;
        const char *prefix = // xmp schema
          !strncmp(key, "dc/", 3) ? "dc" :
          !strncmp(key, "dm/", 3) ? "xmpDM" : NULL;
        if (prefix && key[3])
        {
          if (!strcmp(key, "dm/markers")) continue;

          if (!strcmp(key, "dc/description") || !strcmp(key, "dc/title"))
          {
            // elements
            if (!pass) continue;

            key += 3;
            const char *lang=metadata->Get("XMP:dc/language");
            if (!lang) lang="x-default";
            xmp.AppendFormatted(1024, "<%s:%s>", prefix, key);
            xmp.AppendFormatted(1024, "<rdf:Alt><rdf:li xml:lang=\"%s\">", lang);
            xmp.Append(val);
            xmp.Append("</rdf:li></rdf:Alt>");
            xmp.AppendFormatted(1024, "</%s:%s>", prefix, key);
          }
          else if (!strcmp(key, "dm/relativeTimestamp"))
          {
            // element
            if (!pass) continue;

            key += 3;
            xmp.AppendFormatted(1024, "<%s:%s xmpDM:value=\"%s\" xmpDM:scale=\"1/1000\"/>",
              prefix, key, val);
          }
          else
          {
            // attributes
            if (pass) continue;

            key += 3;
            xmp.AppendFormatted(1024, " %s:%s=\"%s\"", prefix, key, val);
          }
        }
      }
    }
  }

  static const char *track_hdr1=
    "<xmpDM:Tracks>"
      "<rdf:Bag>"
        "<rdf:li rdf:parseType=\"Resource\">"
          "<xmpDM:trackType>Cue</xmpDM:trackType>"
          "<xmpDM:frameRate>f";
  static const char *track_hdr2=
          "</xmpDM:frameRate>"
          "<xmpDM:markers>"
            "<rdf:Seq>";
  static const char *track_ftr=
            "</rdf:Seq>"
          "</xmpDM:markers>"
        "</rdf:li>"
      "</rdf:Bag>"
    "</xmpDM:Tracks>";

  char buf[128];
  int cnt=0;
  for (int i=0; i < 1000; ++i)
  {
    snprintf(buf, sizeof(buf), "XMP:MARK%03d", i);
    const char *val=metadata->Get(buf);
    if (!val || !val[0]) break;

    const char *sep1=strchr(val, ':');
    if (WDL_NOT_NORMALLY(!sep1)) break;
    double st=atof(val);
    double et=atof(sep1+1);
    if (WDL_NOT_NORMALLY(st < 0.0 || et < st)) break;
    const char *sep2=strchr(sep1+1, ':');
    const char *name = sep2 ? sep2+1 : ""; // might need to make this xml compliant?

    if (!cnt++) xmp.AppendFormatted(1024, "%s%d%s", track_hdr1, XMP_MARKER_RESOLUTION, track_hdr2);

    xmp.Append("<rdf:li rdf:parseType=\"Resource\">");
    xmp.AppendFormatted(1024, "<xmpDM:startTime>%.0f</xmpDM:startTime>", st);
    if (et > st) xmp.AppendFormatted(1024, "<xmpDM:duration>%.0f</xmpDM:duration>", et-st);
    if (name[0]) xmp.AppendFormatted(1024, "<xmpDM:name>%s</xmpDM:name>", name);
    xmp.Append("</rdf:li>");
  }
  if (cnt) xmp.Append(track_ftr);

  xmp.Append(xmp_ftr);

  int xmplen=xmp.GetLength();
  int len=xmplen+1;
  if (len&1) ++len;
  unsigned char *p=(unsigned char*)hb->ResizeOK(olen+len);
  if (WDL_NORMALLY(p != NULL))
  {
    memcpy(p+olen, xmp.Get(), xmplen);
    memset(p+olen+xmplen, 0, len-xmplen);
  }

  return hb->GetSize()-olen;
}

int PackVorbisFrame(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata, bool for_vorbis)
{
  if (!hb || !metadata) return 0;

  // for vorbis, we need an empty frame even if there's no metadata
  if (!for_vorbis && !HasScheme("VORBIS", metadata)) return 0;

  int olen=hb->GetSize();

  const char *vendor="REAPER";
  const int vendorlen=strlen(vendor);
  int framelen=4+vendorlen+4+for_vorbis;

  int i, tagcnt=0;
  for (i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (!key || !key[0] || !val || !val[0]) continue;
    if (!strncmp(key, "VORBIS:", 7) && key[7])
    {
      key += 7;
      const char *k=key, *v=val;
      int klen=strlen(k), vlen=strlen(v);
      if (!strncmp(key, "USER", 4))
      {
        ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
      }

      int taglen=4+klen+1+vlen;
      if (framelen+taglen >= 0xFFFFFF) break;
      framelen += taglen;
      ++tagcnt;
    }
  }

  unsigned char *buf=(unsigned char*)hb->ResizeOK(olen+framelen);
  if (WDL_NORMALLY(buf != NULL))
  {
    buf += olen;
    unsigned char *p=buf;
    memcpy(p, &vendorlen, 4);
    p += 4;
    memcpy(p, vendor, vendorlen);
    p += vendorlen;
    memcpy(p, &tagcnt, 4);
    p += 4;

    for (i=0; i < metadata->GetSize(); ++i)
    {
      const char *key;
      const char *val=metadata->Enumerate(i, &key);
      if (!key || !key[0] || !val || !val[0]) continue;
      if (!strncmp(key, "VORBIS:", 7) && key[7])
      {
        key += 7;
        const char *k=key, *v=val;
        int klen=strlen(k), vlen=strlen(v);
        if (!strncmp(key, "USER", 4))
        {
          ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
        }

        int taglen=klen+1+vlen;
        memcpy(p, &taglen, 4);
        p += 4;
        while (*k)
        {
          *p++ = (*k >= ' ' && *k <= '}' && *k != '=') ? *k : ' ';
          k++;
        }
        *p++='=';
        memcpy(p, v, vlen);
        p += vlen;

        if (!--tagcnt) break;
      }
    }

    if (for_vorbis) *p++=1; // framing bit

    if (WDL_NOT_NORMALLY(p-buf != framelen) || framelen > 0xFFFFFF)
    {
      hb->Resize(olen);
    }
  }

  return hb->GetSize()-olen;
}

bool UnpackVorbisFrame(unsigned char *frame, int framelen,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!frame || !framelen || !metadata) return 0;

  char *p=(char*)frame;

  int vendor_len=*(int*)p;
  if (4+vendor_len+4 > framelen) return false;

  p += 4+vendor_len;
  int tagcnt=*(int*)p;
  p += 4;

  WDL_String str;
  int pos=4+vendor_len+4;
  while (pos < framelen && tagcnt--)
  {
    int taglen=*(int*)p;
    p += 4;
    if (pos+taglen > framelen) return false;
    str.Set("VORBIS:");
    str.Append(p, taglen);
    p += taglen;
    const char *sep=strchr(str.Get(), '=');
    if (!sep) return false;
    *(char*)sep=0;
    metadata->Insert(str.Get(), strdup(sep+1));
  }

  return (pos == framelen && tagcnt == 0);
}


#define _AddInt32LE(i) \
  *p++=((i)&0xFF); \
  *p++=(((i)>>8)&0xFF); \
  *p++=(((i)>>16)&0xFF); \
  *p++=(((i)>>24)&0xFF);

#define _GetInt32LE(p) \
  (((p)[0])|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24))


int PackApeChunk(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata)
{
  if (!hb || !metadata) return false;

  if (!HasScheme("APE", metadata)) return false;

  int olen=hb->GetSize();

  int i, apelen=0, cnt=0;
  for (i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (strlen(key) < 5 || strncmp(key, "APE:", 4) || !val || !val[0]) continue;
    key += 4;
    if (!apelen) apelen=64; // includes header and footer
    if (!strncmp(key, "User Defined", 12))
    {
      const char *k, *v;
      int klen, vlen;
      ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
      apelen += 8+klen+1+vlen;
    }
    else
    {
      apelen += 8+strlen(key)+1+strlen(val);
    }
    ++cnt;
  }
  if (!apelen) return false;

  unsigned char *buf=(unsigned char*)hb->ResizeOK(olen+apelen);
  if (WDL_NORMALLY(buf != NULL))
  {
    buf += olen;
    unsigned char *p=buf;
    memcpy(p, "APETAGEX", 8);
    p += 8;
    _AddInt32LE(2000); // version
    _AddInt32LE(apelen-32); // includes footer but not header
    _AddInt32LE(cnt);
    _AddInt32LE((1<<31)|(1<<30)|(1<<29)); // tag contains header and footer, this is the header
    _AddInt32LE(0);
    _AddInt32LE(0);

    for (i=0; i < metadata->GetSize(); ++i)
    {
      const char *key;
      const char *val=metadata->Enumerate(i, &key);
      if (strlen(key) < 5 || strncmp(key, "APE:", 4) || !val || !val[0]) continue;
      key += 4;
      const char *k=key, *v=val;
      int klen, vlen;
      if (!strncmp(key, "User Defined", 12))
      {
        ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
      }
      else
      {
        klen=strlen(k);
        vlen=strlen(v);
      }
      _AddInt32LE(vlen);
      _AddInt32LE(0);
      while (klen--)
      {
        *p = (*k >= 0x20 && *k <= 0x7E ? *k : ' ');
        ++p;
        ++k;
      }
      *p++=0;
      memcpy(p, v, vlen);
      p += vlen;
    }

    memcpy(p, "APETAGEX", 8);
    p += 8;
    _AddInt32LE(2000); // version
    _AddInt32LE(apelen-32); // includes footer but not header
    _AddInt32LE(cnt);
    _AddInt32LE((1<<31)|(1<<30)|(1<<28)); // tag contains header and footer, this is the footer
    _AddInt32LE(0);
    _AddInt32LE(0);

    if (WDL_NOT_NORMALLY(p-buf != apelen)) hb->Resize(olen);
  }

  return hb->GetSize()-olen;
}


const char *EnumMetadataSchemeFromFileType(const char *filetype, int idx)
{
  if (!filetype || !filetype[0]) return NULL;

  if (filetype[0] == '.') ++filetype;
  if (!stricmp(filetype, "bwf")) filetype="wav";
  else if (!stricmp(filetype, "opus")) filetype="ogg";
  else if (!stricmp(filetype, "aiff")) filetype="aif";
  else if (!stricmp(filetype, "caff")) filetype="caf";

  static const char *WAV_SCHEMES[]=
  {
    "BWF", "INFO", "IXML", "ASWG", "XMP", "AXML", "CART", "ID3",
  };
  static const char *MP3_SCHEMES[]=
  {
    "ID3", "APE", "IXML", "ASWG", "XMP",
  };
  static const char *FLAC_SCHEMES[]=
  {
    "VORBIS", "BWF", "IXML", "ASWG", "XMP",
  };
  static const char *OGG_SCHEMES[]=
  {
    "VORBIS",
  };
  static const char *WV_SCHEMES[]=
  {
    "BWF", "APE",
  };
  static const char *AIF_SCHEMES[]=
  {
    "IFF", "XMP", "ID3"
  };
  static const char *CAF_SCHEMES[]=
  {
    "CAFINFO",
  };
  static const char *RX2_SCHEMES[]=
  {
    "REX",
  };

#define DO_SCHEME_MAP(X) if (!stricmp(filetype, #X)) \
  return idx < sizeof(X##_SCHEMES)/sizeof(X##_SCHEMES[0]) ? X##_SCHEMES[idx] : NULL;

  DO_SCHEME_MAP(WAV);
  DO_SCHEME_MAP(MP3);
  DO_SCHEME_MAP(FLAC);
  DO_SCHEME_MAP(OGG);
  DO_SCHEME_MAP(WV);
  DO_SCHEME_MAP(AIF);
  DO_SCHEME_MAP(CAF);
  DO_SCHEME_MAP(RX2);

#undef DO_SCHEME_MAP

  return NULL;
}


bool EnumMetadataKeyFromMexKey(const char *mexkey, int idx, char *key, int keylen)
{
  if (!mexkey || !mexkey[0] || idx < 0 || !key || !keylen) return false;

  // TO_DO_IF_METADATA_UPDATE
  // "TITLE", "ARTIST", "ALBUM", "YEAR", "GENRE", "COMMENT", "DESC", "BPM", "KEY", "DB_CUSTOM", "TRACKNUMBER"

  if (!strcmp(mexkey, "DATE")) mexkey="YEAR";
  // callers handle PREFPOS

  // general priority order here:
  // BWF
  // INFO
  // ID3
  // APE
  // VORBIS
  // CART
  // IXML
  // ASWG
  // XMP
  // CAFINFO
  // IFF
  // REX

  static const char *TITLE_KEYS[]=
  {
    "INFO:INAM",
    "ID3:TIT2",
    "APE:Title",
    "VORBIS:TITLE",
    "CART:Title",
    "IXML:PROJECT",
    "ASWG:project",
    "XMP:dc/title",
    "CAFINFO:title",
    "IFF:NAME",
    "REX:Name",
  };
  static const char *ARTIST_KEYS[]=
  {
    "INFO:IART",
    "ID3:TPE1",
    "APE:Artist",
    "VORBIS:ARTIST",
    "CART:Artist",
    "XMP:dm/artist",
    "CAFINFO:artist",
    "IFF:AUTH",
  };
  static const char *ALBUM_KEYS[]=
  {
    "INFO:IALB",
    "INFO:IPRD",
    "ID3:TALB",
    "APE:Album",
    "VORBIS:ALBUM",
    "XMP:dm/album",
    "CAFINFO:album",
  };
  static const char *YEAR_KEYS[]= // really DATE
  {
    "BWF:OriginationDate",
    "INFO:ICRD",
    "ID3:TYER",
    "ID3:TDRC",
    "APE:Year",
    "APE:Record Date",
    "VORBIS:DATE",
    "CART:StartDate",
    "XMP:dc/date",
    "CAFINFO:year",
  };
  static const char *GENRE_KEYS[]=
  {
    "INFO:IGNR",
    "ID3:TCON",
    "APE:Genre",
    "VORBIS:GENRE",
    "CART:Category",
    "XMP:dm/genre",
    "CAFINFO:genre",
  };
  static const char *COMMENT_KEYS[]=
  {
    "INFO:ICMT",
    "ID3:COMM",
    "APE:Comment",
    "VORBIS:COMMENT",
    "CART:TagText",
    "IXML:NOTE",
    "ASWG:notes",
    "XMP:dm/logComment",
    "CAFINFO:comments",
    "CAFINFO:comment", // spec is "comments" but ffmpeg may write "comment"
    "REX:FreeText",
  };
  static const char *DESC_KEYS[]=
  {
    "BWF:Description",
    "INFO:ISBJ",
    "INFO:IKEY",
    "ID3:TIT3",
    "APE:Subtitle",
    "VORBIS:DESCRIPTION",
    "XMP:dc/description",
    "IFF:ANNO",
  };
  static const char *BPM_KEYS[]=
  {
    "ACID:BPM",
    "ID3:TBPM",
    "APE:BPM",
    "VORBIS:BPM",
    "XMP:dm/tempo",
    "CAFINFO:tempo",
  };
  static const char *KEY_KEYS[]=
  {
    "ACID:KEY",
    "ID3:TKEY",
    "APE:Key",
    "VORBIS:KEY",
    "XMP:dm/key",
    "CAFINFO:key signature",
  };
  static const char *TRACKNUMBER_KEYS[]=
  {
    "INFO:TRCK",
    "ID3:TRCK",
    "APE:Track",
    "VORBIS:TRACKNUMBER",
    "CART:CutID",
    // "IXML:TRACK",
    "XMP:dm/trackNumber",
    "CAFINFO:track number",
  };

#define DO_MEXKEY_MAP(K) \
if (!strcmp(mexkey, #K)) \
{ \
  if (idx >= sizeof(K##_KEYS)/sizeof(K##_KEYS[0])) return false; \
  lstrcpyn(key, K##_KEYS[idx], keylen); return true; \
}

  key[0]=0;
  DO_MEXKEY_MAP(TITLE);
  DO_MEXKEY_MAP(ARTIST);
  DO_MEXKEY_MAP(ALBUM);
  DO_MEXKEY_MAP(TRACKNUMBER);
  DO_MEXKEY_MAP(YEAR);
  DO_MEXKEY_MAP(GENRE);
  DO_MEXKEY_MAP(COMMENT);
  DO_MEXKEY_MAP(DESC);
  DO_MEXKEY_MAP(BPM);
  DO_MEXKEY_MAP(KEY);

#undef DO_MEXKEY_MAP

  static const char *DB_CUSTOM_KEYS[]=
  {
    "ID3:TXXX",
    "APE",
    "VORBIS",
    "IXML:USER",
  };
  if (idx >= sizeof(DB_CUSTOM_KEYS)/sizeof(DB_CUSTOM_KEYS[0])) return false;
  if (!strcmp(mexkey, "DB_CUSTOM")) mexkey="REAPER";
  snprintf(key, keylen, "%s:%s", DB_CUSTOM_KEYS[idx], mexkey);
  return true;
}

const char *GetMexKeyFromMetadataKey(const char *key)
{
  int i=0;
  const char *mexkey;
  while ((mexkey=EnumMexKeys(i++)))
  {
    int j=0;
    char tkey[256];
    while (EnumMetadataKeyFromMexKey(mexkey, j++, tkey, sizeof(tkey)) && tkey[0])
    {
      if (!strcmp(key, tkey)) return mexkey;
    }
  }
  return NULL;
}

bool HandleMexMetadataRequest(const char *mexkey, char *buf, int buflen,
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!mexkey || !mexkey[0] || !buf || !buflen || !metadata) return false;
  buf[0]=0;

  buf[0]=0;
  int i=0;
  char key[256];
  while (EnumMetadataKeyFromMexKey(mexkey, i++, key, sizeof(key)) && key[0])
  {
    const char *val=metadata->Get(key);
    if (val && val[0])
    {
      lstrcpyn(buf, val, buflen);
      return true;
    }
  }

  if (strchr(mexkey, ':'))
  {
    const char *val=metadata->Get(mexkey);
    if (val && val[0])
    {
      lstrcpyn(buf, val, buflen);
      return true;
    }
  }

  return false;
}


void WriteMetadataPrefPos(double prefpos, int srate,  // prefpos <= 0.0 to clear
  WDL_StringKeyedArray<char*> *metadata)
{
  if (!metadata) return;

  metadata->Delete("BWF:TimeReference");
  metadata->Delete("ID3:TXXX:TIME_REFERENCE");
  metadata->Delete("IXML:BEXT:BWF_TIME_REFERENCE_HIGH");
  metadata->Delete("IXML:BEXT:BWF_TIME_REFERENCE_LOW");
  metadata->Delete("XMP:dm/relativeTimestamp");
  metadata->Delete("VORBIS:TIME_REFERENCE");

  if (prefpos > 0.0 && srate > 1)
  {
    char buf[128];
    if (srate > 0.0)
    {
      snprintf(buf, sizeof(buf), "%.0f", floor(prefpos*(double)srate));
      metadata->Insert("BWF:TimeReference", strdup(buf));
      // BWF:TimeReference causes IXML:BEXT element to be written as well
      metadata->Insert("ID3:TXXX:TIME_REFERENCE", strdup(buf));
      metadata->Insert("VORBIS:TIME_REFERENCE", strdup(buf));
    }
    snprintf(buf, sizeof(buf), "%.0f", floor(prefpos*1000.0));
    metadata->Insert("XMP:dm/relativeTimestamp", strdup(buf));
  }
}

bool IsImageMetadata(const char *key)
{
  return !strncmp(key, "ID3:APIC", 8) || !strncmp(key, "FLACPIC:APIC", 12);
}

void DeleteAllImageMetadata(WDL_StringKeyedArray<char*> *metadata)
{
  for (int i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    metadata->Enumerate(i, &key);
    if (IsImageMetadata(key)) metadata->DeleteByIndex(i--);
  }
}

void AddMexMetadata(WDL_StringKeyedArray<char*> *mex_metadata,
  WDL_StringKeyedArray<char*> *metadata, int srate)
{
  if (!mex_metadata || !metadata) return;

  bool added_img_metadata=false;

  for (int idx=0; idx < mex_metadata->GetSize(); ++idx)
  {
    const char *mexkey;
    const char *val=mex_metadata->Enumerate(idx, &mexkey);

    if (!strcmp(mexkey, "PREFPOS"))
    {
      WDL_UINT64 ms = val && val[0] ? ParseUInt64(val) : 0;
      WriteMetadataPrefPos((double)ms/1000.0, srate, metadata);
      // caller may still have to do stuff if prefpos is represented
      // in some other way outside the metadata we handle, like wavpack
      continue;
    }

    if (IsImageMetadata(mexkey))
    {
      if (!added_img_metadata)
      {
        added_img_metadata=true;
        DeleteAllImageMetadata(metadata);
      }
      metadata->Insert(mexkey, strdup(val));
      continue;
    }

    int i=0;
    char key[256];
    while (EnumMetadataKeyFromMexKey(mexkey, i++, key, sizeof(key)) && key[0])
    {
      if (val && val[0]) metadata->Insert(key, strdup(val));
      else metadata->Delete(key);
    }
  }
}


void DumpMetadata(WDL_FastString *str, WDL_StringKeyedArray<char*> *metadata)
{
  if (!str || !metadata || !metadata->GetSize()) return;

  char scheme[256];
  scheme[0]=0;

  char buf[2048];
  int j=0;
  const char *mexkey, *mexdesc;
  while ((mexkey=EnumMexKeys(j++, &mexdesc)))
  {
    if (HandleMexMetadataRequest(mexkey, buf, sizeof(buf), metadata))
    {
      if (!scheme[0])
      {
        lstrcpyn(scheme, "mex", sizeof(scheme));
        str->Append("Metadata:\r\n");
      }
      str->AppendFormatted(4096, "    %s:%s%s\r\n",
        mexdesc, strchr(buf, '\n') ? "\r\n" : " ", buf);
    }
  }

  for (int i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (!key || !key[0] || !val || !val[0] || !strncmp(val, "[Binary data]", 13)) continue;

    const char *sep=strchr(key, ':');
    if (sep)
    {
      int slen=wdl_min(sep-key, sizeof(scheme)-1);
      if (strncmp(scheme, key, slen))
      {
        lstrcpyn(scheme, key, slen+1);
        str->AppendFormatted(256, "%s tags:\r\n", scheme);
      }
      key += slen+1;
    }
    str->AppendFormatted(4096, "    %s:%s%s\r\n",
      key, strchr(val, '\n') ? "\r\n" : " ", val);
  }

  int unk_cnt=0;
  for (int i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (key && key[0] && val && (!val[0] || !strncmp(val, "[Binary data]", 13)))
    {
      if (!unk_cnt++) str->Append("Other file sections:\r\n");
      str->AppendFormatted(4096, "    %s%s\r\n", key,
        !strnicmp(key, "smed", 4) ?
        " (proprietary Soundminer metadata)" : "");
    }
  }
}

void CopyMetadata(WDL_StringKeyedArray<char*> *src, WDL_StringKeyedArray<char*> *dest)
{
  if (!dest || !src) return;

  dest->DeleteAll(false);
  for (int i=0; i < src->GetSize(); ++i)
  {
    const char *key;
    const char *val=src->Enumerate(i, &key);
    dest->AddUnsorted(key, strdup(val));
  }
  dest->Resort(); // safe in case src/dest have diff sort attributes
}


bool CopyFileData(WDL_FileRead *fr, WDL_FileWrite *fw, WDL_INT64 len)
{
  while (len)
  {
    char tmp[32768];
    const int amt = (int) (wdl_min(len,sizeof(tmp)));
    const int rd = fr->Read(tmp, amt);
    if (rd != amt) return false;
    if (fw->Write(tmp, rd) != rd) return false;
    len -= rd;
  }
  return true;
}


bool EnumVorbisChapters(WDL_StringKeyedArray<char*> *metadata, int idx,
  double *pos, const char **name)
{
  if (!metadata) return false;

  int cnt=0;
  bool ismatch=false;
  const char *prev=NULL;
  int i=metadata->LowerBound("VORBIS:CHAPTER", &ismatch);
  for (; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (strncmp(key, "VORBIS:CHAPTER", 14)) return false;
    if (!prev || strncmp(key, prev, 17))
    {
      prev=key;
      if (idx == cnt)
      {
        if (!key[17] && val && val[0])
        {
          // VORBIS:CHAPTER001 => 00:00:00.000
          if (pos)
          {
            int hh=0, mm=0, ss=0, ms=0;
            if (sscanf(val, "%d:%d:%d.%d", &hh, &mm, &ss, &ms) == 4)
            {
              *pos=(double)hh*3600.0+(double)mm*60.0+(double)ss+(double)ms*0.001;
            }
          }
          if (name)
          {
            val=metadata->Enumerate(i+1, &key);
            if (!strncmp(key, prev, 17) && !strcmp(key+17, "NAME"))
            {
              // VORBIS:CHAPTER001NAME => chapter name
              *name=val;
            }
          }
          return true;
        }
        return false;
      }
      ++cnt;
    }
  }
  return false;
}


#define _AddSyncSafeInt32(i) \
*p++=(((i)>>21)&0x7F); \
*p++=(((i)>>14)&0x7F); \
*p++=(((i)>>7)&0x7F); \
*p++=((i)&0x7F);

#define _AddInt32(i) \
*p++=(((i)>>24)&0xFF); \
*p++=(((i)>>16)&0xFF); \
*p++=(((i)>>8)&0xFF); \
*p++=((i)&0xFF);

#define _GetSyncSafeInt32(p) \
(((p)[0]<<21)|((p)[1]<<14)|((p)[2]<<7)|((p)[3]))

void WriteSyncSafeInt32(WDL_FileWrite *fw, int i)
{
  unsigned char buf[4];
  unsigned char *p=buf;
  _AddSyncSafeInt32(i);
  fw->Write(buf, 4);
}


#define CTOC_NAME "TOC" // arbitrary name of table of contents element


static bool _isnum(const char *v, int pos, int len)
{
  for (int i=pos; i < pos+len; ++i)
  {
    if (v[i] < '0' || v[i] > '9') return false;
  }
  return true;
}

int IsID3TimeVal(const char *v)
{
  if (strlen(v) == 4 && _isnum(v, 0, 4)) return 1;
  if (strlen(v) == 5 && _isnum(v, 0, 2) && _isnum(v, 3, 2)) return 2;
  return 0;
}

struct ID3RawTag
{
  char key[8]; // we only use 4 chars + nul
  WDL_HeapBuf val; // includes everything after taglen (flags, etc)
};

int ReadID3Raw(WDL_FileRead *fr, WDL_PtrList<ID3RawTag> *rawtags)
{
  if (!fr || !fr->IsOpen() || !rawtags) return 0;

  unsigned char buf[16];
  if (fr->Read(buf, 10) != 10) return 0;
  if (memcmp(buf, "ID3\x04", 4) && memcmp(buf, "ID3\x03", 4)) return 0;
  int id3len=_GetSyncSafeInt32(buf+6);
  if (!id3len) return 0;

  int rdlen=0;
  WDL_HeapBuf hb;
  while (rdlen < id3len)
  {
    if (fr->Read(buf, 8) != 8) return 0;
    if (!buf[0]) return 10+id3len; // padding
    if ((buf[0] < 'A' || buf[0] > 'Z') && (buf[0] < '0' || buf[0] > '9')) return 0; // unexpected

    int taglen=_GetSyncSafeInt32(buf+4)+2; // include flags in taglen

    unsigned char *p=(unsigned char*)hb.ResizeOK(taglen);
    if (!p || fr->Read(p, taglen) != taglen) return 0;

    ID3RawTag *rawtag=rawtags->Add(new ID3RawTag);
    memcpy(rawtag->key, buf, 4);
    rawtag->key[4]=0;
    rawtag->val=hb;

    rdlen += 8+taglen;
    if (rdlen == id3len) return 10+id3len;
  }
  return 0;
}

void DeleteID3Raw(WDL_PtrList<ID3RawTag> *rawtags, const char *key)
{
  if (!rawtags || !rawtags->GetSize()) return;
  if (strncmp(key, "ID3:", 4)) return;
  if (WDL_NOT_NORMALLY(strlen(key) < 8)) return;

  key += 4;
  const char *subkey=NULL;
  int suboffs=0, sublen=0;
  if (key[4] && strncmp(key, "APIC", 4))
  {
    if (!strncmp(key, "TXXX:", 5)) suboffs=3;
    else if (!strncmp(key, "PRIV:", 5)) suboffs=2;
    if (!suboffs || !key[5]) return;
    subkey=key+5;
    sublen=strlen(subkey);
  }

  for (int i=0; i < rawtags->GetSize(); ++i)
  {
    ID3RawTag *rawtag=rawtags->Get(i);
    if (!strncmp(key, rawtag->key, 4))
    {
      if (subkey &&
        (rawtag->val.GetSize() < sublen+suboffs ||
         memcmp((unsigned char*)rawtag->val.Get()+suboffs, subkey, sublen)))
      {
        continue; // key is like ID3:AAAA:BBBB but rawtag->val does not match *BBBB
      }
      rawtags->Delete(i--, true);
    }
  }
}

int PackID3Chunk(WDL_HeapBuf *hb, WDL_StringKeyedArray<char*> *metadata,
  bool want_embed_otherschemes, int *ixml_lenwritten, int ixml_padtolen,
  WDL_PtrList<ID3RawTag> *rawtags)
{
  if (!hb || !metadata) return false;

  bool want_ixml = want_embed_otherschemes &&
    (HasScheme("IXML", metadata) || HasScheme("ASWG", metadata) || HasScheme("BWF", metadata));
  bool want_xmp = want_embed_otherschemes && HasScheme("XMP", metadata);
  if (!HasScheme("ID3", metadata) && !want_ixml && !want_xmp) return false;

  int olen=hb->GetSize();

  int id3len=0, chapcnt=0;
  WDL_TypedQueue<char> toc;
  int i;
  for (i=0; i < metadata->GetSize(); ++i)
  {
    const char *key;
    const char *val=metadata->Enumerate(i, &key);
    if (strlen(key) < 8 || strncmp(key, "ID3:", 4) || !val) continue;
    key += 4;
    if (!strncmp(key, "TXXX", 4))
    {
      const char *k, *v;
      int klen, vlen;
      ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
      id3len += 10+1+klen+1+vlen;
    }
    else if (!strncmp(key, "TIME", 4))
    {
      if (IsID3TimeVal(val)) id3len += 10+1+4;
    }
    else if (key[0] == 'T' && strlen(key) == 4)
    {
      id3len += 10+1+strlen(val);
    }
    else if (!strcmp(key, "COMM") || !strcmp(key, "USLT"))
    {
      id3len += 10+5+strlen(val);
    }
    else if (!strncmp(key, "CHAP", 4) && chapcnt < 255)
    {
      const char *c1=strchr(val, ':');
      const char *c2 = c1 ? strchr(c1+1, ':') : NULL;
      if (c1)
      {
        ++chapcnt;
        const char *toc_entry=key; // use "CHAP001", etc as the internal toc entry
        const char *chap_name = c2 ? c2+1 : NULL;
        toc.Add(toc_entry, strlen(toc_entry)+1);
        id3len += 10+strlen(toc_entry)+1+16;
        if (chap_name) id3len += 10+1+strlen(chap_name)+1;
      }
    }
  }
  if (chapcnt)
  {
    id3len += 10+strlen(CTOC_NAME)+1+2+toc.GetSize();
  }

  WDL_HeapBuf apic_hdr;
  int apic_datalen=0;
  const char *apic_fn=metadata->Get("ID3:APIC_FILE");
  if (apic_fn && apic_fn[0])
  {
    const char *mime=NULL;
    const char *ext=WDL_get_fileext(apic_fn);
    if (ext && (!stricmp(ext, ".jpg") || !stricmp(ext, ".jpeg"))) mime="image/jpeg";
    else if (ext && !stricmp(ext, ".png")) mime="image/png";
    if (mime)
    {
      FILE *fp=fopenUTF8(apic_fn, "rb"); // could stat but let's make sure we can open the file
      if (fp)
      {
        fseek(fp, 0, SEEK_END);
        apic_datalen=ftell(fp);
        fclose(fp);
      }
    }
    if (apic_datalen)
    {
      const char *t=metadata->Get("ID3:APIC_TYPE");
      int type=-1;
      if (t && t[0] >= '0' && t[0] <= '9') type=atoi(t);
      if (type < 0 || type >= 16) type=3; // default "Cover (front)"

      const char *desc=metadata->Get("ID3:APIC_DESC");
      if (!desc) desc="";
      int desclen=wdl_min(strlen(desc), 63);

      int apic_hdrlen=1+strlen(mime)+1+1+desclen+1;
      char *p=(char*)apic_hdr.ResizeOK(apic_hdrlen);
      if (p)
      {
        *p++=3; // UTF-8
        memcpy(p, mime, strlen(mime)+1);
        p += strlen(mime)+1;
        *p++=type;
        memcpy(p, desc, desclen);
        p += desclen;
        *p++=0;
        id3len += 10+apic_hdrlen+apic_datalen;
      }
    }
  }

  WDL_HeapBuf ixml;
  if (want_ixml)
  {
    PackIXMLChunk(&ixml, metadata, ixml_padtolen);
    if (ixml.GetSize())
    {
      if (ixml_lenwritten) *ixml_lenwritten=ixml.GetSize();
      id3len += 10+5+ixml.GetSize();
    }
  }

  WDL_HeapBuf xmp;
  if (want_xmp)
  {
    PackXMPChunk(&xmp, metadata);
    if (xmp.GetSize()) id3len += 10+4+xmp.GetSize();
  }

  if (rawtags)
  {
    for (int i=0; i < rawtags->GetSize(); ++i)
    {
      ID3RawTag *rawtag=rawtags->Get(i);
      if (WDL_NORMALLY(rawtag && rawtag->key[0] && rawtag->val.GetSize()))
      {
        id3len += 8+rawtag->val.GetSize();
      }
    }
  }

  if (id3len)
  {
    id3len += 10;
    unsigned char *buf=(unsigned char*)hb->ResizeOK(olen+id3len);
    if (WDL_NORMALLY(buf != NULL))
    {
      buf += olen;
      chapcnt=0;
      unsigned char *p=buf;
      memcpy(p,"ID3\x04\x00\x00", 6);
      p += 6;
      _AddSyncSafeInt32(id3len-10);
      for (i=0; i < metadata->GetSize(); ++i)
      {
        const char *key;
        const char *val=metadata->Enumerate(i, &key);
        if (strlen(key) < 8 || strncmp(key, "ID3:", 4) || !val) continue;
        key += 4;
        if (!strncmp(key, "TXXX", 4))
        {
          memcpy(p, key, 4);
          p += 4;
          const char *k, *v;
          int klen, vlen;
          ParseUserDefMetadata(key, val, &k, &v, &klen, &vlen);
          _AddSyncSafeInt32(1+klen+1+vlen);
          memcpy(p, "\x00\x00\x03", 3); // UTF-8
          p += 3;
          memcpy(p, k, klen);
          p += klen;
          *p++=0;
          memcpy(p, v, vlen);
          p += vlen;
        }
        else if (!strncmp(key, "TIME", 4))
        {
          int tv=IsID3TimeVal(val);
          if (tv)
          {
            memcpy(p, key, 4);
            p += 4;
            _AddSyncSafeInt32(1+4);
            memcpy(p, "\x00\x00\x03", 3); // UTF-8
            p += 3;
            memcpy(p, val, 2);
            if (tv == 1) memcpy(p+2, val+2, 2);
            else memcpy(p+2, val+3, 2);
            p += 4;
          }
        }
        else if (key[0] == 'T' && strlen(key) == 4)
        {
          memcpy(p, key, 4);
          p += 4;
          int len=strlen(val);
          _AddSyncSafeInt32(1+len);
          memcpy(p, "\x00\x00\x03", 3); // UTF-8
          p += 3;
          memcpy(p, val, len);
          p += len;
        }
        else if (!strcmp(key, "COMM") || !strcmp(key, "USLT"))
        {
          // http://www.loc.gov/standards/iso639-2/php/code_list.php
          // most apps ignore this, itunes wants "eng" or something locale-specific
          const char *lang=NULL;
          if (!strcmp(key, "USLT")) lang=metadata->Get("ID3:LYRIC_LANG");
          if (!lang) lang=metadata->Get("ID3:COMM_LANG");
          if (!lang) lang=metadata->Get("ID3:COMMENT_LANG");

          memcpy(p, key, 4);
          p += 4;
          int len=strlen(val);
          _AddSyncSafeInt32(5+len);
          memcpy(p, "\x00\x00\x03", 3); // UTF-8
          p += 3;
          if (lang && strlen(lang) >= 3 &&
              tolower(*lang) >= 'a' && tolower(*lang) <= 'z')
          {
            *p++=tolower(*lang++);
            *p++=tolower(*lang++);
            *p++=tolower(*lang++);
            *p++=0;
          }
          else
          {
            // some apps write "XXX" for "no particular language"
            memcpy(p, "XXX\x00", 4);
            p += 4;
          }
          memcpy(p, val, len);
          p += len;
        }
        else if (!strncmp(key, "CHAP", 4) && chapcnt < 255)
        {
          const char *c1=strchr(val, ':');
          const char *c2 = c1 ? strchr(c1+1, ':') : NULL;
          if (c1)
          {
            // note, the encoding ignores the chapter number (CHAP001, etc)

            ++chapcnt;
            const char *toc_entry=key; // use "CHAP001", etc as the internal toc entry
            const char *chap_name = c2 ? c2+1 : NULL;
            int st=atoi(val);
            int et=atoi(c1+1);

            int framelen=strlen(toc_entry)+1+16;
            if (chap_name) framelen += 10+1+strlen(chap_name)+1;

            memcpy(p, "CHAP", 4);
            p += 4;
            _AddSyncSafeInt32(framelen);
            memset(p, 0, 2);
            p += 2;
            memcpy(p, toc_entry, strlen(toc_entry)+1);
            p += strlen(toc_entry)+1;
            _AddInt32(st);
            _AddInt32(et);
            memset(p, 0, 8);
            p += 8;

            if (chap_name)
            {
              int name_framelen=1+strlen(chap_name)+1;
              memcpy(p, "TIT2", 4);
              p += 4;
              _AddSyncSafeInt32(name_framelen);
              memcpy(p, "\x00\x00\x03", 3); // UTF-8
              p += 3;
              memcpy(p, chap_name, strlen(chap_name)+1);
              p += strlen(chap_name)+1;
            }
          }
        }
      }

      if (chapcnt)
      {
        int toc_framelen=strlen(CTOC_NAME)+1+2+toc.GetSize();
        memcpy(p, "CTOC", 4);
        p += 4;
        _AddSyncSafeInt32(toc_framelen);
        memset(p, 0, 2);
        p += 2;
        memcpy(p, CTOC_NAME, strlen(CTOC_NAME)+1);
        p += strlen(CTOC_NAME)+1;
        *p++=3; // CTOC flags: &1=top level, &2=ordered
        *p++=(chapcnt&0xFF);
        memcpy(p, toc.Get(), toc.GetSize());
        p += toc.GetSize();
      }

      if (apic_hdr.GetSize() && apic_datalen)
      {
        memcpy(p, "APIC", 4);
        p += 4;
        int len=apic_hdr.GetSize()+apic_datalen;
        _AddSyncSafeInt32(len);
        memcpy(p, "\x00\x00", 2);
        p += 2;
        memcpy(p, apic_hdr.Get(), apic_hdr.GetSize());
        p += apic_hdr.GetSize();
        FILE *fp=fopenUTF8(apic_fn, "rb");
        if (WDL_NORMALLY(fp))
        {
          fread(p, 1, apic_datalen, fp);
          fclose(fp);
        }
        else // uh oh
        {
          memset(p, 0, apic_datalen);
        }
        p += apic_datalen;
      }

      if (ixml.GetSize())
      {
        memcpy(p, "PRIV", 4);
        p += 4;
        int len=ixml.GetSize()+5;
        _AddSyncSafeInt32(len);
        memcpy(p, "\x00\x00", 2);
        p += 2;
        memcpy(p, "iXML\x00", 5);
        p += 5;
        memcpy(p, ixml.Get(), ixml.GetSize());
        p += ixml.GetSize();
      }

      if (xmp.GetSize())
      {
        memcpy(p, "PRIV", 4);
        p += 4;
        int len=xmp.GetSize()+4;
        _AddSyncSafeInt32(len);
        memcpy(p, "\x00\x00", 2);
        p += 2;
        memcpy(p, "XMP\x00", 4);
        p += 4;
        memcpy(p, xmp.Get(), xmp.GetSize());
        p += xmp.GetSize();
      }

      if (rawtags)
      {
        for (int i=0; i < rawtags->GetSize(); ++i)
        {
          ID3RawTag *rawtag=rawtags->Get(i);
          if (WDL_NORMALLY(rawtag && rawtag->key[0] && rawtag->val.GetSize()))
          {
            memcpy(p, rawtag->key, strlen(rawtag->key));
            p += strlen(rawtag->key);
            int vallen=rawtag->val.GetSize(); // includes flags
            _AddSyncSafeInt32(vallen-2);
            memcpy(p, rawtag->val.Get(), vallen);
            p += vallen;
          }
        }
      }

      if (WDL_NOT_NORMALLY(p-buf != id3len)) hb->Resize(olen);
    }
  }

  return hb->GetSize()-olen;
}

double ReadMetadataPrefPos(WDL_StringKeyedArray<char*> *metadata, double srate)
{
  if (!metadata) return -1.0;

  const char *v=metadata->Get("BWF:TimeReference");
  if (!v || !v[0]) v=metadata->Get("ID3:TXXX:TIME_REFERENCE");
  if (!v || !v[0]) v=metadata->Get("VORBIS:TIME_REFERENCE");
  if (v && v[0] && srate > 0.0)
  {
    WDL_UINT64 i=ParseUInt64(v);
    return (double)i/srate;
  }

  v=metadata->Get("IXML:BEXT:BWF_TIME_REFERENCE_LOW");
  if (v && v[0] && srate > 0.0)
  {
    WDL_UINT64 ipos=atoi(v);
    v=metadata->Get("IXML:BEXT:BWF_TIME_REFERENCE_HIGH");
    if (v && v[0]) ipos |= ((WDL_UINT64)atoi(v))<<32;
    return (double)ipos/srate;
  }

  v=metadata->Get("XMP:dm/relativeTimestamp");
  if (v && v[0])
  {
    WDL_UINT64 i=ParseUInt64(v);
    return (double)i/1000.0;
  }

  return -1.0;
}


// nch 0 means channel count agnostic
// nch -1 means high order ambisonic, nch must? be an integer squared, layout tag must be or'd with the number of channels
struct ChanLayout { const char *fmts; int nch; const char *desc; int chan_layout, chan_mask; };
static const ChanLayout CHAN_LAYOUTS[]=
{
  // using Ls/Rs for left side/right side
  // using Lb/Rb for left back/right back

  { "cw", 2, "L R",
    kAudioChannelLayoutTag_UseChannelBitmap,
    kAudioChannelBit_Left | kAudioChannelBit_Right },

  { "cw", 3, "L R C",
    kAudioChannelLayoutTag_UseChannelBitmap,
    kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_Center },

  { "cw", 4, "L R Lb Rb",
    kAudioChannelLayoutTag_UseChannelBitmap,
    kAudioChannelBit_Left | kAudioChannelBit_Right |
    kAudioChannelBit_LeftSurround | kAudioChannelBit_RightSurround },

  { "cw", 5, "L R C Lb Rb",
    kAudioChannelLayoutTag_UseChannelBitmap,
    kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_Center |
    kAudioChannelBit_LeftSurround | kAudioChannelBit_RightSurround },

  { "cw", 6, "L R C LFE Lb Rb",
    kAudioChannelLayoutTag_UseChannelBitmap,
    kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_Center |
    kAudioChannelBit_LFEScreen |
    kAudioChannelBit_LeftSurround | kAudioChannelBit_RightSurround },

  { "cw", 6, "L R Lb Rb Ls Rs",
    kAudioChannelLayoutTag_UseChannelBitmap,
    kAudioChannelBit_Left | kAudioChannelBit_Right |
    kAudioChannelBit_LeftSurround | kAudioChannelBit_RightSurround |
    kAudioChannelBit_LeftSurroundDirect | kAudioChannelBit_RightSurroundDirect },

  { "cw", 8, "L R C LFE Lb Rb Ls Rs",
    kAudioChannelLayoutTag_UseChannelBitmap,
    kAudioChannelBit_Left | kAudioChannelBit_Right | kAudioChannelBit_Center |
    kAudioChannelBit_LFEScreen |
    kAudioChannelBit_LeftSurround | kAudioChannelBit_RightSurround |
    kAudioChannelBit_LeftSurroundDirect | kAudioChannelBit_RightSurroundDirect },

  { "c", 2, "Mid-Side", kAudioChannelLayoutTag_MidSide, 0 },
  { "c", 2, "Binaural", kAudioChannelLayoutTag_Binaural, 0 },
  { "c", 4, "Ambisonic B-Format - W X Y Z", kAudioChannelLayoutTag_Ambisonic_B_Format, 0 },

  { "c", 6, "MPEG 5.1A - L R C LFE Lb Rb", kAudioChannelLayoutTag_MPEG_5_1_A, 0 },
  { "c", 6, "MPEG 5.1B - L R Lb Rb C LFE", kAudioChannelLayoutTag_MPEG_5_1_B, 0 },
  { "c", 6, "MPEG 5.1C - L C R Lb Rb LFE", kAudioChannelLayoutTag_MPEG_5_1_C, 0 },
  { "c", 6, "MPEG 5.1D - C L R Lb Rb LFE", kAudioChannelLayoutTag_MPEG_5_1_D, 0 },

  { "c", 8, "MPEG 7.1A - L R C LFE Lb Rb Lc Rc", kAudioChannelLayoutTag_MPEG_7_1_A, 0 },
  { "c", 8, "MPEG 7.1B - C Lc Rc L R Lb Rb LFE", kAudioChannelLayoutTag_MPEG_7_1_B, 0 },
  { "c", 8, "MPEG 7.1C (SMPTE 7.1) - L R C LFE Ls Rs Lb Rb", kAudioChannelLayoutTag_MPEG_7_1_C, 0 },

  { "c", 6, "ITU 3.2.1 - L R C LFE Lb Rb", kAudioChannelLayoutTag_ITU_3_2_1, 0 },
  { "c", 8, "ITU 3.4.1 - L R C LFE Lb Rb Rls Rrs", kAudioChannelLayoutTag_ITU_3_4_1, 0 },

  { "c", -1, "HO Ambisonic SN3D", kAudioChannelLayoutTag_HOA_ACN_SN3D, 0 },
  { "c", -1, "HO Ambisonic N3D", kAudioChannelLayoutTag_HOA_ACN_N3D, 0 },

  { "c", 8, "Atmos 5.1.2 - L R C LFE Lb Rb Ltm Rtm", kAudioChannelLayoutTag_Atmos_5_1_2, 0 },
  { "c", 12, "Atmos 7.1.4 - L R C LFE Lb Rb Rls Rrs Ltf Rtf Ltr Rtr", kAudioChannelLayoutTag_Atmos_7_1_4, 0 },
  { "c", 16, "Atmos 9.1.6 - L R C LFE Lb Rb Rls Rrs Lw Rw Ltf Rtf Ltm Rtm Ltr Rtr", kAudioChannelLayoutTag_Atmos_9_1_6, 0 },
};

#define LAYOUT_MASK_VALID(layout, mask) \
  (((layout) == kAudioChannelLayoutTag_UseChannelBitmap) == ((mask) != 0))


const char *EnumSupportedChannelLayouts(int idx, char fmt)
{
  int n=0;
  for (int i=0; i < sizeof(CHAN_LAYOUTS)/sizeof(CHAN_LAYOUTS[0]); ++i)
  {
    WDL_ASSERT(LAYOUT_MASK_VALID(CHAN_LAYOUTS[i].chan_layout, CHAN_LAYOUTS[i].chan_mask));

    if ((!fmt || strchr(CHAN_LAYOUTS[i].fmts, fmt)) && idx == n++)
    {
      return CHAN_LAYOUTS[i].desc;
    }
  }
  return NULL;
}


const char *GetChannelLayoutDesc(int chan_layout, int chan_mask)
{
  if (!LAYOUT_MASK_VALID(chan_layout, chan_mask)) return NULL;

  bool is_match=false;
  for (int i=0; i < sizeof(CHAN_LAYOUTS)/sizeof(CHAN_LAYOUTS[0]); ++i)
  {
    if (CHAN_LAYOUTS[i].chan_mask != 0)
    {
      if (CHAN_LAYOUTS[i].chan_mask == chan_mask) is_match=true;
    }
    else if (CHAN_LAYOUTS[i].nch == -1)
    {
      // high order ambisonic layout tags are OR'd with the actual number of channels
      if ((CHAN_LAYOUTS[i].chan_layout&0xFFFF0000) == (chan_layout&0xFFFF0000)) is_match=true;
    }
    else
    {
      if (CHAN_LAYOUTS[i].chan_layout == chan_layout) is_match=true;
    }
    if (is_match)
    {
      return CHAN_LAYOUTS[i].desc;
    }
  }
  return NULL;
}

bool GetChannelLayoutFromDesc(const char *desc, int *chan_layout, int *chan_mask, int *nch)
{
  if (!desc || !desc[0]) return false;
  for (int i=0; i < sizeof(CHAN_LAYOUTS)/sizeof(CHAN_LAYOUTS[0]); ++i)
  {
    if (!strcmp(desc, CHAN_LAYOUTS[i].desc))
    {
      if (chan_layout) *chan_layout=CHAN_LAYOUTS[i].chan_layout;
      if (chan_mask) *chan_mask=CHAN_LAYOUTS[i].chan_mask;
      if (nch) *nch=CHAN_LAYOUTS[i].nch;
      return true;
    }
  }
  return false;
}


bool PackFlacPicBase64(WDL_StringKeyedArray<char*> *metadata,
  int img_w, int img_h, int bpp, WDL_HeapBuf *hb)
{
  if (!metadata || !hb || img_w <= 0 || img_h <= 0) return false;

  const char *picfn=metadata->Get("FLACPIC:APIC_FILE");
  const char *pictype=metadata->Get("FLACPIC:APIC_TYPE");
  const char *picdesc=metadata->Get("FLACPIC:APIC_DESC");

  if (!picfn || !picfn[0]) return false;
  if (!pictype) pictype="3";
  if (!picdesc) picdesc="";

  const char *mime=NULL;
  const char *ext=WDL_get_fileext(picfn);
  if (ext && (!stricmp(ext, ".jpg") || !stricmp(ext, ".jpeg"))) mime="image/jpeg";
  else if (ext && !stricmp(ext, ".png")) mime="image/png";
  if (!mime) return false;

  WDL_FileRead fr(picfn);
  if (!fr.IsOpen()) return false;

  WDL_INT64 datalen64=fr.GetSize();
  if (datalen64 < 1 || datalen64 >= (1<<30))
  {
    return false;
  }
  int datalen = (int)datalen64;
  int r8 = (datalen&7) ? 8-(datalen&7) : 0;

  // see opusfile src/info.c opus_picture_tag_parse_impl
  // for what we are apparently encoding

  int mimelen=strlen(mime);
  int desclen=strlen(picdesc);

  int binlen =
    4+ // pictype
    4+mimelen+
    4+desclen+
    4+4+4+4+ // w, h, depth, colors
    4+datalen+r8;

  WDL_HeapBuf hb_bin;
  unsigned char *p=(unsigned char*)hb_bin.ResizeOK(binlen);
  if (!p)
  {
    return false;
  }
  unsigned char *op=p;

  int t=atoi(pictype);
  _AddInt32(t);
  _AddInt32(mimelen);
  memcpy(p, mime, mimelen);
  p += mimelen;
  _AddInt32(desclen);
  memcpy(p, picdesc, desclen);
  p += desclen;
  _AddInt32(img_w);
  _AddInt32(img_h);
  _AddInt32(bpp);
  _AddInt32(0);
  _AddInt32(datalen+r8);

  fr.Read(p, datalen);
  p += datalen;

  memset(p, 0, r8);
  p += r8;

  if (WDL_NORMALLY(p-op == binlen))
  {
    int base64len=binlen*4/3;
    if (base64len&3) base64len += 4-(base64len&3);
    ++base64len; // nul terminated return

    int osz=hb->GetSize();
    char *pout=(char*)hb->ResizeOK(osz+base64len);
    if (pout)
    {
      wdl_base64encode(op, pout, binlen);
      return true;
    }
  }

  return false;
}

bool ExportMetadataImageToTmpFile(const char *srcfn, const char *infostr,
  WDL_FastString *imgdesc, WDL_FastString *imgtype, WDL_FastString *imgfn)
{
  if (!srcfn || !srcfn[0] || !infostr || !infostr[0] || !imgfn) return false;

  WDL_HeapBuf tmp;
  int tmplen=strlen(infostr);
  if (!tmp.ResizeOK(tmplen+1)) return false;
  memcpy(tmp.Get(), infostr, tmplen+1);

  const char *ext=NULL, *mime=NULL, *desc=NULL, *type=NULL, *offs=NULL, *len=NULL;
  char *p=(char*)tmp.Get();
  for (int i=0; i < tmplen; ++i)
  {
    if (!strncmp(p+i, "ext:", 4)) { if (i) p[i-1]=0; i += 4; ext=p+i; }
    else if (!strncmp(p+i, "mime:", 5)) { if (i) p[i-1]=0; i += 5; mime=p+i; }
    else if (!strncmp(p+i, "desc:", 5)) { if (i) p[i-1]=0; i += 5; desc=p+i; }
    else if (!strncmp(p+i, "type:", 5)) { if (i) p[i-1]=0; i += 5; type=p+i; }
    else if (!strncmp(p+i, "offset:", 7)) { if (i) p[i-1]=0; i += 7; offs=p+i; }
    else if (!strncmp(p+i, "length:", 7)) { if (i) p[i-1]=0; i += 7; len=p+i; }
  }

  WDL_INT64 ioffs = offs ? (WDL_INT64)atof(offs) : 0;
  int ilen = len ? atoi(len) : 0;

  bool ok=false;
  if ((ext || mime) && ioffs > 0 && ilen > 0)
  {
    WDL_FileRead fr(srcfn);
    if (fr.IsOpen() && fr.GetSize() >= ioffs+ilen)
    {
      fr.SetPosition(ioffs);

      char tmppath[2048];
      tmppath[0]=0;
      GetTempPath(sizeof(tmppath), tmppath);
      imgfn->Set(tmppath);
      imgfn->Append(WDL_get_filepart(srcfn));
      imgfn->Append(".");
      if (ext) imgfn->Append(ext);
      else if (mime && !strncmp(mime, "image/", 6)) imgfn->Append(mime+6);

      WDL_FileWrite fw(imgfn->Get());
      if (fw.IsOpen() && CopyFileData(&fr, &fw, ilen))
      {
        if (desc && imgdesc) imgdesc->Set(desc);
        if (type && imgtype) imgtype->Set(type);
        ok=true;
      }
    }
  }

  return ok;
}



#endif // _METADATA_H_
