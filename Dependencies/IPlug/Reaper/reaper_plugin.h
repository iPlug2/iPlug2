/***************************************
*** REAPER Plug-in API
**
** Copyright (C) 2006-2014, Cockos Incorporated
**
**    This software is provided 'as-is', without any express or implied
**    warranty.  In no event will the authors be held liable for any damages
**    arising from the use of this software.
**
**    Permission is granted to anyone to use this software for any purpose,
**    including commercial applications, and to alter it and redistribute it
**    freely, subject to the following restrictions:
**
**    1. The origin of this software must not be misrepresented; you must not
**       claim that you wrote the original software. If you use this software
**       in a product, an acknowledgment in the product documentation would be
**       appreciated but is not required.
**    2. Altered source versions must be plainly marked as such, and must not be
**       misrepresented as being the original software.
**    3. This notice may not be removed or altered from any source distribution.
**
** Notes: the C++ interfaces used require MSVC on win32, or at least the MSVC-compatible C++ ABI. Sorry, mingw users :(
**
*/

#ifndef _REAPER_PLUGIN_H_
#define _REAPER_PLUGIN_H_


#ifndef REASAMPLE_SIZE
#define REASAMPLE_SIZE 8 // if we change this it will break everything!
#endif

#if REASAMPLE_SIZE == 4
typedef float ReaSample;
#else
typedef double ReaSample;
#endif



#ifdef _WIN32
#include <windows.h>

#define REAPER_PLUGIN_DLL_EXPORT __declspec(dllexport)
#define REAPER_PLUGIN_HINSTANCE HINSTANCE

#else
#include "../WDL/swell/swell.h"
#include <pthread.h>

#define REAPER_PLUGIN_DLL_EXPORT __attribute__((visibility("default")))
#define REAPER_PLUGIN_HINSTANCE void *
#endif

#define REAPER_PLUGIN_ENTRYPOINT ReaperPluginEntry
#define REAPER_PLUGIN_ENTRYPOINT_NAME "ReaperPluginEntry"

#ifdef _MSC_VER
#define INT64 __int64
#define INT64_CONSTANT(x) (x##i64)
#else
#define INT64 long long
#define INT64_CONSTANT(x) (x##LL)
#endif

/* 
** Endian-tools and defines (currently only __ppc__ and BIG_ENDIAN is recognized, for OS X -- all other platforms are assumed to be LE)
*/

static int REAPER_BSWAPINT(int x)
{
  return ((((x))&0xff)<<24)|((((x))&0xff00)<<8)|((((x))&0xff0000)>>8)|(((x)>>24)&0xff);
}
static void REAPER_BSWAPINTMEM(void *buf)
{
  char *p=(char *)buf;
  char tmp=p[0]; p[0]=p[3]; p[3]=tmp;
  tmp=p[1]; p[1]=p[2]; p[2]=tmp;
}
static void REAPER_BSWAPINTMEM8(void *buf)
{
  char *p=(char *)buf;
  char tmp=p[0]; p[0]=p[7]; p[7]=tmp;
  tmp=p[1]; p[1]=p[6]; p[6]=tmp;
  tmp=p[2]; p[2]=p[5]; p[5]=tmp;
  tmp=p[3]; p[3]=p[4]; p[4]=tmp;
}

#if defined(__ppc__)

#define REAPER_BIG_ENDIAN
#define REAPER_FOURCC(d,c,b,a) (((unsigned int)(d)&0xff)|(((unsigned int)(c)&0xff)<<8)|(((unsigned int)(b)&0xff)<<16)|(((unsigned int)(a)&0xff)<<24))

#define REAPER_MAKEBEINT(x) (x)
#define REAPER_MAKEBEINTMEM(x)
#define REAPER_MAKEBEINTMEM8(x)
#define REAPER_MAKELEINT(x) REAPER_BSWAPINT(x)
#define REAPER_MAKELEINTMEM(x) REAPER_BSWAPINTMEM(x)
#define REAPER_MAKELEINTMEM8(x) REAPER_BSWAPINTMEM8(x)

#else

#define REAPER_FOURCC(a,b,c,d) (((unsigned int)(d)&0xff)|(((unsigned int)(c)&0xff)<<8)|(((unsigned int)(b)&0xff)<<16)|(((unsigned int)(a)&0xff)<<24))

#define REAPER_MAKELEINT(x) (x)
#define REAPER_MAKELEINTMEM(x)
#define REAPER_MAKELEINTMEM8(x)
#define REAPER_MAKEBEINT(x) REAPER_BSWAPINT(x)
#define REAPER_MAKEBEINTMEM(x) REAPER_BSWAPINTMEM(x)
#define REAPER_MAKEBEINTMEM8(x) REAPER_BSWAPINTMEM8(x)

#endif


#if 1
#define ADVANCE_TIME_BY_SAMPLES(t, spls, srate) ((t) += (spls)/(double)(srate))  // not completely accurate but still quite accurate.
#else
#define ADVANCE_TIME_BY_SAMPLES(t, spls, srate) ((t) = floor((t) * (srate) + spls + 0.5)/(double)(srate)) // good forever? may have other issues though if srate changes. disabled for now
#endif



//  int ReaperPluginEntry(HINSTANCE hInstance, reaper_plugin_info_t *rec);
//  return 1 if you are compatible (anything else will result in plugin being unloaded)
//  if rec == NULL, then time to unload 

#define REAPER_PLUGIN_VERSION 0x20E

typedef struct reaper_plugin_info_t
{
  int caller_version; // REAPER_PLUGIN_VERSION

  HWND hwnd_main;

  // this is the API that plug-ins register most things, be it keyboard shortcuts, project importers, etc.
  // typically you register things on load of the DLL, for example:

  // static pcmsink_register_t myreg={ ... };
  // rec->Register("pcmsink",&myreg);
 
  // then on plug-in unload (or if you wish to remove it for some reason), you should do:
  // rec->Register("-pcmsink",&myreg);
  // the "-" prefix is supported for most registration types.
  int (*Register)(const char *name, void *infostruct); // returns 1 if registered successfully

  // get a generic API function, there many of these defined.
  void * (*GetFunc)(const char *name); // returns 0 if function not found

} reaper_plugin_info_t;




/****************************************************************************************
**** interface for plugin objects to save/load state. they should use ../WDL/LineParser.h too...
***************************************************************************************/

// ProjectStateContext tempflags meaning for &0xFFFF
enum 
{
  PROJSTATECTX_UNDO_REDO=1,
  PROJSTATECTX_SAVE_LOAD=2, // project, track template, etc
  PROJSTATECTX_CUTCOPY_PASTE=3,
};
// &0xFFFF0000 is for receiver use

