#include "Log.h"
#include "stdio.h"
#include "string.h"
#include "time.h"
#include "ctype.h"

#define TRACETOSTDOUT

#ifdef OS_WIN
#define LOGFILE "C:\\IPlugLog.txt" // TODO: what if no write permissions?

void DBGMSG(const char *format, ...)
{
  char    buf[4096], *p = buf;
  va_list args;
  int     n;

  va_start(args, format);
  n = _vsnprintf(p, sizeof buf - 3, format, args); // buf-3 is room for CR/LF/NUL
  va_end(args);

  p += (n < 0) ? sizeof buf - 3 : n;

  while ( p > buf  &&  isspace(p[-1]) )
    *--p = '\0';

  *p++ = '\r';
  *p++ = '\n';
  *p   = '\0';

  OutputDebugString(buf);
}

#else // OSX
  #define LOGFILE "IPlugLog.txt" // will get put on Desktop
#endif

const int TXTLEN = 1024;

// _vsnsprintf

#define VARARGS_TO_STR(str) { \
  try { \
    va_list argList;  \
    va_start(argList, format);  \
    int i = vsnprintf(str, TXTLEN-2, format, argList); \
    if (i < 0 || i > TXTLEN-2) {  \
      str[TXTLEN-1] = '\0';  \
    } \
    va_end(argList);  \
  } \
  catch(...) {  \
    strcpy(str, "parse error"); \
  } \
  strcat(str, "\r\n"); \
}

struct LogFile
{
  FILE* mFP;

  LogFile()
  {
    #ifdef _WIN32
    mFP = fopen(LOGFILE, "w");
    #else
    char logFilePath[100];
    char* home = getenv("HOME");
    sprintf(logFilePath, "%s/Desktop/%s", home, LOGFILE);
    mFP = fopen(logFilePath, "w");
    #endif
    assert(mFP);
  }

  ~LogFile()
  {
    fclose(mFP);
    mFP = 0;
  }
};

Timer::Timer()
{
  mT = clock();
}

bool Timer::Every(double sec)
{
  if (clock() - mT > sec * CLOCKS_PER_SEC)
  {
    mT = clock();
    return true;
  }
  return false;
};

// Needs rewriting for WDL.
//StrVector ReadFileIntoStr(WDL_String* pFileName)
//{
//  WDL_PtrList<WDL_String> lines;
//  if (!strlen(pFileName->Get())) {
//    WDL_String line;
//    std::ifstream ifs(pFileName->Get());
//    if (ifs) {
//      while (std::getline(ifs, line, '\n')) {
//        if (!line.empty()) {
//          lines.push_back(line);
//        }
//      }
//      ifs.close();
//    }
//  }
//  return lines;
//}

// Needs rewriting for WDL.
//int WriteBufferToFile(const DBuffer& buf, const std::string& fileName)
//{
//    std::ofstream ofs(fileName.c_str());
//    if (ofs) {
//        for (int i = 0, p = buf.Pos(); i < buf.Size(); ++i, ++p) {
//            ofs << buf[p] << ' ';
//        }
//        ofs << '\n';
//        ofs.close();
//        return buf.Size();
//    }
//    return 0;
//}

bool IsWhitespace(char c)
{
  return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}

void ToLower(char* cDest, const char* cSrc)
{
  int i, n = (int) strlen(cSrc);
  for (i = 0; i < n; ++i)
  {
    cDest[i] = tolower(cSrc[i]);
  }
  cDest[i] = '\0';
}

const char* CurrentTime()
{
  time_t t = time(0);
  tm* pT = localtime(&t);

	char cStr[32];
	strftime(cStr, 32, "%Y%m%d %H:%M ", pT);

	int tz = 60 * pT->tm_hour + pT->tm_min;
	int yday = pT->tm_yday;
	pT = gmtime(&t);
	tz -= 60 * pT->tm_hour + pT->tm_min;
	yday -= pT->tm_yday;
	if (yday != 0)
	{
		if (yday > 1) yday = -1;
		else if (yday < -1) yday = 1;
		tz += 24 * 60 * yday;
	}
	int i = strlen(cStr);
	cStr[i++] = tz >= 0 ? '+' : '-';
	if (tz < 0) tz = -tz;
	sprintf(&cStr[i], "%02d%02d", tz / 60, tz % 60);
  
  static char sTimeStr[32];
  strcpy(sTimeStr, cStr);
  return sTimeStr;
}

