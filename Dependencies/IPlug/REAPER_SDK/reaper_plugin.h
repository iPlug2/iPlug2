/***************************************
*** REAPER Plug-in API
**
** Copyright (C) 2006-2015, Cockos Incorporated
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

#ifdef __GNUC__
  #define REAPER_STATICFUNC __attribute__((unused)) static
#else
  #define REAPER_STATICFUNC static
#endif

/* 
** Endian-tools and defines (currently only __ppc__ and BIG_ENDIAN is recognized, for OS X -- all other platforms are assumed to be LE)
*/

REAPER_STATICFUNC int REAPER_BSWAPINT(int x)
{
  return ((((x))&0xff)<<24)|((((x))&0xff00)<<8)|((((x))&0xff0000)>>8)|(((x)>>24)&0xff);
}
REAPER_STATICFUNC void REAPER_BSWAPINTMEM(void *buf)
{
  char p[4],tmp;
  memcpy(p,buf,4);
  tmp=p[0]; p[0]=p[3]; p[3]=tmp;
  tmp=p[1]; p[1]=p[2]; p[2]=tmp;
  memcpy(buf,p,4);
}
REAPER_STATICFUNC void REAPER_BSWAPINTMEM8(void *buf)
{
  char p[8],tmp;
  memcpy(p,buf,8);
  tmp=p[0]; p[0]=p[7]; p[7]=tmp;
  tmp=p[1]; p[1]=p[6]; p[6]=tmp;
  tmp=p[2]; p[2]=p[5]; p[5]=tmp;
  tmp=p[3]; p[3]=p[4]; p[4]=tmp;
  memcpy(buf,p,8);
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


//  REAPER extensions must support this entry function:
//  int ReaperPluginEntry(HINSTANCE hInstance, reaper_plugin_info_t *rec);
//  return 1 if you are compatible (anything else will result in plugin being unloaded)
//  if rec == NULL, then time to unload 
//
//  CLAP plugins can access the REAPER API via:
//  const void *clap_host.get_extension(const clap_host *host, "cockos.reaper_extension");
//  which returns a pointer to a reaper_plugin_info_t struct
//
//  CLAP plugins can also (v6.80+) get their context information by getFunc("clap_get_reaper_context")
//  void *(*clap_get_reaper_context)(const clap_host *host, int sel);
//  sel=1 for parent track if track FX
//  sel=2 for parent take if item FX
//  sel=3 for project
//  sel=4 for FxDsp
//  sel=5 for track channel count (INT_PTR)
//  sel=6 for index in chain

#define REAPER_PLUGIN_VERSION 0x20E

typedef struct reaper_plugin_info_t
{
  int caller_version; // REAPER_PLUGIN_VERSION

  HWND hwnd_main;

  /*
  Register() is the API that plug-ins register most things, be it keyboard shortcuts, project importers, etc.
  Register() is also available by using GetFunc("plugin_register")

  extensions typically register things on load of the DLL, for example:

     static pcmsink_register_t myreg={ ... };
     rec->Register("pcmsink",&myreg);

  on plug-in unload (or if the extension wishes to remove it for some reason):
     rec->Register("-pcmsink",&myreg);

  the "-" prefix is supported for most registration types.
  some types support the < prefix to register at the start of the list

  Registration types:

  API_*:
    if you have a function called myfunction(..) that you want to expose to other extensions, use:
      rec->Register("API_myfunction",funcaddress);
    other extensions then use GetFunc("myfunction") to get the function pointer.

  APIdef_*:
    To make a function registered with API_* available via ReaScript, follow the API_ registration with:
      double myfunction(char* str, int flag);
      const char *defstring = "double\0char*,int\0str,flag\0help text for myfunction"
      rec->Register("APIdef_myfunction",(void*)defstring);
    defstring is four null-separated fields: return type, argument types, argument names, and help.

  APIvararg_*:
    Used to set the reascript vararg function pointer for an API_:
       (void *) (void * (*faddr_vararg)(void **arglist, int numparms))

      numparms will typically be the full requested parameter count (including NULL pointers for any
      unspecified optional parameters)

      arguments and return values:
        integer as (void *)(INT_PTR) intval
        double as (void *)&some_double_in_memory
        pointers directly as pointers (can be NULL, especially if Optional)


  hookcommand:
    Registers a hook which runs prior to every action in the main section:
      bool runCommand(int command, int flag);
      rec->Register("hookcommand",runCommand);
    runCommand() should return true if it processed the command (prevent further hooks or the action from running)
    It is OK to call Main_OnCommand() from runCommand(), but it must check for and handle any recursion.

  hookpostcommand:
    Registers a hook which runs after each action in the main section:
      void postCommand(int command, int flag);
      rec->Register("hookpostcommand",postCommand);

  hookcommand2:
    Registers a hook which runs prior to every action triggered by a key/MIDI event:
      bool onAction(KbdSectionInfo *sec, int command, int val, int val2, int relmode, HWND hwnd);
      rec->Register("hookcommand2",hook); \
    onAction returns true if it processed the command (preventing further hooks or actions from running)
      val/val2 are used for actions triggered by MIDI/OSC/mousewheel
        - val = [0..127] and val2 = -1 for MIDI CC,
        - val2 >=0 for MIDI pitch or OSC with value = (val2|(val<<7))/16383.0
        - relmode absolute(0) or 1/2/3 for relative adjust modes

  hookpostcommand2:
     void (*hook)(KbdSectionInfo *section, int actionCommandID, int val, int valhw, int relmode, HWND hwnd, ReaProject *proj);
     rec->Register("hookpostcommand2",hook);

  command_id:
    Registers/looks up a command ID for an action. Parameter is a unique string with only A-Z, a-z, 0-9.
      int command = Register("command_id","MyCommandName");
    returns 0 if unsupported/out of actions

  command_id_lookup:
    Like command_id but only looks up, does not create a new command ID.

  pcmsink_ext:
    Registers an extended audio sink type:
      (pcmsink_register_ext_t *)

  pcmsink:
    Registers an audio sink type:
      (pcmsink_register_t *)

  pcmsrc:
    Registers an audio source:
    (pcmsrc_register_t *)

  timer:
    Runs a timer periodically:
      void (*timer_function)();

  hwnd_info:  (6.29+)
    query information about a hwnd
      int (*callback)(HWND hwnd, INT_PTR info_type);
     -- note, for v7.23+ ( -- check with GetAppVersion() -- ), you may also use a function with this prototype:
      int (*callback)(HWND hwnd, INT_PTR info_type, const MSG *msg); // if msg is non-NULL, it will have information about the currently-processing event.

    return 0 if hwnd is not a known window, or if info_type is unknown

    info_type:
       0 = query if hwnd should be treated as a text-field for purposes of global hotkeys
           return: 1 if text field
           return: -1 if not text field
       1 = query if global hotkeys should be processed for this context  (6.60+)
           return 1 if global hotkey should be skipped (and default accelerator processing used instead)
           return -1 if global hotkey should be forced


  file_in_project_ex:
     void *p[2] = {(void *)fn, projptr };
     plugin_register("file_in_project_ex",p);
     plugin_register("-file_in_project_ex",p);

     fn does not need to persist past the call of the function (it is copied)
     projptr must be a valid ReaProject (the default NULL=g_project semantics do not apply).
     file references are reference counted so you can add twice/remove twice etc.

  file_in_project_ex2:
     Extended syntax to receive rename notifications, or to have your plug-in request that the file go in a subdirectory:

     INT_PTR fileInProjectCallback(void *_userdata, int msg, void *parm) {
       if (msg == 0 && parm)
       {
          // rename notification, parm is (const char *)new filename
       }
       if (msg == 0x100) return (INT_PTR)"samples"; // subdirectory name, if desired (return 0 if not desired)

       if (msg == 0x101) return (INT_PTR)fxdspparentcontext_if_any; // if fx, optional
       if (msg == 0x102) return (INT_PTR)takecontext_if_any; // if pcmsrc, optional
       if (msg == 0x103) return (INT_PTR)"context/plug-in name"; // optional
       return 0;
     }

     void *p[4] = {(void *)fn, projptr, userdatacontext, fileInProjectCallback };
     plugin_register("file_in_project_ex2",p);
     plugin_register("-file_in_project_ex2",p);

  toolbar_icon_map:
     Allows a plugin to override default toolbar images for its registered commands.

     const char *GetToolbarIconName(const char *toolbar_name, int cmd, int state)
     {
       if (!strcmp(toolbarid,"Main toolbar") || !strncmp(toolbarid,"Floating toolbar",16))
         if (cmd == g_registered_command_id) return "toolbar_whatever";
       return NULL;
     }
     plugin_register("toolbar_icon_map", (void *)GetToolbarIconName);

  accel_section:
  action_help:
  custom_action:
  gaccel:
  hookcustommenu:
  prefpage:
  projectimport:
  projectconfig:
  editor:
  accelerator:
  csurf:
  csurf_inst:
  toggleaction:
  on_update_hooks:
  open_file_reduce:

  */

  int (*Register)(const char *name, void *infostruct); // returns 1 if registered successfully

  // get a generic API function, there many of these defined. see reaper_plugin_functions.h
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

#ifdef __cplusplus

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

struct MIDI_event_t
{
  int frame_offset;
  int size; // bytes used by midi_message, can be >3, but should never be <3, even if a short 1 or 2 byte msg
  unsigned char midi_message[4]; // size is number of bytes valid -- can be more than 4!

  // new helpers
  bool is_note() const { return (midi_message[0]&0xe0)==0x80; }
  bool is_note_on() const {
    return (midi_message[0]&0xf0)==0x90 && midi_message[2];
  }
  bool is_note_off() const {
    switch (midi_message[0]&0xf0)
    {
      case 0x80: return true;
      case 0x90: return midi_message[2]==0;
    }
    return false;
  }

  enum {
   CC_ALL_SOUND_OFF=120,
   CC_ALL_NOTES_OFF=123,
   CC_EOF_INDICATOR = CC_ALL_NOTES_OFF
  };
};

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

#endif // __cplusplus

typedef struct _MIDI_eventprops
{
  double ppqpos;
  double ppqpos_end_or_bezier_tension; // only for note events or CC events
  char flag; // &1=selected, &2=muted, >>4&0xF=cc shape
  unsigned char msg[3]; // msg is not valid if varmsglen > 0
  char* varmsg;
  int varmsglen;
  int setflag; // &1:selected, &2:muted, &4:ppqpos, &8:endppqpos or bez tension, &16:msg1 high bits, &32:msg1 low bits, &64:msg2, &128:msg3, &256:varmsg, &1024:shape/tension fields used, &16384:no sort after set
} MIDI_eventprops;


/***************************************************************************************
**** PCM source API
***************************************************************************************/



typedef struct _PCM_source_transfer_t
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

#ifdef __cplusplus

class REAPER_PeakGet_Interface;

typedef struct _PCM_source_peaktransfer_t
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

#define PEAKINFO_EXTRADATA_SPECTRAL1 ((int)'s')
#define PEAKINFO_EXTRADATA_SPECTROGRAM1 ((int)'g')
#define PEAKINFO_EXTRADATA_MIDITEXT ((int)'m')
#define PEAKINFO_EXTRADATA_LOUDNESS_DEPRECATED ((int)'l') // use PEAKINFO_EXTRADATA_LOUDNESS_RAW instead
#define PEAKINFO_EXTRADATA_LOUDNESS_RAW ((int)'r')
#define PEAKINFO_EXTRADATA_LOUDNESS_INTERNAL ((int)'!')

  int extra_requested_data_type; // PEAKINFO_EXTRADATA_* for spectral information
  int extra_requested_data_out; // output: number of samples returned (== peaks_out if successful)
  void *extra_requested_data;

  REAPER_PeakGet_Interface *__peakgetter;
#ifdef __LP64__
  int *exp[27];
#else
  int *exp[26];
#endif

  static inline int extra_blocksize(int extra_requested_data_type)
  {
    switch (extra_requested_data_type) 
    {
      case PEAKINFO_EXTRADATA_SPECTRAL1: return SPECTRAL1_BYTES;
      case PEAKINFO_EXTRADATA_SPECTROGRAM1: return SPECTROGRAM1_BLOCKSIZE_BYTES;
      case PEAKINFO_EXTRADATA_MIDITEXT: return MIDITEXT_BYTES;
      case PEAKINFO_EXTRADATA_LOUDNESS_DEPRECATED: return LOUDNESS_DEPRECATED_BYTES;
      case PEAKINFO_EXTRADATA_LOUDNESS_RAW: return LOUDNESS_RAW_BYTES;
      case PEAKINFO_EXTRADATA_LOUDNESS_INTERNAL: return LOUDNESS_INTERNAL_BYTES;
    }
    return 0;
  }

  enum {
    SPECTROGRAM1_BLOCKSIZE_BYTES=128 * 3 / 2, // 128 bins, 12 bits each (MSB1, (LSN1<<4)|LSN2, MSB2)
    SPECTRAL1_BYTES=4, // one LE int per channel per sample spectral info: low 15 bits frequency, next 14 bits density (16383=tonal, 0=noise, 12288 = a bit noisy)
    MIDITEXT_BYTES=1, // at most one character per pixel
    LOUDNESS_DEPRECATED_BYTES=4, // 4 byte LE integer: LUFS-M low 12 bits, LUFS-S next 12 bits, 0-3000 valid: ex: 0 means -150.0 LU (consider this -inf), 1500 means +0 LU - loudness values returned for each channel even if the calculation is for all channels combined
    LOUDNESS_RAW_BYTES=8, // 4 byte LE float LUFS-M low 32 bits, 4 byte LE float LUFS-S high 32 bits:
         // stored values are a windowed, weighted mean square of samples for each channel,
         // which must be combined to calculate total loudness. specifically,
         // the stored values are z(i) in formula 2 in this document:
         // https://www.itu.int/dms_pubrec/itu-r/rec/bs/R-REC-BS.1770-0-200607-S!!PDF-E.pdf
    LOUDNESS_INTERNAL_BYTES=4, // REAPER internal use only
  };

} PCM_source_peaktransfer_t;


// used to update MIDI sources with new events during recording
typedef struct _REAPER_midi_realtime_write_struct_t
{
  double global_time;
  double global_item_time;
  double srate;
  int length; // length in samples
  int overwritemode; // 0=overdub, 1=replace, 
                     // -1 = literal (do nothing just add), -2 = literal, do not apply default curves
                     // 65536+(16 bit mask) = replace notes on just these channels (touch-replace)
  MIDI_eventlist *events;
  double item_playrate;

  double latency;

  unsigned int *overwrite_actives; // [16(note)+16(CC)+16(poly AT)][4]; only used when overwritemode is >0
                                   // CC: 127=pitch, 126=program, 125=channel pressure

  double do_not_quantize_past_sec; // amount in future that quantizing should never move things past (or 0 for not used)
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
    virtual const char *GetFileName() { return NULL; } // return NULL if no filename (not purely a file)
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

typedef struct _REAPER_cue
{
  int m_id; // ignored for PCM_SINK_EXT_ADDCUE, populated for PCM_SOURCE_EXT_ENUMCUES
  double m_time;
  double m_endtime;
  bool m_isregion;
  char *m_name; // can be NULL if unnamed
  int m_flags; // &1:DEPRECATED caller must call Extended(PCM_SOURCE_EXT_ENUMCUES, -1, &cue, 0) when finished, &2:time is QN, &0x10000:write cue regardless of sink settings, &4:is chapter, &(8|16)=8:has low confidence cue name, &(8|16)=16:has medium confidence cue name, &(8|16)=24:has high confidence cue name
  char resvd[124]; // future expansion -- should be 0
} REAPER_cue;

typedef struct _REAPER_slice
{
  PCM_source* m_sliceSrc;
  double m_beatSnapOffset;
  int flag; // &1=only return beatsnapoffset, not slicesrc
  char resvd[124];  // future expansion -- should be 0
} REAPER_slice;

typedef struct _REAPER_inline_positioninfo
{
  double draw_start_time; // project time at pixel start of draw
  int draw_start_y;       // if y-scroll is partway into the item, positive pixel value
  double pixels_per_second;

  int width, height; // width and height of view of the item. if doing a partial update this may be larger than the bitmap passed in
  int mouse_x, mouse_y; // valid only on mouse/key/setcursor/etc messages

  void *extraParms[8];
  // WM_KEYDOWN handlers can use MSG *msg = (MSG *)extraParms[0]
  // WM_SETCURSOR handlers should set *extraParms[0] = hcursor
} REAPER_inline_positioninfo;


typedef struct _REAPER_tempochg
{
  double timepos, qnpos, bpm;
  int tsnum, tsdenom;
  int flag; // &1=linear tempo change, &2=internal use
} REAPER_tempochg;


#define PCM_SOURCE_EXT_INLINEEDITOR 0x100  /* parm1 = (void *)(INT_PTR)message, parm2/parm3 = parms

                             note: for the WM_* constants, you can use windows.h if on Windows, and SWELL's definitions if on other platforms
                             note: for LICE_IBitmap interface, you can use LICE's definition
                             for SWELL and LICE, see Cockos WDL -- https://www.cockos.com/wdl

                             NOTE: fx-embed documentation is now in reaper_plugin_fx_embed.h

                                              messages:
                                                0 = query if editor is available/supported. returns <0 if supported but unavailable, >0 if available, 0=if not supported
                                                WM_CREATE to create the editor instance
                                                WM_DESTROY to destroy the editor instance (nonzero if success)

                                                WM_LBUTTON*, WM_RBUTTON*, WM_MOUSEMOVE, WM_MOUSEWHEEL -- parm2=rsvd, parm3= REAPER_inline_positioninfo)
                                                  these can return any combination of:
                                                    REAPER_INLINE_RETNOTIFY_INVALIDATE
                                                    REAPER_INLINE_RETNOTIFY_SETCAPTURE
                                                    REAPER_INLINE_RETNOTIFY_SETFOCUS

                                               WM_KEYDOWN -- parm3=REAPER_inline_positioninfo*, MSG *kbmsg = (MSG *)rec->extraParms[0]
                                                  return nonzero to eat the key. can return REAPER_INLINE_RETNOTIFY_INVALIDATE.

                                               WM_SETCURSOR -- parm3=REAPER_inline_positioninfo*
                                                 --  *rec->extraParms[0] = hcursor

                                              paint messages: parm2 = LICE_IBitmap *, parm3 = (REAPER_inline_positioninfo*).
                                                  WM_ERASEBKGND -- draw first pass -- should return 1 if supported
                                                  WM_PAINT -- draw second pass     -- should return 1 if paint supported
                                                  WM_NCPAINT -- draw third pass

                                               Notes on Retina/HiDPI:
                                                     on macOS retina, (int)LICE_IBitmap::Extended(LICE_EXT_GET_SCALING,NULL) may return 512. in this case LICE will internally render things double-sized
                                                     on other platforms HiDPI, if (int)LICE_IBitmap::Extended(LICE_EXT_GET_ADVISORY_SCALING,NULL) returns nonzero, then it is a 24.8 scale factor which things
                                                     should be scaled by.

                                               */

#define REAPER_INLINE_RETNOTIFY_INVALIDATE 0x1000000 // want refresh of display
#define REAPER_INLINE_RETNOTIFY_SETCAPTURE 0x2000000 // setcapture
#define REAPER_INLINE_RETNOTIFY_SETFOCUS   0x4000000 // set focus to item
#define REAPER_INLINE_RETNOTIFY_NOAUTOSCROLL 0x8000000 // modifier only valid when setcapture set

#define REAPER_INLINEFLAG_SHOWALLTAKES          0x1000 // only valid as a return value flag for message=0, display in inactive take lanes
#define REAPER_INLINEFLAG_WANTOVERLAYEDCONTROLS 0x4000 // only valid as a return value flag for message=0, to have fades/etc still drawn over like normal





#define PCM_SOURCE_EXT_PROJCHANGENOTIFY 0x2000 // parm1 = nonzero if activated project, zero if deactivated project

#define PCM_SOURCE_EXT_OPENEDITOR 0x10001 // parm1=hwnd, implementation dependent parm2/parm3
#define PCM_SOURCE_EXT_GETEDITORSTRING 0x10002 // parm1=index (0 or 1), parm2=(const char**)desc, optional parm3=(int*)has_had_editor
#define PCM_SOURCE_EXT_DEPRECATED_1 0x10003 // was PCM_SOURCE_EXT_CLOSESECONDARYSRC 
#define PCM_SOURCE_EXT_SETITEMCONTEXT 0x10004 // parm1=MediaItem*,  parm2=MediaItem_Take*
#define PCM_SOURCE_EXT_ADDMIDIEVENTS 0x10005 // parm1=pointer to midi_realtime_write_struct_t, nch=1 for replace, =0 for overdub, parm2=midi_quantize_mode_t* (optional)
#define PCM_SOURCE_EXT_GETASSOCIATED_RPP 0x10006 // parm1=pointer to char* that will receive a pointer to the string
#define PCM_SOURCE_EXT_GETMETADATA 0x10007 // parm1=pointer to name string, parm2=pointer to buffer, parm3=(int)buffersizemax. returns length used. defined strings are "TITLE", "ARTIST", "ALBUM", "TRACKNUMBER", "YEAR", "GENRE", "COMMENT", "DESC", "BPM", "KEY", "DB_CUSTOM"
#define PCM_SOURCE_EXT_SETASSECONDARYSOURCE 0x10008  // parm1=optional pointer to src (same subtype as receiver), if supplied, set the receiver as secondary src for parm1's editor, if not supplied, receiver has to figure out if there is an appropriate editor open to attach to, parm2/3 impl defined
#define PCM_SOURCE_EXT_SHOWMIDIPREVIEW 0x10009  // parm1=(MIDI_eventlist*), can be NULL for all-notes-off (also to check if this source supports showing preview at this moment)
#define PCM_SOURCE_EXT_SEND_EDITOR_MSG 0x1000A  // impl defined parameters
#define PCM_SOURCE_EXT_SETSECONDARYSOURCELIST 0x1000B // parm1=(PCM_source**)sourcelist, parm2=list size, parm3=close any existing src not in the list
#define PCM_SOURCE_EXT_ISOPENEDITOR 0x1000C // returns 1 if this source is currently open in an editor, 2 if open in a secondary editor. parm1=1 to close. parm2=(int*)&flags to get extra flags (&1=editor is currently hidden)
#define PCM_SOURCE_EXT_SETEDITORGRID 0x1000D // parm1=(double*)griddiv: 0.25=quarter note, 1.0/3.0=half note triplet, etc. parm2=int* swingmode(1=swing), parm3=double*swingamt
#define PCM_SOURCE_EXT_GETITEMCONTEXT 0x10010 // parm1=MediaItem**, parm2=MediaItem_Take**, parm3=MediaTrack**
#define PCM_SOURCE_EXT_GETALLMETADATA_DEPRECATED 0x10011 // no longer supported
#define PCM_SOURCE_EXT_GETBITRATE 0x10012 // parm1=(double*)bitrate, if different from samplerate*channels*bitdepth/length
#define PCM_SOURCE_EXT_ENUMMETADATA 0x10013 // parm1=(int)index, parm2=(const char**)key, parm3=(const char**)value. enumerates all metadata, returns 0 when no more metadata exists
#define PCM_SOURCE_EXT_GETINFOSTRING 0x10014 // parm1=(char*)buffer, parm2=(int)buffer length, return the data that would be displayed in the properties window
#define PCM_SOURCE_EXT_CONFIGISFILENAME 0x20000
#define PCM_SOURCE_EXT_WRITEMETADATA_DEPRECATED 0x20007 // no longer supported
#define PCM_SOURCE_EXT_WRITE_METADATA 0x20008 // parm1=char* new file name, parm2=(char**)NULL-terminated array of key,value,key2,value2,... pointers, parm3=flags (&1=merge, &2=do not allow update in-place). returns 1 if successfully generated a new file, 2 if original file was updated
#define PCM_SOURCE_EXT_GETBPMANDINFO 0x40000 // parm1=pointer to double for bpm. parm2=pointer to double for snap/downbeat offset (seconds).
#define PCM_SOURCE_EXT_GETNTRACKS 0x80000 // for midi data, returns number of tracks that would have been available. optional parm1=(int*)mask of channels available, mask&(1<<16)=metadata
#define PCM_SOURCE_EXT_GETTITLE   0x80001 // parm1=(char**)title (string persists in plugin)
#define PCM_SOURCE_EXT_ENUMTEMPOMAP 0x80002 // parm1=index, parm2=pointer to REAPER_tempochg, returns 0 if no tempo map or enumeration complete
#define PCM_SOURCE_EXT_WANTOLDBEATSTYLE 0x80003
#define PCM_SOURCE_EXT_GETNOTATIONSETTINGS 0x80004 // parm1=(int)what, (what==0) => parm2=(double*)keysigmap, parm3=(int*)keysigmapsize; (what==1) => parm2=(int*)display transpose semitones, (what==2) => parm2=(char*)clef1, parm3=(char*)clef2
#define PCM_SOURCE_EXT_RELOADTRACKDATA 0x80005 // internal use
#define PCM_SOURCE_EXT_WANT_TRIM 0x90001 // parm1=(int64*)total number of decoded samples after trimming, parm2=(int*)number of samples to trim from start, parm3=(int*)number of samples to trim from end
#define PCM_SOURCE_EXT_WANTTRIM_DEPRECATED 0x90002 // no longer supported
#define PCM_SOURCE_EXT_TRIMITEM 0x90003 // parm1=lrflag, parm2=double *{position,length,startoffs,rate}
#define PCM_SOURCE_EXT_EXPORTTOFILE 0x90004 // parm1=output filename, only currently supported by MIDI but in theory any source could support this
#define PCM_SOURCE_EXT_ENUMCUES 0x90005 // DEPRECATED, use PCM_SOURCE_EXT_ENUMCUES_EX instead.  parm1=(int) index of cue to get (-1 to free cue), parm2=(optional)REAPER_cue **.  Returns 0 and sets parm2 to NULL when out of cues. return value otherwise is how much to advance parm2 (1, or 2 usually)
#define PCM_SOURCE_EXT_ENUMCUES_EX 0x90016 // parm1=(int) index of cue (source must provide persistent backing store for cue->m_name), parm2=(REAPER_cue*) optional. Returns 0 when out of cues, otherwise returns how much to advance index (1 or 2 usually). 
// a PCM_source may be the parent of a number of beat-based slices, if so the parent should report length and nchannels only, handle ENUMSLICES, and be deleted after the slices are retrieved
#define PCM_SOURCE_EXT_ENUMSLICES 0x90006 // parm1=(int*) index of slice to get, parm2=REAPER_slice* (pointing to caller's existing slice struct), parm3=(double*)bpm. if parm2 passed in zero, returns the number of slices. returns 0 if no slices or out of slices.
#define PCM_SOURCE_EXT_ENDPLAYNOTIFY 0x90007 // notify a source that it can release any pooled resources
#define PCM_SOURCE_EXT_SETPREVIEWTEMPO 0x90008 // parm1=(double*)bpm, only meaningful for MIDI or slice-based source media; bpm==0 to follow project tempo changes

enum { RAWMIDI_NOTESONLY=1, RAWMIDI_UNFILTERED=2, RAWMIDI_CHANNELFILTER=3 }; // if RAWMIDI_CHANNELFILTER, flags>>4 is a mask of which channels to play
#define PCM_SOURCE_EXT_GETRAWMIDIEVENTS 0x90009 // parm1 = (PCM_source_transfer_t *), parm2 = RAWMIDI flags

#define PCM_SOURCE_EXT_SETRESAMPLEMODE 0x9000A // parm1= mode to pass to resampler->Extended(RESAMPLE_EXT_SETRSMODE,mode,0,0)
#define PCM_SOURCE_EXT_NOTIFYPREVIEWPLAYPOS 0x9000B // parm1 = ptr to double of play position, or NULL if stopped
#define PCM_SOURCE_EXT_SETSIZE 0x9000C // parm1=(double*)startpos, parm2=(double*)endpos, parm3=flags. Start can be negative. Receiver may adjust start/end to avoid erasing content, in which case the adjusted values are returned in parm1 and parm2. parm3/flags: 1 if start/end in QN (always the case now). 2=resize even pooled items
#define PCM_SOURCE_EXT_GETSOURCETEMPO 0x9000D // parm1=(double*)bpm, parm2=(int*)timesig_numerator<<8|timesig_denominator, parm3=(double*)current preview tempo if applicable. this is for reporting purposes only, does not necessarily mean the media should be adjusted (as PCM_SOURCE_EXT_GETBPMANDINFO means)
#define PCM_SOURCE_EXT_ISABNORMALAUDIO  0x9000E // return 1 if rex, video, etc (meaning file export will just copy file directly rather than trim/converting)
#define PCM_SOURCE_EXT_GETPOOLEDMIDIID 0x9000F // parm1=(char*)id, parm2=(int*)pool user count, parm3=(MediaItem_Take**)firstuser
#define PCM_SOURCE_EXT_REMOVEFROMMIDIPOOL 0x90010 
#define PCM_SOURCE_EXT_GETHASH 0x90011 // parm1=(WDL_UINT64*)hash (64-bit hash of the source data)
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
#define PCM_SOURCE_EXT_GETSCALE 0x90024 // parm1=unsigned int: &0xF=pitch (0=C), &0x10=root, &0x20=min2, &0x40=maj2, &0x80=min3, &0xF0=maj3, &0x100=4, etc) ; parm2=(char*)name (optional), parm3=int size of name buffer
#define PCM_SOURCE_EXT_SELECTCONTENT 0x90025 // parm1=1 to select, 0 to deselect
#define PCM_SOURCE_EXT_GETGRIDINFO 0x90026 // parm1=(double*)snap grid size, parm2=(double*)swing strength, parm3=(double*)note insert length, -1 if follows grid size
#define PCM_SOURCE_EXT_SORTMIDIEVTS 0x9027
#define PCM_SOURCE_EXT_MIDI_COMPACTPHRASES 0x90028 // compact the notation phrase ID space
#define PCM_SOURCE_EXT_GETSETALLMIDI 0x90029 // parm1=(unsigned char*)data buffer, parm2=(int*)buffer length in bytes, parm2=(1:set, 0:get). Buffer is a list of { int offset, char flag, int msglen, unsigned char msg[] }. offset: MIDI ticks from previous event, flag: &1=selected &2=muted, msglen: byte length of msg (usually 3), msg: the MIDI message.
#define PCM_SOURCE_EXT_DISABLESORTMIDIEVTS 0x90030 // disable sorting for PCM_SOURCE_EXT_GETSETMIDIEVT until PCM_SOURCE_EXT_SORTMIDIEVTS is called
#define PCM_SOURCE_EXT_GETPOOLEDMIDIID2 0x90031 // parm1=(GUID*)id, parm2=(int*)pool user count, parm3=(MediaItem_Take**)firstuser
#define PCM_SOURCE_EXT_GETSETMIDICHANFILTER 0x90032 // parm1=(int*)filter: filter&(1<<n) to play channel n, parm2=(int)set: 0 to get, 1 to set
#define PCM_SOURCE_EXT_REFRESH_EDITORS 0x90033 // synchronously refresh any open editors
#define PCM_SOURCE_EXT_GET_LYRICS      0x90040 // parm1=buffer, parm2= &len, parm3=(INT_PTR)flag - see GetTrackMIDILyrics
#define PCM_SOURCE_EXT_SET_LYRICS      0x90041 // parm1=nul term buffer
#define PCM_SOURCE_EXT_GETLAPPING 0xC0100 // parm1 = ReaSample buffer, parm2=(INT_PTR)maxlap, returns size of lapping returned. usually not supported. special purpose.
#define PCM_SOURCE_EXT_SET_PREVIEW_POS_OVERRIDE 0xC0101 // parm1 = (double *)&tickpos, tickpos<0 for no override
#define PCM_SOURCE_EXT_SET_PREVIEW_LOOPCNT 0xC0102 // parm1 = (INT64*)&decoding_loopcnt, valid only for the immediately following GetSamples(), only in track preview contexts, when not using buffering source


// register with Register("pcmsrc",&struct ... and unregister with "-pcmsrc"
typedef struct _REAPER_pcmsrc_register_t {
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

typedef struct _REAPER_midi_quantize_mode_t
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
#define PCM_SINK_EXT_DONE 0x80001 // parm1 = optional HWND of the render dialog in case the sink needs more user interaction, parm2 = optional (char**)NULL-terminated array of key,value,key2,value2,... pointers if the sink supports updating metadata after write (may require the caller to have set padded metadata via SetCurrentSinkMetadata before creating the sink)
#define PCM_SINK_EXT_VERIFYFMT 0x80002 // parm1=int *srate, parm2= int *nch. plugin can override (and return 1!!)
#define PCM_SINK_EXT_SETQUANT 0x80003 // parm1 = (midi_quantize_mode_t*), or NULL to disable
#define PCM_SINK_EXT_SETRATE 0x80004 // parm1 = (double *) rateadj
#define PCM_SINK_EXT_GETBITDEPTH 0x80005 // parm1 = (int*) bitdepth (return 1 if supported)
#define PCM_SINK_EXT_ADDCUE 0x80006 // parm1=(PCM_cue*)cue OR parm2=(double*)transient position
#define PCM_SINK_EXT_SETCURBLOCKTIME 0x80007 // parm1 = (double *) project position -- called before each WriteDoubles etc
#define PCM_SINK_EXT_IS_VIDEO 0x80008 // deprecated/unused

typedef struct _REAPER_pcmsink_register_t // register using "pcmsink"
{
  unsigned int (*GetFmt)(const char **desc);

  const char *(*GetExtension)(const void *cfg, int cfg_l);
  HWND (*ShowConfig)(const void *cfg, int cfg_l, HWND parent);
  PCM_sink *(*CreateSink)(const char *filename, void *cfg, int cfg_l, int nch, int srate, bool buildpeaks);

} pcmsink_register_t;

typedef struct _REAPER_pcmsink_register_ext_t // register using "pcmsink_ext"
{
  pcmsink_register_t sink; 

  // for extended calls that refer to the generic type of sink, rather than a specific instance of a sink
  int (*Extended)(int call, void* parm1, void* parm2, void* parm3); 


  char expand[256];
} pcmsink_register_ext_t;

// supported via pcmsink_register_ext_t::Extended:
#define PCMSINKEXT_GETFORMATDESC 0x80000 // parm1=(void*)cfg, parm2=(int)cfglen, parm3=(const char*)retstring 
#define PCMSINKEXT_GETFORMATDATARATE 0x80001 // parm1=(void*)cfg, parm2=(int)cfglen, parm3 = int[] {channels, samplerate}
#define PCMSINKEXT_GETFORMATBITDEPTH 0x80002 // parm1=(void*)cfg, parm2=(int)cfglen, returns bit depth if supported (negative with effective size if FP)
#define PCMSINKEXT_GETFORMATSTREAMDESC 0x80003 // parm1=(void*)cfg, parm2=(int)cfglen, parm3=(int*)streams (&1=has video, &2=has audio), returns 1 if supported

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
#define RESAMPLE_EXT_PREALLOC 0x1002 // parm1 = nch, parm2=input blocksize, parm3=output blocksize
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
#define REAPER_PITCHSHIFT_EXT_GET_CHAN_LIMIT 0x100 // return channel limit if any
#define REAPER_PITCHSHIFT_EXT_SET_OUTPUT_BPM 0x200 // parm1 = *(double *)bpm
#define REAPER_PITCHSHIFT_EXT_WANT_OUTPUT_BPM 0x201 // returns 1 if desired


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


#endif // __cplusplus



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
  // -10 (macOS only) to process event raw
  // -20 (Windows only) to passed to the window, even if it is WM_SYSKEY*/VK_MENU which would otherwise be dropped
  // -666 to force it to the main window's accel table (with the exception of ESC)
  // -667 to force it to the main window's accel table, even if in a text field (5.24+ or so)
  int (*translateAccel)(MSG *msg, accelerator_register_t *ctx); 
  bool isLocal; // must be TRUE, now (false is no longer supported, heh)
  void *user;
} accelerator_register_t;


/*
** custom_action_register_t allows you to register ("custom_action") an action or a reascript into a section of the action list
** register("custom_action",ca) will return the command ID (instance-dependent but unique across all sections), 
** or 0 if failed (e.g dupe idStr for actions, or script not found/supported, etc)
** for actions, the related callback should be registered with "hookcommand2"
*/

typedef struct _REAPER_custom_action_register_t
{
  int uniqueSectionId; // 0/100=main/main alt, 32063=media explorer, 32060=midi editor, 32061=midi event list editor, 32062=midi inline editor, etc
  const char* idStr; // must be unique across all sections for actions, NULL for reascripts (automatically generated)
  const char* name; // name as it is displayed in the action list, or full path to a reascript file
  void *extra; // reserved for future use
} custom_action_register_t;


/*
** gaccel_register_t allows you to register ("gaccel") an action into the main keyboard section action list, and at the same time
** a default binding for it (accel.cmd is the command ID, desc is the description, and accel's other parameters are the
** key to bind. 
** 7.07+ - can use "gaccel_global" or "gaccel_globaltext" to make the accel registered as a global by default
*/

typedef struct _REAPER_gaccel_register_t
{
  ACCEL accel; // key flags/etc represent default values (user may customize)
  const char *desc; // description (for user customizability)
} gaccel_register_t; // use "gaccel"

/*
** action_help_t lets you register help text ("action_help") for an action, mapped by action name
** (a "help" plugin could register help text for Reaper built-in actions)
*/

typedef struct _REAPER_action_help_t
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

typedef struct _REAPER_editor_register_t // register with "editor"
{
  int (*editFile)(const char *filename, HWND parent, int trackidx); // return TRUE if handled for this file
  const char *(*wouldHandle)(const char *filename); // return your editor's description string

} editor_register_t;


/*
** Project import registration.
** 
** Implemented as a your-format->RPP converter, allowing you to generate directly to a ProjectStateContext
*/
typedef struct _REAPER_project_import_register_t // register with "projectimport"
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

typedef struct _REAPER_KbdCmd
{
  DWORD cmd;  // action command ID
  const char *text; // description of action
} KbdCmd;

typedef struct _REAPER_KbdKeyBindingInfo
{
  int key;  // key identifier
  int cmd;  // action command ID
  int flags; // key flags
} KbdKeyBindingInfo;



typedef struct _REAPER_KbdSectionInfo
{
  int uniqueID; // 0=main, < 0x10000000 for cockos use only plzkthx
  const char *name; // section name

  KbdCmd *action_list;   // list of assignable actions
  int action_list_cnt;

  const KbdKeyBindingInfo *def_keys; // list of default key bindings
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



typedef struct _REAPER_preview_register_t
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
  SCREENSET_ACTION_GETHWND = 0, // returns HWND of screenset window. If searching for a specific window, it will be passed in actionParm

  SCREENSET_ACTION_IS_DOCKED = 1, // returns 1 if docked
  SCREENSET_ACTION_SWITCH_DOCK = 4, //dock if undocked and vice-versa

  SCREENSET_ACTION_LOAD_STATE=0x100, // load state from actionParm (of actionParmSize). if both are NULL, hide.
  SCREENSET_ACTION_SAVE_STATE,  // save state to actionParm, max length actionParmSize (will usually be max(4096, value_returned by SCREENSET_ACTION_WANT_STATE_SIZE)), return length actually used
  SCREENSET_ACTION_WANT_STATE_SIZE, // returns desired size for SCREENSET_ACTION_SAVE_STATE, may or may not be fulfilled!
};
typedef LRESULT (*screensetNewCallbackFunc)(int action, const char *id, void *param, void *actionParm, int actionParmSize);

// This is managed using screenset_registerNew(), screenset_unregister(), etc


/*
** MIDI hardware device access.
**
*/

#ifdef __cplusplus

class midi_Output
{
public:
  virtual ~midi_Output() {}

  virtual void BeginBlock() { }  // outputs can implement these if they wish to have timed block sends
  virtual void EndBlock(int length, double srate, double curtempo) { }
  virtual void SendMsg(MIDI_event_t *msg, int frame_offset)=0; // frame_offset can be <0 for "instant" if supported
  virtual void Send(unsigned char status, unsigned char d1, unsigned char d2, int frame_offset)=0; // frame_offset can be <0 for "instant" if supported

  virtual void Destroy() { delete this; } // allows implementations to do asynchronous destroy (5.95+)

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

  virtual void Destroy() { delete this; } // allows implementations to do asynchronous destroy (5.95+)
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
#define CSURF_EXT_SETFXPARAM_RECFX 0x00010018 // parm1=(MediaTrack*)track, parm2=(int*)(fxidx<<16|paramidx), parm3=(double*)normalized value
#define CSURF_EXT_SETBPMANDPLAYRATE 0x00010009 // parm1=*(double*)bpm (may be NULL), parm2=*(double*)playrate (may be NULL)
#define CSURF_EXT_SETLASTTOUCHEDFX 0x0001000A // parm1=(MediaTrack*)track, parm2=(int*)mediaitemidx (may be NULL), parm3=(int*)fxidx. all parms NULL=clear last touched FX
#define CSURF_EXT_SETFOCUSEDFX 0x0001000B // parm1=(MediaTrack*)track, parm2=(int*)mediaitemidx (may be NULL), parm3=(int*)fxidx. all parms NULL=clear focused FX
#define CSURF_EXT_SETLASTTOUCHEDTRACK 0x0001000C // parm1=(MediaTrack*)track
#define CSURF_EXT_SETMIXERSCROLL 0x0001000D // parm1=(MediaTrack*)track, leftmost track visible in the mixer
#define CSURF_EXT_SETPAN_EX 0x0001000E // parm1=(MediaTrack*)track, parm2=(double*)pan, parm3=(int*)mode 0=v1-3 balance, 3=v4+ balance, 5=stereo pan, 6=dual pan. for modes 5 and 6, (double*)pan points to an array of two doubles.  if a csurf supports CSURF_EXT_SETPAN_EX, it should ignore CSurf_SetSurfacePan.
#define CSURF_EXT_SETRECVVOLUME 0x00010010 // parm1=(MediaTrack*)track, parm2=(int*)recvidx, parm3=(double*)volume
#define CSURF_EXT_SETRECVPAN 0x00010011 // parm1=(MediaTrack*)track, parm2=(int*)recvidx, parm3=(double*)pan
#define CSURF_EXT_SETFXOPEN 0x00010012 // parm1=(MediaTrack*)track, parm2=(int*)fxidx, parm3=0 if UI closed, !0 if open
#define CSURF_EXT_SETFXCHANGE 0x00010013 // parm1=(MediaTrack*)track, whenever FX are added, deleted, or change order. flags=(INT_PTR)parm2, &1=rec fx
#define CSURF_EXT_SETPROJECTMARKERCHANGE 0x00010014 // whenever project markers are changed
#define CSURF_EXT_TRACKFX_PRESET_CHANGED  0x00010015 // parm1=(MediaTrack*)track, parm2=(int*)fxidx (6.13+ probably)
#define CSURF_EXT_SUPPORTS_EXTENDED_TOUCH 0x00080001 // returns nonzero if GetTouchState can take isPan=2 for width, etc
#define CSURF_EXT_MIDI_DEVICE_REMAP 0x00010099 // parm1 = isout, parm2 = old idx, parm3 = new idx

typedef struct _REAPER_reaper_csurf_reg_t
{
  const char *type_string; // simple unique string with only A-Z, 0-9, no spaces or other chars
  const char *desc_string; // human readable description

  IReaperControlSurface *(*create)(const char *type_string, const char *configString, int *errStats); // errstats gets |1 if input error, |2 if output error
  HWND (*ShowConfig)(const char *type_string, HWND parent, const char *initConfigString); 
} reaper_csurf_reg_t; // register using "csurf"/"-csurf"

// note you can also add a control surface behind the scenes with "csurf_inst" (IReaperControlSurface*)instance

#endif // __cplusplus


#ifndef UNDO_STATE_ALL
#define UNDO_STATE_ALL 0xFFFFFFFF
#define UNDO_STATE_TRACKCFG 1 // has track/master vol/pan/routing, routing/hwout envelopes too
#define UNDO_STATE_FX 2  // track/master fx
#define UNDO_STATE_ITEMS 4  // track items and linkedlanes
#define UNDO_STATE_MISCCFG 8 // loop selection, markers, regions, extensions!
#define UNDO_STATE_FREEZE 16 // freeze state -- note that isfreeze is used independently, this is only used for the undo system to serialize the already frozen state
#define UNDO_STATE_TRACKENV 32 // non-FX envelopes only
#define UNDO_STATE_FXENV 64   // FX envelopes, implied by UNDO_STATE_FX too
#define UNDO_STATE_POOLEDENVS 128 // contents of pooled envs -- not position, length, rate etc of pooled env instances, which is part of envelope state
#endif

#ifndef IS_MSG_VIRTKEY
  #ifdef _WIN32
    #define IS_MSG_VIRTKEY(msg) ((msg)->message != WM_CHAR)
  #else
    #define IS_MSG_VIRTKEY(msg) ((msg)->lParam&FVIRTKEY)
  #endif
#endif
#define IS_MSG_FKEY(msg) ((msg)->wParam >= VK_F1 && (msg)->wParam <= VK_F24 && IS_MSG_VIRTKEY(msg))

#define WDL_FILEWRITE_ON_ERROR(is_full) update_disk_counters(0,-101010110 - ((is_full) ? 1 : 0));

#define REAPER_MAX_CHANNELS 128

#endif//_REAPER_PLUGIN_H_