#ifndef _WDL_PROJECTSTATECONTEXT_DEFINED_
#define _REAPER_PLUGIN_PROJECTSTATECONTEXT_DEFINED_
class ProjectStateContext // this is also defined in WDL, need to keep these interfaces identical
{
public:
  virtual ~ProjectStateContext(){};

#ifdef __GNUC__
  virtual void  __attribute__ ((format (printf,2,3))) AddLine(const char *fmt, ...) = 0;
#else
  virtual void AddLine(const char *fmt, ...)=0;
#endif
  virtual int GetLine(char *buf, int buflen)=0; // returns -1 on eof

  virtual INT64 GetOutputSize()=0; // output size written so far, only usable on REAPER 3.14+

  virtual int GetTempFlag()=0;
  virtual void SetTempFlag(int flag)=0;
};
#endif



/***************************************************************************************
**** MIDI event definition and abstract list
***************************************************************************************/

typedef struct
{
  int frame_offset;
  int size; // bytes used by midi_message, can be >3, but should never be <3, even if a short 1 or 2 byte msg
  unsigned char midi_message[4]; // size is number of bytes valid -- can be more than 4!
} MIDI_event_t;

class MIDI_eventlist
{
public:
  virtual void AddItem(MIDI_event_t *evt)=0;
  virtual MIDI_event_t *EnumItems(int *bpos)=0; 
  virtual void DeleteItem(int bpos)=0;
  virtual int GetSize()=0; // size of block in bytes
  virtual void Empty()=0;

protected:
  // this is only defined in REAPER 4.60+, for 4.591 and earlier you should delete only via the implementation pointer
  virtual ~MIDI_eventlist() { }
};

typedef struct
{
  double ppqpos;
  double ppqpos_end; // only for note events
  char flag; // &1=selected, &2=muted
  unsigned char msg[3]; // if varmsg is valid, msg[0] is the text event type or 0xF0 for sysex
  char* varmsg;
  int varmsglen;
  int setflag; // &1:selected, &2:muted, &4:ppqpos, &8:endppqpos, &16:msg1 high bits, &32:msg1 low bits, &64:msg2, &128:msg3, &256:varmsg, &512:text/sysex type (msg[0])
} MIDI_eventprops;


/***************************************************************************************
**** PCM source API
***************************************************************************************/



typedef struct
{
  double time_s; // start time of block

  double samplerate; // desired output samplerate and channels
  int nch;

  int length; // desired length in sample(pair)s of output

  ReaSample *samples; // samples filled in (the caller allocates this)
  int samples_out; // updated with number of sample(pair)s actually rendered

  MIDI_eventlist *midi_events;

  double approximate_playback_latency; // 0.0 if not supported
  double absolute_time_s;
  double force_bpm;
} PCM_source_transfer_t;

typedef struct
{
  double start_time; // start time of block
  double peakrate;   // peaks per second (see samplerate below)

  int numpeak_points; // desired number of points for data

  int nchpeaks; // number of channels of peaks data requested

  ReaSample *peaks;  // peaks output (caller allocated)
  int peaks_out; // number of points actually output (less than desired means at end)

  enum 
  { 
    PEAKTRANSFER_PEAKS_MODE=0, 
    PEAKTRANSFER_WAVEFORM_MODE=1, 
    PEAKTRANSFER_MIDI_NOTE_MODE=2,
    PEAKTRANSFER_MIDI_DRUM_MODE=3,
    PEAKTRANSFER_MIDI_DRUM_TRIANGLE_MODE=4,
  };
  int output_mode; // see enum above

  double absolute_time_s;

  ReaSample *peaks_minvals; // can be NULL, otherwise receives minimum values
  int peaks_minvals_used;

  double samplerate; // peakrate is peaks per second, samplerate is used only as a hint for what style of peaks to draw, OK to pass in zero

  int *exp[30];

} PCM_source_peaktransfer_t;


// used to update MIDI sources with new events during recording
typedef struct
{
  double global_time;
  double global_item_time;
  double srate;
  int length; // length in samples
  int overwritemode; // 0=overdub, 1=replace, 
                     // -1 = literal (do nothing just add)
                     // 65536+(16 bit mask) = replace just these channels (touch-replace)
  MIDI_eventlist *events;
  double item_playrate;

  double latency;

  unsigned int *overwrite_actives; // [16][4]; only used when overwritemode is >0
} midi_realtime_write_struct_t;


// abstract base class
class PCM_source
{
  public:
    virtual ~PCM_source() { }

    virtual PCM_source *Duplicate()=0;

    virtual bool IsAvailable()=0;
    virtual void SetAvailable(bool avail) { } // optional, if called with avail=false, close files/etc, and so on
    virtual const char *GetType()=0;
    virtual const char *GetFileName() { return NULL; }; // return NULL if no filename (not purely a file)
    virtual bool SetFileName(const char *newfn)=0; // return TRUE if supported, this will only be called when offline

    virtual PCM_source *GetSource() { return NULL; }
    virtual void SetSource(PCM_source *src) { }
    virtual int GetNumChannels()=0; // return number of channels
    virtual double GetSampleRate()=0; // returns preferred sample rate. if < 1.0 then it is assumed to be silent (or MIDI)
    virtual double GetLength()=0; // length in seconds
    virtual double GetLengthBeats() { return -1.0; } // length in beats if supported
    virtual int GetBitsPerSample() { return 0; } // returns bits/sample, if available. only used for metadata purposes, since everything returns as doubles anyway
    virtual double GetPreferredPosition() { return -1.0; } // not supported returns -1

    virtual int PropertiesWindow(HWND hwndParent)=0;

    virtual void GetSamples(PCM_source_transfer_t *block)=0;
    virtual void GetPeakInfo(PCM_source_peaktransfer_t *block)=0;

    virtual void SaveState(ProjectStateContext *ctx)=0;
    virtual int LoadState(const char *firstline, ProjectStateContext *ctx)=0; // -1 on error


    // these are called by the peaks building UI to build peaks for files.
    virtual void Peaks_Clear(bool deleteFile)=0;
    virtual int PeaksBuild_Begin()=0; // returns nonzero if building is opened, otherwise it may mean building isn't necessary
    virtual int PeaksBuild_Run()=0; // returns nonzero if building should continue
    virtual void PeaksBuild_Finish()=0; // called when done

    virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};

typedef struct
{
  int m_id;
  double m_time;
  double m_endtime;
  bool m_isregion;
  char *m_name; // can be NULL if unnamed
  int m_flags; // DEPRECATED, for legacy use only when calling PCM_SOURCE_EXT_ENUMCUES. &1=caller must call Extended(PCM_SOURCE_EXT_ENUMCUES, -1, &cue, 0) when finished
  char resvd[124]; // future expansion -- should be 0
} REAPER_cue;

typedef struct
{
  PCM_source* m_sliceSrc;
  double m_beatSnapOffset;
  char resvd[128];  // future expansion -- should be 0
} REAPER_slice;