void CompileTimestamp(const char* Mmm_dd_yyyy, const char* hh_mm_ss, WDL_String* pStr)
{
  pStr->Set("[");
  pStr->Append(Mmm_dd_yyyy);
  pStr->SetLen(7);
  pStr->DeleteSub(4, 1);
  pStr->Append(" ");
  pStr->Append(hh_mm_ss);
  pStr->SetLen(12);
  pStr->Append("]");
}

const char* AppendTimestamp(const char* Mmm_dd_yyyy, const char* hh_mm_ss, const char* cStr)
{
  static WDL_String str;
  str.Set(cStr);
  WDL_String tStr;
  CompileTimestamp(Mmm_dd_yyyy, hh_mm_ss, &tStr);
  str.Append(" ");
  str.Append(tStr.Get());
  return str.Get();
}

#if defined TRACER_BUILD

intptr_t GetOrdinalThreadID(intptr_t sysThreadID)
{
  static WDL_TypedBuf<intptr_t> sThreadIDs;
  int i, n = sThreadIDs.GetSize();
  intptr_t* pThreadID = sThreadIDs.Get();
  for (i = 0; i < n; ++i, ++pThreadID)
  {
    if (sysThreadID == *pThreadID)
    {
      return i;
    }
  }
  sThreadIDs.Resize(n + 1);
  *(sThreadIDs.Get() + n) = sysThreadID;
  return n;
}

#define MAX_LOG_LINES 16384
void Trace(const char* funcName, int line, const char* format, ...)
{
  static int sTrace = 0;
  if (sTrace++ < MAX_LOG_LINES)
  {
#ifndef TRACETOSTDOUT
    static LogFile sLogFile;
#endif
    static WDL_Mutex sLogMutex;
    char str[TXTLEN];
    VARARGS_TO_STR(str);

#ifdef TRACETOSTDOUT
    #ifdef OS_WIN
    DBGMSG("[%ld:%s:%d]%s", GetOrdinalThreadID(SYS_THREAD_ID), funcName, line, str);
    #else
    printf("[%ld:%s:%d]%s", GetOrdinalThreadID(SYS_THREAD_ID), funcName, line, str);
    #endif
#else
    WDL_MutexLock lock(&sLogMutex);
    fprintf(sLogFile.mFP, "[%ld:%s:%d]%s", GetOrdinalThreadID(SYS_THREAD_ID), funcName, line, str);
    fflush(sLogFile.mFP);
#endif
  }
}