typedef struct
{
  double draw_start_time;
  int draw_start_y; // can be >0 to treat there as extra data
  double pixels_per_second;
  int width, height; // width and height of view
  int mouse_x, mouse_y; // valid only on mouse/key messages
  void *extraParms[8]; // WM_KEYDOWN uses [0] for MSG *
} REAPER_inline_positioninfo;

#define PCM_SOURCE_EXT_INLINEEDITOR 0x100  // parm1 = (void *)(INT_PTR)message, parm2/parm3 = parms
                                           // messages: 0 = query if editor is available/supported. returns <0 if supported but unavailable, >0 if available, 0=if not supported
                                           //  WM_CREATE to create the editor instance, WM_DESTROY to destroy it (nonzero if success)
                                           //  WM_ERASEBKGND / WM_PAINT / WM_NCPAINT (3 stages) for drawing, parm2 = LICE_IBitmap *, parm3 = (REAPER_inline_positioninfo*)
                                           //  WM_LBUTTON*, WM_RBUTTON*, WM_MOUSEMOVE, WM_MOUSEWHEEL
                                           // parm2=rsvd, parm3= REAPER_inline_positioninfo) -- 
                                           // return REAPER_INLINE_* flags
                                           // WM_SETCURSOR gets parm3=REAPER_inline_positioninfo*, UPDATED: should set extraParms[0] to HCURSOR
                                           // WM_KEYDOWN gets parm3=REAPER_inline_positioninfo* with extraParms[0] to MSG*
#define REAPER_INLINE_RETNOTIFY_INVALIDATE 0x1000000 // want refresh of display
#define REAPER_INLINE_RETNOTIFY_SETCAPTURE 0x2000000 // setcapture
#define REAPER_INLINE_RETNOTIFY_SETFOCUS   0x4000000 // set focus to item
#define REAPER_INLINE_RETNOTIFY_NOAUTOSCROLL 0x8000000

#define REAPER_INLINEFLAG_WANTOVERLAYEDCONTROLS 0x4000 // only valid as a ret for msg 0, to have fades/etc still drawn over like normal


#define PCM_SOURCE_EXT_PROJCHANGENOTIFY 0x2000 // parm1 = nonzero if activated project, zero if deactivated project

#define PCM_SOURCE_EXT_OPENEDITOR 0x10001 // parm1=hwnd, parm2=track idx, parm3=item description
#define PCM_SOURCE_EXT_GETEDITORSTRING 0x10002 // parm1=index (0 or 1), parm2=(const char**)desc, optional parm3=(int*)has_had_editor
#define PCM_SOURCE_EXT_DEPRECATED_1 0x10003 // was PCM_SOURCE_EXT_CLOSESECONDARYSRC 
#define PCM_SOURCE_EXT_SETITEMCONTEXT 0x10004 // parm1=MediaItem*,  parm2=MediaItem_Take*
#define PCM_SOURCE_EXT_ADDMIDIEVENTS 0x10005 // parm1=pointer to midi_realtime_write_struct_t, nch=1 for replace, =0 for overdub, parm2=midi_quantize_mode_t* (optional)
#define PCM_SOURCE_EXT_GETASSOCIATED_RPP 0x10006 // parm1=pointer to char* that will receive a pointer to the string
#define PCM_SOURCE_EXT_GETMETADATA 0x10007 // parm1=pointer to name string, parm2=pointer to buffer, parm3=(int)buffersizemax . returns length used. Defined strings are "DESC", "ORIG", "ORIGREF", "DATE", "TIME", "UMID", "CODINGHISTORY" (i.e. BWF)
#define PCM_SOURCE_EXT_SETASSECONDARYSOURCE 0x10008  // parm1=optional pointer to src (same subtype as receiver), if supplied, set the receiver as secondary src for parm1's editor, if not supplied, receiver has to figure out if there is an appropriate editor open to attach to, parm2=trackidx, parm3=itemname
#define PCM_SOURCE_EXT_SHOWMIDIPREVIEW 0x10009  // parm1=(MIDI_eventlist*), can be NULL for all-notes-off (also to check if this source supports showing preview at this moment)
#define PCM_SOURCE_EXT_SEND_EDITOR_MSG 0x1000A  // parm1=int: 1=focus editor to primary, 2=focus editor to all, 3=focus editor to all selected
#define PCM_SOURCE_EXT_SETSECONDARYSOURCELIST 0x1000B // parm1=(PCM_source**)sourcelist, parm2=list size, parm3=close any existing src not in the list
#define PCM_SOURCE_EXT_ISOPENEDITOR 0x1000C // returns 1 if this source is currently open in an editor, parm1=1 to close
#define PCM_SOURCE_EXT_GETITEMCONTEXT 0x10010 // parm1=MediaItem**, parm2=MediaItem_Take**, parm3=MediaTrack**
#define PCM_SOURCE_EXT_CONFIGISFILENAME 0x20000
#define PCM_SOURCE_EXT_GETBPMANDINFO 0x40000 // parm1=pointer to double for bpm. parm2=pointer to double for snap/downbeat offset (seconds).
#define PCM_SOURCE_EXT_GETNTRACKS 0x80000 // for midi data, returns number of tracks that would have been available
#define PCM_SOURCE_EXT_GETTITLE   0x80001 // parm1=(char**)title (string persists in plugin)
#define PCM_SOURCE_EXT_GETTEMPOMAP 0x80002
#define PCM_SOURCE_EXT_WANTOLDBEATSTYLE 0x80003
#define PCM_SOURCE_EXT_WANTTRIM 0x90002 // bla
#define PCM_SOURCE_EXT_TRIMITEM 0x90003 // parm1=lrflag, parm2=double *{position,length,startoffs,rate}
#define PCM_SOURCE_EXT_EXPORTTOFILE 0x90004 // parm1=output filename, only currently supported by MIDI but in theory any source could support this
#define PCM_SOURCE_EXT_ENUMCUES 0x90005 // DEPRECATED, use PCM_SOURCE_EXT_ENUMCUES_EX instead.  parm1=(int) index of cue to get (-1 to free cue), parm2=(optional)REAPER_cue **.  Returns 0 and sets parm2 to NULL when out of cues. return value otherwise is how much to advance parm2 (1, or 2 usually)
#define PCM_SOURCE_EXT_ENUMCUES_EX 0x90016 // parm1=(int) index of cue (source must provide persistent backing store for cue->m_name), parm2=(REAPER_cue*) optional. Returns 0 when out of cues, otherwise returns how much to advance index (1 or 2 usually). 
// a PCM_source may be the parent of a number of beat-based slices, if so the parent should report length and nchannels only, handle ENUMSLICES, and be deleted after the slices are retrieved
#define PCM_SOURCE_EXT_ENUMSLICES 0x90006 // parm1=(int*) index of slice to get, parm2=REAPER_slice* (pointing to caller's existing slice struct). if parm2 passed in zero, returns the number of slices. returns 0 if no slices or out of slices. 
#define PCM_SOURCE_EXT_ENDPLAYNOTIFY 0x90007 // notify a source that it can release any pooled resources
#define PCM_SOURCE_EXT_SETPREVIEWTEMPO 0x90008 // parm1=(double*)bpm, only meaningful for MIDI or slice-based source media

enum { RAWMIDI_NOTESONLY=1, RAWMIDI_UNFILTERED=2 };
#define PCM_SOURCE_EXT_GETRAWMIDIEVENTS 0x90009 // parm1 = (PCM_source_transfer_t *), parm2 = RAWMIDI flags

#define PCM_SOURCE_EXT_SETRESAMPLEMODE 0x9000A // parm1= mode to pass to resampler->Extended(RESAMPLE_EXT_SETRSMODE,mode,0,0)
#define PCM_SOURCE_EXT_NOTIFYPREVIEWPLAYPOS 0x9000B // parm1 = ptr to double of play position, or NULL if stopped
#define PCM_SOURCE_EXT_SETSIZE 0x9000C // parm1=(double*)startpos, parm2=(double*)endpos, parm3=1 if start/end in QN. Start can be negative. Receiver may adjust start/end to avoid erasing content, in which case the adjusted values are returned in parm1 and parm2.
#define PCM_SOURCE_EXT_GETSOURCETEMPO 0x9000D // parm=(double*)bpm, this is for reporting purposes only, does not necessarily mean the media should be adjusted (as PCM_SOURCE_EXT_GETBPMANDINFO means)
#define PCM_SOURCE_EXT_ISABNORMALAUDIO  0x9000E // return 1 if rex, video, etc (meaning file export will just copy file directly rather than trim/converting)
#define PCM_SOURCE_EXT_GETPOOLEDMIDIID 0x9000F // parm1=(char*)id, parm2=(int*)pool user count, parm3=(MediaItem_Take**)firstuser
#define PCM_SOURCE_EXT_REMOVEFROMMIDIPOOL 0x90010 
#define PCM_SOURCE_EXT_GETMIDIDATAHASH 0x90011 // parm1=(WDL_UINT64*)hash (64-bit hash of the MIDI source data)
#define PCM_SOURCE_EXT_GETIMAGE 0x90012  // parm1=(LICE_IBitmap**)image. parm2 = NULL or pointer to int, which is (w<<16)|h desired approx
#define PCM_SOURCE_EXT_NOAUDIO 0x90013 
#define PCM_SOURCE_EXT_HASMIDI 0x90014 // returns 1 if contains any MIDI data, parm1=(double*)time offset of first event
#define PCM_SOURCE_EXT_DELETEMIDINOTES 0x90015 // parm1=(double*)minlen (0.125 for 1/8 notes, etc), parm2=1 if only trailing small notes should be deleted, parm3=(bool*)true if any notes were deleted (return)
#define PCM_SOURCE_EXT_GETGUID 0x90017 // parm1=(GUID*)guid
#define PCM_SOURCE_EXT_DOPASTEINITEM 0x90100 // no parms used, acts as a paste from clipboard
#define PCM_SOURCE_EXT_GETNOTERANGE 0x90018 // parm1=(int*)low note, parm2=(int*)high note
#define PCM_SOURCE_EXT_PPQCONVERT 0x90020 // parm1=(double*)pos, parm2=(int)flag 0=ppq to proj time, 1=proj time to ppq
#define PCM_SOURCE_EXT_COUNTMIDIEVTS 0x90021 // parm1=(int*)notecnt, parm2=(int*)ccevtcnt, parm3=(int*)metaevtcnt
#define PCM_SOURCE_EXT_GETSETMIDIEVT 0x90022 // parm1=(MIDI_eventprops*)event properties (NULL to delete); parm2=(int)event index (<0 to insert); parm2=(int)flag: 1=index counts notes only, 2=index counts CC only, 3=index counts meta-events only
#define PCM_SOURCE_EXT_GETSUGGESTEDTEXT 0x90023 // parm1=char ** which will receive pointer to suggested label text, if any

// register with Register("pcmsrc",&struct ... and unregister with "-pcmsrc"
typedef struct {
  PCM_source *(*CreateFromType)(const char *type, int priority); // priority is 0-7, 0 is highest
  PCM_source *(*CreateFromFile)(const char *filename, int priority); // if priority is 5-7, and the file isn't found, open it in an offline state anyway, thanks

  // this is used for UI only, not so muc
  const char *(*EnumFileExtensions)(int i, const char **descptr); // call increasing i until returns a string, if descptr's output is NULL, use last description
} pcmsrc_register_t;


/*
** OK so the pcm source class has a lot of responsibility, and people may not wish to
** have to implement them. So, for basic audio decoders, you can use this class instead:
**
** And you can use the PCM_Source_CreateFromSimple API to create a PCM_source from your ISimpleMediaDecoder derived class
*/
class ISimpleMediaDecoder
{
public:
  virtual ~ISimpleMediaDecoder() { }

  virtual ISimpleMediaDecoder *Duplicate()=0;

  // filename can be NULL to use "last filename"
  // diskread* are suggested values to pass to WDL_FileRead if you use it, otherwise can ignore
  virtual void Open(const char *filename, int diskreadmode, int diskreadbs, int diskreadnb)=0;

  // if fullClose=0, close disk resources, but can leave decoders etc initialized (and subsequently check the file date on re-open)
  virtual void Close(bool fullClose)=0; 

  virtual const char *GetFileName()=0;
  virtual const char *GetType()=0;

  // an info string suitable for a dialog, and a title for that dialog
  virtual void GetInfoString(char *buf, int buflen, char *title, int titlelen)=0; 

  virtual bool IsOpen()=0;
  virtual int GetNumChannels()=0;

  virtual int GetBitsPerSample()=0;
  virtual double GetSampleRate()=0;


  // positions in sample frames
  virtual INT64 GetLength()=0;
  virtual INT64 GetPosition()=0;
  virtual void SetPosition(INT64 pos)=0;

  // returns sample-frames read. buf will be at least length*GetNumChannels() ReaSamples long.
  virtual int ReadSamples(ReaSample *buf, int length)=0; 


  // these extended messages may include PCM_source messages
  virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};





/***************************************************************************************
**** PCM sink API
***************************************************************************************/

typedef struct
{
  bool doquant;
  char movemode; // 0=default(l/r), -1=left only, 1=right only
  char sizemode; // 0=preserve length, 1=quantize end
  char quantstrength; // 1-100
  double quantamt; // quantize to (in qn)
  char swingamt; // 1-100
  char range_min; // 0-100
  char range_max; 
} midi_quantize_mode_t;