#include "../../VST_SDK/aeffectx.h"
const char* VSTOpcodeStr(int opCode)
{
  switch (opCode)
  {
  case effOpen:
    return "effOpen";
  case effClose:
    return "effClose";
  case effSetProgram:
    return "effSetProgram";
  case effGetProgram:
    return "effGetProgram";
  case effSetProgramName:
    return "effSetProgramName";
  case effGetProgramName:
    return "effGetProgramName";
  case effGetParamLabel:
    return "effGetParamLabel";
  case effGetParamDisplay:
    return "effGetParamDisplay";
  case effGetParamName:
    return "effGetParamName";
  case __effGetVuDeprecated:
    return "__effGetVuDeprecated";
  case effSetSampleRate:
    return "effSetSampleRate";
  case effSetBlockSize:
    return "effSetBlockSize";
  case effMainsChanged:
    return "effMainsChanged";
  case effEditGetRect:
    return "effEditGetRect";
  case effEditOpen:
    return "effEditOpen";
  case effEditClose:
    return "effEditClose";
  case __effEditDrawDeprecated:
    return "__effEditDrawDeprecated";
  case __effEditMouseDeprecated:
    return "__effEditMouseDeprecated";
  case __effEditKeyDeprecated:
    return "__effEditKeyDeprecated";
  case effEditIdle:
    return "effEditIdle";
  case __effEditTopDeprecated:
    return "__effEditTopDeprecated";
  case __effEditSleepDeprecated:
    return "__effEditSleepDeprecated";
  case __effIdentifyDeprecated:
    return "__effIdentifyDeprecated";
  case effGetChunk:
    return "effGetChunk";
  case effSetChunk:
    return "effSetChunk";
  case effProcessEvents:
    return "effProcessEvents";
  case effCanBeAutomated:
    return "effCanBeAutomated";
  case effString2Parameter:
    return "effString2Parameter";
  case __effGetNumProgramCategoriesDeprecated:
    return "__effGetNumProgramCategoriesDeprecated";
  case effGetProgramNameIndexed:
    return "effGetProgramNameIndexed";
  case __effCopyProgramDeprecated:
    return "__effCopyProgramDeprecated";
  case __effConnectInputDeprecated:
    return "__effConnectInputDeprecated";
  case __effConnectOutputDeprecated:
    return "__effConnectOutputDeprecated";
  case effGetInputProperties:
    return "effGetInputProperties";
  case effGetOutputProperties:
    return "effGetOutputProperties";
  case effGetPlugCategory:
    return "effGetPlugCategory";
  case __effGetCurrentPositionDeprecated:
    return "__effGetCurrentPositionDeprecated";
  case __effGetDestinationBufferDeprecated:
    return "__effGetDestinationBufferDeprecated";
  case effOfflineNotify:
    return "effOfflineNotify";
  case effOfflinePrepare:
    return "effOfflinePrepare";
  case effOfflineRun:
    return "effOfflineRun";
  case effProcessVarIo:
    return "effProcessVarIo";
  case effSetSpeakerArrangement:
    return "effSetSpeakerArrangement";
  case __effSetBlockSizeAndSampleRateDeprecated:
    return "__effSetBlockSizeAndSampleRateDeprecated";
  case effSetBypass:
    return "effSetBypass";
  case effGetEffectName:
    return "effGetEffectName";
  case __effGetErrorTextDeprecated:
    return "__effGetErrorTextDeprecated";
  case effGetVendorString:
    return "effGetVendorString";
  case effGetProductString:
    return "effGetProductString";
  case effGetVendorVersion:
    return "effGetVendorVersion";
  case effVendorSpecific:
    return "effVendorSpecific";
  case effCanDo:
    return "effCanDo";
  case effGetTailSize:
    return "effGetTailSize";
  case __effIdleDeprecated:
    return "__effIdleDeprecated";
  case __effGetIconDeprecated:
    return "__effGetIconDeprecated";
  case __effSetViewPositionDeprecated:
    return "__effSetViewPositionDeprecated";
  case effGetParameterProperties:
    return "effGetParameterProperties";
  case __effKeysRequiredDeprecated:
    return "__effKeysRequiredDeprecated";
  case effGetVstVersion:
    return "effGetVstVersion";
  case effEditKeyDown:
    return "effEditKeyDown";
  case effEditKeyUp:
    return "effEditKeyUp";
  case effSetEditKnobMode:
    return "effSetEditKnobMode";
  case effGetMidiProgramName:
    return "effGetMidiProgramName";
  case effGetCurrentMidiProgram:
    return "effGetCurrentMidiProgram";
  case effGetMidiProgramCategory:
    return "effGetMidiProgramCategory";
  case effHasMidiProgramsChanged:
    return "effHasMidiProgramsChanged";
  case effGetMidiKeyName:
    return "effGetMidiKeyName";
  case effBeginSetProgram:
    return "effBeginSetProgram";
  case effEndSetProgram:
    return "effEndSetProgram";
  case effGetSpeakerArrangement:
    return "effGetSpeakerArrangement";
  case effShellGetNextPlugin:
    return "effShellGetNextPlugin";
  case effStartProcess:
    return "effStartProcess";
  case effStopProcess:
    return "effStopProcess";
  case effSetTotalSampleToProcess:
    return "effSetTotalSampleToProcess";
  case effSetPanLaw:
    return "effSetPanLaw";
  case effBeginLoadBank:
    return "effBeginLoadBank";
  case effBeginLoadProgram:
    return "effBeginLoadProgram";
  case effSetProcessPrecision:
    return "effSetProcessPrecision";
  case effGetNumMidiInputChannels:
    return "effGetNumMidiInputChannels";
  case effGetNumMidiOutputChannels:
    return "effGetNumMidiOutputChannels";
  default:
    return "unknown";
  }
}
#if defined __APPLE__
#include <AudioUnit/AudioUnitProperties.h>
#include <AudioUnit/AudioUnitCarbonView.h>
const char* AUSelectStr(int select)
{
  switch (select)
  {
  case kComponentOpenSelect:
    return "kComponentOpenSelect";
  case kComponentCloseSelect:
    return "kComponentCloseSelect";
  case kComponentVersionSelect:
    return "kComponentVersionSelect";
  case kAudioUnitInitializeSelect:
    return "kAudioUnitInitializeSelect";
  case kAudioUnitUninitializeSelect:
    return "kAudioUnitUninitializeSelect";
  case kAudioUnitGetPropertyInfoSelect:
    return "kAudioUnitGetPropertyInfoSelect";
  case kAudioUnitGetPropertySelect:
    return "kAudioUnitGetPropertySelect";
  case kAudioUnitSetPropertySelect:
    return "kAudioUnitSetPropertySelect";
  case kAudioUnitAddPropertyListenerSelect:
    return "kAudioUnitAddPropertyListenerSelect";
  case kAudioUnitRemovePropertyListenerSelect:
    return "kAudioUnitRemovePropertyListenerSelect";
  case kAudioUnitAddRenderNotifySelect:
    return "kAudioUnitAddRenderNotifySelect";
  case kAudioUnitRemoveRenderNotifySelect:
    return "kAudioUnitRemoveRenderNotifySelect";
  case kAudioUnitGetParameterSelect:
    return "kAudioUnitGetParameterSelect";
  case kAudioUnitSetParameterSelect:
    return "kAudioUnitSetParameterSelect";
  case kAudioUnitScheduleParametersSelect:
    return "kAudioUnitScheduleParametersSelect";
  case kAudioUnitRenderSelect:
    return "kAudioUnitRenderSelect";
  case kAudioUnitResetSelect:
    return "kAudioUnitResetSelect";
  case kComponentCanDoSelect:
    return "kComponentCanDoSelect";
  case kAudioUnitCarbonViewRange:
    return "kAudioUnitCarbonViewRange";
  case kAudioUnitCarbonViewCreateSelect:
    return "kAudioUnitCarbonViewCreateSelect";
  case kAudioUnitCarbonViewSetEventListenerSelect:
    return "kAudioUnitCarbonViewSetEventListenerSelect";
  default:
    return "unknown";
  }
}
const char* AUPropertyStr(int propID)
{
  switch (propID)
  {
  case kAudioUnitProperty_ClassInfo:
    return "kAudioUnitProperty_ClassInfo";
  case kAudioUnitProperty_MakeConnection:
    return "kAudioUnitProperty_MakeConnection";
  case kAudioUnitProperty_SampleRate:
    return "kAudioUnitProperty_SampleRate";
  case kAudioUnitProperty_ParameterList:
    return "kAudioUnitProperty_ParameterList";
  case kAudioUnitProperty_ParameterInfo:
    return "kAudioUnitProperty_ParameterInfo";
  case kAudioUnitProperty_FastDispatch:
    return "kAudioUnitProperty_FastDispatch";
  case kAudioUnitProperty_CPULoad:
    return "kAudioUnitProperty_CPULoad";
  case kAudioUnitProperty_StreamFormat:
    return "kAudioUnitProperty_StreamFormat";
  case kAudioUnitProperty_ElementCount:
    return "kAudioUnitProperty_ElementCount";
  case kAudioUnitProperty_Latency:
    return "kAudioUnitProperty_Latency";
  case kAudioUnitProperty_SupportedNumChannels:
    return "kAudioUnitProperty_SupportedNumChannels";
  case kAudioUnitProperty_MaximumFramesPerSlice:
    return "kAudioUnitProperty_MaximumFramesPerSlice";
  case kAudioUnitProperty_SetExternalBuffer:
    return "kAudioUnitProperty_SetExternalBuffer";
  case kAudioUnitProperty_ParameterValueStrings:
    return "kAudioUnitProperty_ParameterValueStrings";
  case kAudioUnitProperty_GetUIComponentList:
    return "kAudioUnitProperty_GetUIComponentList";
  case kAudioUnitProperty_AudioChannelLayout:
    return "kAudioUnitProperty_AudioChannelLayout";
  case kAudioUnitProperty_TailTime:
    return "kAudioUnitProperty_TailTime";
  case kAudioUnitProperty_BypassEffect:
    return "kAudioUnitProperty_BypassEffect";
  case kAudioUnitProperty_LastRenderError:
    return "kAudioUnitProperty_LastRenderError";
  case kAudioUnitProperty_SetRenderCallback:
    return "kAudioUnitProperty_SetRenderCallback";
  case kAudioUnitProperty_FactoryPresets:
    return "kAudioUnitProperty_FactoryPresets";
  case kAudioUnitProperty_ContextName:
    return "kAudioUnitProperty_ContextName";
  case kAudioUnitProperty_RenderQuality:
    return "kAudioUnitProperty_RenderQuality";
  case kAudioUnitProperty_HostCallbacks:
    return "kAudioUnitProperty_HostCallbacks";
  case kAudioUnitProperty_CurrentPreset:
    return "kAudioUnitProperty_CurrentPreset";
  case kAudioUnitProperty_InPlaceProcessing:
    return "kAudioUnitProperty_InPlaceProcessing";
  case kAudioUnitProperty_ElementName:
    return "kAudioUnitProperty_ElementName";
  case kAudioUnitProperty_CocoaUI:
    return "kAudioUnitProperty_CocoaUI";
  case kAudioUnitProperty_SupportedChannelLayoutTags:
    return "kAudioUnitProperty_SupportedChannelLayoutTags";
  case kAudioUnitProperty_ParameterIDName:
    return "kAudioUnitProperty_ParameterIDName";
  case kAudioUnitProperty_ParameterClumpName:
    return "kAudioUnitProperty_ParameterClumpName";
  case kAudioUnitProperty_PresentPreset:
    return "kAudioUnitProperty_PresentPreset";
  case kAudioUnitProperty_OfflineRender:
    return "kAudioUnitProperty_OfflineRender";
  case kAudioUnitProperty_ParameterStringFromValue:
    return "kAudioUnitProperty_ParameterStringFromValue";
  case kAudioUnitProperty_ParameterValueFromString:
    return "kAudioUnitProperty_ParameterValueFromString";
  case kAudioUnitProperty_IconLocation:
    return "kAudioUnitProperty_IconLocation";
  case kAudioUnitProperty_PresentationLatency:
    return "kAudioUnitProperty_PresentationLatency";
  case kAudioUnitProperty_DependentParameters:
    return "kAudioUnitProperty_DependentParameters";
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  case kAudioUnitProperty_AUHostIdentifier:
    return "kAudioUnitProperty_AUHostIdentifier";
  case kAudioUnitProperty_MIDIOutputCallbackInfo:
    return "kAudioUnitProperty_MIDIOutputCallbackInfo";
  case kAudioUnitProperty_MIDIOutputCallback:
    return "kAudioUnitProperty_MIDIOutputCallback";
  case kAudioUnitProperty_InputSamplesInOutput:
    return "kAudioUnitProperty_InputSamplesInOutput";
  case kAudioUnitProperty_ClassInfoFromDocument:
    return "kAudioUnitProperty_ClassInfoFromDocument";
#endif
  default:
    return "unknown";
  }
}
const char* AUScopeStr(int scope)
{
  switch (scope)
  {
  case kAudioUnitScope_Global:
    return "kAudioUnitScope_Global";
  case kAudioUnitScope_Input:
    return "kAudioUnitScope_Input";
  case kAudioUnitScope_Output:
    return "kAudioUnitScope_Output";
  case kAudioUnitScope_Group:
    return "kAudioUnitScope_Group";
  case kAudioUnitScope_Part:
    return "kAudioUnitScope_Part";
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
  case kAudioUnitScope_Note:
    return "kAudioUnitScope_Note";
#endif
  default:
    return "unknown";
  }
}
#endif // __APPLE__
#else
void Trace(const char* funcName, int line, const char* format, ...) {}
const char* VSTOpcodeStr(int opCode)
{
  return "";
}
const char* AUSelectStr(int select)
{
  return "";
}
const char* AUPropertyStr(int propID)
{
  return "";
}
const char* AUScopeStr(int scope)
{
  return "";
}
#endif // TRACER_BUILD