class PCM_sink
{
  public:
    PCM_sink() { m_st=0.0; }
    virtual ~PCM_sink() { }

    virtual void GetOutputInfoString(char *buf, int buflen)=0;
    virtual double GetStartTime() { return m_st; }
    virtual void SetStartTime(double st) { m_st=st; }
    virtual const char *GetFileName()=0; // get filename, if applicable (otherwise "")
    virtual int GetNumChannels()=0; // return number of channels
    virtual double GetLength()=0; // length in seconds, so far
    virtual INT64 GetFileSize()=0;

    virtual void WriteMIDI(MIDI_eventlist *events, int len, double samplerate)=0;
    virtual void WriteDoubles(ReaSample **samples, int len, int nch, int offset, int spacing)=0;
    virtual bool WantMIDI() { return 0; }

    virtual int GetLastSecondPeaks(int sz, ReaSample *buf) { return 0; }
    virtual void GetPeakInfo(PCM_source_peaktransfer_t *block) { } // allow getting of peaks thus far

    virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported

  private:
    double m_st;
};

#define PCM_SINK_EXT_CREATESOURCE 0x80000 // parm1 = PCM_source ** (will be created if supported)
#define PCM_SINK_EXT_DONE 0x80001 // parm1 = HWND of the render dialog
#define PCM_SINK_EXT_VERIFYFMT 0x80002 // parm1=int *srate, parm2= int *nch. plugin can override (and return 1!!)
#define PCM_SINK_EXT_SETQUANT 0x80003 // parm1 = (midi_quantize_mode_t*), or NULL to disable
#define PCM_SINK_EXT_SETRATE 0x80004 // parm1 = (double *) rateadj
#define PCM_SINK_EXT_GETBITDEPTH 0x80005 // parm1 = (int*) bitdepth (return 1 if supported)
#define PCM_SINK_EXT_ADDCUE 0x80006 // parm1=(PCM_cue*)cue
#define PCM_SINK_EXT_SETCURBLOCKTIME 0x80007 // parm1 = (double *) project position -- called before each WriteDoubles etc

typedef struct  // register using "pcmsink"
{
  unsigned int (*GetFmt)(const char **desc);

  const char *(*GetExtension)(const void *cfg, int cfg_l);
  HWND (*ShowConfig)(const void *cfg, int cfg_l, HWND parent);
  PCM_sink *(*CreateSink)(const char *filename, void *cfg, int cfg_l, int nch, int srate, bool buildpeaks);

} pcmsink_register_t;

typedef struct  // register using "pcmsink_ext"
{
  pcmsink_register_t sink; 

  // for extended calls that refer to the generic type of sink, rather than a specific instance of a sink
  int (*Extended)(int call, void* parm1, void* parm2, void* parm3); 


  char expand[256];
} pcmsink_register_ext_t;

// supported via pcmsink_register_ext_t::Extended:
#define PCMSINKEXT_GETFORMATDESC 0x80000 // parm1=(void*)cfg, parm2=(int)cfglen, parm3=(const char*)retstring 
#define PCMSINKEXT_GETFORMATDATARATE 0x80001 // parm1=(void*)cfg, parm2=(int)cfglen, parm3 = int[] {channels, samplerate}

/***************************************************************************************
**** Resampler API (plug-ins can use this for SRC)
**
** 
** See API functions Resampler_Create() and Resample_EnumModes()
***************************************************************************************/

class REAPER_Resample_Interface
{
public:
  virtual ~REAPER_Resample_Interface(){}
  virtual void SetRates(double rate_in, double rate_out)=0;
  virtual void Reset()=0;

  virtual double GetCurrentLatency()=0; // latency in seconds buffered -- do not call between resampleprepare and resampleout, undefined if you do...
  virtual int ResamplePrepare(int out_samples, int nch, ReaSample **inbuffer)=0; // sample ratio
  virtual int ResampleOut(ReaSample *out, int nsamples_in, int nsamples_out, int nch)=0; // returns output samples

  virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};
#define RESAMPLE_EXT_SETRSMODE 0x1000 // parm1 == (int)resamplemode, or -1 for project default
#define RESAMPLE_EXT_SETFEEDMODE 0x1001 // parm1 = nonzero to set ResamplePrepare's out_samples to refer to request a specific number of input samples
#define RESAMPLE_EXT_RESETWITHFRACPOS 0x6000 // parm1 = (double*)&fracpos


/***************************************************************************************
**** Pitch shift API (plug-ins can use this for pitch shift/time stretch)
**
** See API calls ReaperGetPitchShiftAPI(), EnumPitchShiftModes(), EnumPitchShiftSubModes()
**
***************************************************************************************/

#define REAPER_PITCHSHIFT_API_VER 0x14

class IReaperPitchShift
{
  public:
    virtual ~IReaperPitchShift() { };
    virtual void set_srate(double srate)=0;
    virtual void set_nch(int nch)=0;
    virtual void set_shift(double shift)=0;
    virtual void set_formant_shift(double shift)=0; // shift can be <0 for "only shift when in formant preserve mode", so that you can use it for effective rate changes etc in that mode
    virtual void set_tempo(double tempo)=0;

    virtual void Reset()=0;  // reset all buffers/latency
    virtual ReaSample *GetBuffer(int size)=0;
    virtual void BufferDone(int input_filled)=0;

    virtual void FlushSamples()=0; // make sure all output is available

    virtual bool IsReset()=0;

    virtual int GetSamples(int requested_output, ReaSample *buffer)=0; // returns number of samplepairs returned

    virtual void SetQualityParameter(int parm)=0; // set to: (mode<<16)+(submode), or -1 for "project default" (default)
    virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};
#define REAPER_PITCHSHIFT_EXT_GETMINMAXPRODUCTS 0x1


/***************************************************************************************
**** Peak getting/building API
**
** These are really only needed if you implement a PCM_source or PCM_sink.
**
** See functions PeakGet_Create(), PeakBuild_Create(), GetPeakFileName(), ClearPeakCache()
**
***************************************************************************************/


class REAPER_PeakGet_Interface
{
public:
  virtual ~REAPER_PeakGet_Interface() { }

  virtual double GetMaxPeakRes()=0;
  virtual void GetPeakInfo(PCM_source_peaktransfer_t *block)=0;

  virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};

class REAPER_PeakBuild_Interface
{
public:
  virtual ~REAPER_PeakBuild_Interface() { }

  virtual void ProcessSamples(ReaSample **samples, int len, int nch, int offs, int spread)=0; // in case a sink wants to build its own peaks (make sure it was created with src=NULL)
  virtual int Run()=0; // or let it do it automatically (created with source!=NULL)

  virtual int GetLastSecondPeaks(int sz, ReaSample *buf)=0; // returns number of peaks in the last second, sz is maxsize
  virtual void GetPeakInfo(PCM_source_peaktransfer_t *block)=0; // allow getting of peaks thus far (won't hit the highest resolution mipmap, just the 10/sec one or so)

  virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};

// recommended settings for sources switching to peak caches
#define REAPER_PEAKRES_MAX_NOPKS 200.0
#define REAPER_PEAKRES_MUL_MIN 0.00001 // recommended for plug-ins, when 1000peaks/pix, toss hires source
#define REAPER_PEAKRES_MUL_MAX 1.0 // recommended for plug-ins, when 1.5pix/peak, switch to hi res source. this may be configurable someday via some sneakiness



/*
** accelerator_register_t allows you to register ("accelerator") a record which lets you get a place in the 
** keyboard processing queue.
*/

typedef struct accelerator_register_t
{
  // translateAccel returns:
  // 0 if not our window, 
  // 1 to eat the keystroke, 
  // -1 to pass it on to the window, 
  // -666 to force it to the main window's accel table (with the exception of ESC)
  int (*translateAccel)(MSG *msg, accelerator_register_t *ctx); 
  bool isLocal; // must be TRUE, now (false is no longer supported, heh)
  void *user;
} accelerator_register_t;


/*
** custom_action_register_t allows you to register ("custom_action") an action into a keyboard section action list
** register("custom_action",ca) will return the command ID (instance-dependent but unique across all sections), or 0 if failed (e.g dupe idStr)
** the related callback should be registered with "hookcommand2"
*/

typedef struct
{
  int uniqueSectionId; // 0/100=main/main alt, 32063=media explorer, 32060=midi editor, 32061=midi event list editor, 32062=midi inline editor, etc
  const char* idStr; // must be unique across all sections
  const char* name;
  void *extra; // reserved for future use
} custom_action_register_t;


/*
** gaccel_register_t allows you to register ("gaccel") an action into the main keyboard section action list, and at the same time
** a default binding for it (accel.cmd is the command ID, desc is the description, and accel's other parameters are the
** key to bind. 
*/

typedef struct 
{
  ACCEL accel; // key flags/etc represent default values (user may customize)
  const char *desc; // description (for user customizability)
} gaccel_register_t; // use "gaccel"

/*
** action_help_t lets you register help text ("action_help") for an action, mapped by action name
** (a "help" plugin could register help text for Reaper built-in actions)
*/

typedef struct
{
  const char* action_desc;
  const char* action_help;
} action_help_t;

/*
** register("toggleaction", toggleactioncallback) lets you register a callback function
** that is called to check if an action registered by an extension has an on/off state.

callback function:
  int toggleactioncallback(int command_id);

return:
  -1=action does not belong to this extension, or does not toggle
  0=action belongs to this extension and is currently set to "off"
  1=action belongs to this extension and is currently set to "on"
*/


/*
** register("hookcustommenu", menuhook) lets you register a menu hook function that is called 
** when a customizable Reaper menu is initialized or shown.

hook (callback) function:
  void menuhook(const char* menuidstr, void* menu, int flag); 

flag:
  0=default menu is being initialized
  1=menu is about to be shown

Do not retrieve or modify any Reaper menus except when your menuhook is called.

Reaper calls menuhook with flag=0 when first initializing the menu, before the first time the menu
is displayed.  You can add menu items or submenus during this call, which then become part of the 
default menu structure (which the user can modify). Do not set any menu checked/grayed state,
do not add menu icons, if you add submenus do not retain handles to the submenus.
Do not modify any menus that don't call menuhook.

Reaper calls menuhook with flag=1 before each time the menu is displayed.  You can do any
dynamic menu populating, setting check/grayed states, adding icons during this call.

All handling should be done relative to menu commands, not menu item positions, 
because these menus can be customized and item order can change.
*/


/*
** editor_register_t lets you integrate editors for "open in external editor" for files directly.
*/

typedef struct // register with "editor"
{
  int (*editFile)(const char *filename, HWND parent, int trackidx); // return TRUE if handled for this file
  const char *(*wouldHandle)(const char *filename); // return your editor's description string

} editor_register_t;


/*
** Project import registration.
** 
** Implemented as a your-format->RPP converter, allowing you to generate directly to a ProjectStateContext
*/
typedef struct // register with "projectimport"
{
  bool (*WantProjectFile)(const char *fn); // is this our file?
  const char *(*EnumFileExtensions)(int i, char **descptr); // call increasing i until returns NULL. if descptr's output is NULL, use last description
  int (*LoadProject)(const char *fn, ProjectStateContext *genstate); // return 0=ok, Generate RPP compatible project info in genstate
} project_import_register_t;


typedef struct project_config_extension_t // register with "projectconfig"
{
  // plug-ins may or may not want to save their undo states (look at isUndo)
  // undo states will be saved if UNDO_STATE_MISCCFG is set (for adding your own undo points)
  bool (*ProcessExtensionLine)(const char *line, ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg); // returns BOOL if line (and optionally subsequent lines) processed (return false if not plug-ins line)
  void (*SaveExtensionConfig)(ProjectStateContext *ctx, bool isUndo, struct project_config_extension_t *reg);

  // optional: called on project load/undo before any (possible) ProcessExtensionLine. NULL is OK too
  // also called on "new project" (wont be followed by ProcessExtensionLine calls in that case)
  void (*BeginLoadProjectState)(bool isUndo, struct project_config_extension_t *reg); 

  void *userData;
} project_config_extension_t;


typedef struct prefs_page_register_t // register useing "prefpage"
{
  const char *idstr; // simple id str
  const char *displayname;
  HWND (*create)(HWND par);
  int par_id; 
  const char *par_idstr;

  int childrenFlag; // 1 for will have children

  void *treeitem;
  HWND hwndCache;

  char _extra[64]; // 

} prefs_page_register_t;

typedef struct audio_hook_register_t
{
  void (*OnAudioBuffer)(bool isPost, int len, double srate, struct audio_hook_register_t *reg); // called twice per frame, isPost being false then true
  void *userdata1;
  void *userdata2;

  // plug-in should zero these and they will be set by host
  // only call from OnAudioBuffer, nowhere else!!!
  int input_nch, output_nch; 
  ReaSample *(*GetBuffer)(bool isOutput, int idx); 

} audio_hook_register_t;

/*
** Allows you to get callback from the audio thread before and after REAPER's processing.
** register with Audio_RegHardwareHook()

  Note that you should be careful with this! :)

*/


/* 
** Customizable keyboard section definition etc
**
** Plug-ins may register keyboard action sections in by registering a "accel_section" to a KbdSectionInfo*.
*/

struct KbdAccel;

typedef struct  
{
  DWORD cmd;  // action command ID
  const char *text; // description of action
} KbdCmd;

typedef struct
{
  int key;  // key identifier
  int cmd;  // action command ID
  int flags; // key flags
} KbdKeyBindingInfo;



typedef struct
{
  int uniqueID; // 0=main, < 0x10000000 for cockos use only plzkthx
  const char *name; // section name

  KbdCmd *action_list;   // list of assignable actions
  int action_list_cnt;

  KbdKeyBindingInfo *def_keys; // list of default key bindings
  int def_keys_cnt;

  // hwnd is 0 if MIDI etc. return false if ignoring
  bool (*onAction)(int cmd, int val, int valhw, int relmode, HWND hwnd);

  // this is allocated by the host not by the plug-in using it
  // the user can edit the list of actions/macros
#ifdef _WDL_PTRLIST_H_
  WDL_PtrList<struct KbdAccel> *accels;  
  WDL_TypedBuf<int>* recent_cmds;
#else
  void* accels;
  void *recent_cmds;
#endif

  void *extended_data[32]; // for internal use
} KbdSectionInfo;



typedef struct
{
/*
** Note: you must initialize/deinitialize the cs/mutex (depending on OS) manually, and use it if accessing most parameters while the preview is active.
*/

#ifdef _WIN32
  CRITICAL_SECTION cs;
#else
  pthread_mutex_t mutex;
#endif
  PCM_source *src;
  int m_out_chan; // &1024 means mono, low 10 bits are index of first channel
  double curpos;
  bool loop;
  double volume;

  double peakvol[2];
  void *preview_track; // used for track previews, but only if m_out_chan == -1
} preview_register_t;

/*
** preview_register_t is not used with the normal register system, instead it's used with PlayPreview(), StopPreview(), PlayTrackPreview(), StopTrackPreview()
*/



#ifdef REAPER_WANT_DEPRECATED_COLORTHEMESTUFF /* no longer used -- see icontheme.h and GetColorThemeStruct() */

/*
** ColorTheme API access, these are used with GetColorTheme()
*/

#define COLORTHEMEIDX_TIMELINEFG 0
#define COLORTHEMEIDX_ITEMTEXT 1
#define COLORTHEMEIDX_ITEMBG 2
#define COLORTHEMEIDX_TIMELINEBG 4
#define COLORTHEMEIDX_TIMELINESELBG 5
#define COLORTHEMEIDX_ITEMCONTROLS 6
#define COLORTHEMEIDX_TRACKBG1 24
#define COLORTHEMEIDX_TRACKBG2 25
#define COLORTHEMEIDX_PEAKS1 28
#define COLORTHEMEIDX_PEAKS2 29
#define COLORTHEMEIDX_EDITCURSOR 35
#define COLORTHEMEIDX_GRID1 36
#define COLORTHEMEIDX_GRID2 37
#define COLORTHEMEIDX_MARKER 38
#define COLORTHEMEIDX_REGION 40
#define COLORTHEMEIDX_GRID3 61
#define COLORTHEMEIDX_LOOPSELBG 100

#define COLORTHEMEIDX_ITEM_LOGFONT -2 // these return LOGFONT * as (int)
#define COLORTHEMEIDX_TL_LOGFONT -1


#define COLORTHEMEIDX_MIDI_TIMELINEBG 66
#define COLORTHEMEIDX_MIDI_TIMELINEFG 67
#define COLORTHEMEIDX_MIDI_GRID1 68
#define COLORTHEMEIDX_MIDI_GRID2 69
#define COLORTHEMEIDX_MIDI_GRID3 70
#define COLORTHEMEIDX_MIDI_TRACKBG1 71
#define COLORTHEMEIDX_MIDI_TRACKBG2 72
#define COLORTHEMEIDX_MIDI_ENDPT 73
#define COLORTHEMEIDX_MIDI_NOTEBG 74
#define COLORTHEMEIDX_MIDI_NOTEFG 75
#define COLORTHEMEIDX_MIDI_ITEMCONTROLS 76
#define COLORTHEMEIDX_MIDI_EDITCURSOR 77
#define COLORTHEMEIDX_MIDI_PKEY1 78
#define COLORTHEMEIDX_MIDI_PKEY2 79
#define COLORTHEMEIDX_MIDI_PKEY3 80
#define COLORTHEMEIDX_MIDI_PKEYTEXT 81
#define COLORTHEMEIDX_MIDI_OFFSCREENNOTE 103
#define COLORTHEMEIDX_MIDI_OFFSCREENNOTESEL 104

#endif // colortheme stuff deprecated

/*
** Screenset API
** 
*/

/*
  Note that "id" is a unique identifying string (usually a GUID etc) that is valid across 
  program runs (stored in project etc). lParam is instance-specific parameter (i.e. "this" pointer etc).
*/
enum
{
  SCREENSET_ACTION_GETHWND = 0,

  SCREENSET_ACTION_IS_DOCKED = 1, // returns 1 if docked
  SCREENSET_ACTION_SWITCH_DOCK = 4, //dock if undocked and vice-versa

  SCREENSET_ACTION_LOAD_STATE=0x100, // load state from actionParm (of actionParmSize). if both are NULL, hide.
  SCREENSET_ACTION_SAVE_STATE,  // save state to actionParm, max length actionParmSize (will usually be 4k or greater), return length
};
typedef LRESULT (*screensetNewCallbackFunc)(int action, const char *id, void *param, void *actionParm, int actionParmSize);

// This is managed using screenset_registerNew(), screenset_unregister(), etc


/*
** MIDI hardware device access.
**
*/


class midi_Output
{
public:
  virtual ~midi_Output() {}

  virtual void BeginBlock() { }  // outputs can implement these if they wish to have timed block sends
  virtual void EndBlock(int length, double srate, double curtempo) { }
  virtual void SendMsg(MIDI_event_t *msg, int frame_offset)=0; // frame_offset can be <0 for "instant" if supported
  virtual void Send(unsigned char status, unsigned char d1, unsigned char d2, int frame_offset)=0; // frame_offset can be <0 for "instant" if supported

};


class midi_Input
{
public:
  virtual ~midi_Input() {}

  virtual void start()=0;
  virtual void stop()=0;

  virtual void SwapBufs(unsigned int timestamp)=0; // DEPRECATED call SwapBufsPrecise() instead  // timestamp=process ms

  virtual void RunPreNoteTracking(int isAccum) { }

  virtual MIDI_eventlist *GetReadBuf()=0; // note: the event list here has frame offsets that are in units of 1/1024000 of a second, NOT sample frames

  virtual void SwapBufsPrecise(unsigned int coarsetimestamp, double precisetimestamp) // coarse=process ms, precise=process sec, the target will know internally which to use
  {
    SwapBufs(coarsetimestamp);  // default impl is for backward compatibility
  }
};



/*
** Control Surface API
*/

class ReaProject;
class MediaTrack;
class MediaItem;
class MediaItem_Take;
class TrackEnvelope;

class IReaperControlSurface
{
  public:
    IReaperControlSurface() { }
    virtual ~IReaperControlSurface() { }
    
    virtual const char *GetTypeString()=0; // simple unique string with only A-Z, 0-9, no spaces or other chars
    virtual const char *GetDescString()=0; // human readable description (can include instance specific info)
    virtual const char *GetConfigString()=0; // string of configuration data

    virtual void CloseNoReset() { } // close without sending "reset" messages, prevent "reset" being sent on destructor


    virtual void Run() { } // called 30x/sec or so.


    // these will be called by the host when states change etc
    virtual void SetTrackListChange() { }
    virtual void SetSurfaceVolume(MediaTrack *trackid, double volume) { }
    virtual void SetSurfacePan(MediaTrack *trackid, double pan) { }
    virtual void SetSurfaceMute(MediaTrack *trackid, bool mute) { }
    virtual void SetSurfaceSelected(MediaTrack *trackid, bool selected) { }
    virtual void SetSurfaceSolo(MediaTrack *trackid, bool solo) { } // trackid==master means "any solo"
    virtual void SetSurfaceRecArm(MediaTrack *trackid, bool recarm) { }
    virtual void SetPlayState(bool play, bool pause, bool rec) { }
    virtual void SetRepeatState(bool rep) { }
    virtual void SetTrackTitle(MediaTrack *trackid, const char *title) { }
    virtual bool GetTouchState(MediaTrack *trackid, int isPan) { return false; }
    virtual void SetAutoMode(int mode) { } // automation mode for current track

    virtual void ResetCachedVolPanStates() { } // good to flush your control states here

    virtual void OnTrackSelection(MediaTrack *trackid) { } // track was selected
    
    virtual bool IsKeyDown(int key) { return false; } // VK_CONTROL, VK_MENU, VK_SHIFT, etc, whatever makes sense for your surface

    virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};

#define CSURF_EXT_RESET 0x0001FFFF // clear all surface state and reset (harder reset than SetTrackListChange)
#define CSURF_EXT_SETINPUTMONITOR 0x00010001 // parm1=(MediaTrack*)track, parm2=(int*)recmonitor
#define CSURF_EXT_SETMETRONOME 0x00010002 // parm1=0 to disable metronome, !0 to enable
#define CSURF_EXT_SETAUTORECARM 0x00010003 // parm1=0 to disable autorecarm, !0 to enable
#define CSURF_EXT_SETRECMODE 0x00010004 // parm1=(int*)record mode: 0=autosplit and create takes, 1=replace (tape) mode
#define CSURF_EXT_SETSENDVOLUME 0x00010005 // parm1=(MediaTrack*)track, parm2=(int*)sendidx, parm3=(double*)volume
#define CSURF_EXT_SETSENDPAN 0x00010006 // parm1=(MediaTrack*)track, parm2=(int*)sendidx, parm3=(double*)pan
#define CSURF_EXT_SETFXENABLED 0x00010007 // parm1=(MediaTrack*)track, parm2=(int*)fxidx, parm3=0 if bypassed, !0 if enabled
#define CSURF_EXT_SETFXPARAM 0x00010008 // parm1=(MediaTrack*)track, parm2=(int*)(fxidx<<16|paramidx), parm3=(double*)normalized value
#define CSURF_EXT_SETLASTTOUCHEDFX 0x0001000A // parm1=(MediaTrack*)track, parm2=(int*)mediaitemidx (may be NULL), parm3=(int*)fxidx. all parms NULL=clear last touched FX
#define CSURF_EXT_SETFOCUSEDFX 0x0001000B // parm1=(MediaTrack*)track, parm2=(int*)mediaitemidx (may be NULL), parm3=(int*)fxidx. all parms NULL=clear focused FX
#define CSURF_EXT_SETLASTTOUCHEDTRACK 0x0001000C // parm1=(MediaTrack*)track
#define CSURF_EXT_SETMIXERSCROLL 0x0001000D // parm1=(MediaTrack*)track, leftmost track visible in the mixer
#define CSURF_EXT_SETBPMANDPLAYRATE 0x00010009 // parm1=*(double*)bpm (may be NULL), parm2=*(double*)playrate (may be NULL)
#define CSURF_EXT_SETPAN_EX 0x0001000E // parm1=(MediaTrack*)track, parm2=(double*)pan, parm3=(int*)mode 0=v1-3 balance, 3=v4+ balance, 5=stereo pan, 6=dual pan. for modes 5 and 6, (double*)pan points to an array of two doubles.  if a csurf supports CSURF_EXT_SETPAN_EX, it should ignore CSurf_SetSurfacePan.
#define CSURF_EXT_SETRECVVOLUME 0x00010010 // parm1=(MediaTrack*)track, parm2=(int*)recvidx, parm3=(double*)volume
#define CSURF_EXT_SETRECVPAN 0x00010011 // parm1=(MediaTrack*)track, parm2=(int*)recvidx, parm3=(double*)pan
#define CSURF_EXT_SETFXOPEN 0x00010012 // parm1=(MediaTrack*)track, parm2=(int*)fxidx, parm3=0 if UI closed, !0 if open
#define CSURF_EXT_SETFXCHANGE 0x00010013 // parm1=(MediaTrack*)track, whenever FX are added, deleted, or change order
#define CSURF_EXT_SETPROJECTMARKERCHANGE 0x00010014 // whenever project markers are changed
#define CSURF_EXT_SUPPORTS_EXTENDED_TOUCH 0x00080001 // returns nonzero if GetTouchState can take isPan=2 for width, etc

typedef struct
{
  const char *type_string; // simple unique string with only A-Z, 0-9, no spaces or other chars
  const char *desc_string; // human readable description

  IReaperControlSurface *(*create)(const char *type_string, const char *configString, int *errStats); // errstats gets |1 if input error, |2 if output error
  HWND (*ShowConfig)(const char *type_string, HWND parent, const char *initConfigString); 
} reaper_csurf_reg_t; // register using "csurf"/"-csurf"

// note you can also add a control surface behind the scenes with "csurf_inst" (IReaperControlSurface*)instance



#ifndef UNDO_STATE_ALL
#define UNDO_STATE_ALL 0xFFFFFFFF
#define UNDO_STATE_TRACKCFG 1 // has track/master vol/pan/routing, ALL envelopes (matser included)
#define UNDO_STATE_FX 2  // track/master fx
#define UNDO_STATE_ITEMS 4  // track items
#define UNDO_STATE_MISCCFG 8 // loop selection, markers, regions, extensions!
#define UNDO_STATE_FREEZE 16 // freeze state -- note that isfreeze is used independently, this is only used for the undo system to serialize the already frozen state
#endif



#endif//_REAPER_PLUGIN_H_
