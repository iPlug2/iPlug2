#ifndef _LOG_
#define _LOG_

#undef min
#undef max

#include <stdarg.h>
#ifndef _MSC_VER
  #include <stdint.h>
#endif

#include "Containers.h"
#include "IPlugOSDetect.h"

#if defined OS_WIN
  #include <stdio.h>
  #include <ctype.h>
  void DBGMSG(const char *format, ...);
  #define SYS_THREAD_ID (intptr_t) GetCurrentThreadId()

#elif defined __APPLE__ // TODO: check on ios
  #define SYS_THREAD_ID (intptr_t) pthread_self()
  #define DBGMSG(...) printf(__VA_ARGS__)
#else
  #error "No OS defined!"
#endif

#if defined TRACER_BUILD
  #define TRACE Trace(TRACELOC, "");
//  #define TRACE_PROCESS Trace(TRACELOC, "");
  #define TRACE_PROCESS
#else
  #define TRACE
  #define TRACE_PROCESS
#endif

#define TRACELOC __FUNCTION__,__LINE__
void Trace(const char* funcName, int line, const char* fmtStr, ...);

// To trace some arbitrary data:                 Trace(TRACELOC, "%s:%d", myStr, myInt);
// To simply create a trace entry in the log:    TRACE;
// No need to wrap tracer calls in #ifdef TRACER_BUILD because Trace is a no-op unless TRACER_BUILD is defined.

const char* VSTOpcodeStr(int opCode);
const char* AUSelectStr(int select);
const char* AUPropertyStr(int propID);
const char* AUScopeStr(int scope);

struct Timer
{
  int mT;
  Timer();

  // Returns true every sec seconds.
  bool Every(double sec);
};

void ToLower(char* cDest, const char* cSrc);
const char* CurrentTime();
void CompileTimestamp(const char* Mmm_dd_yyyy, const char* hh_mm_ss, WDL_String* pStr);
const char* AppendTimestamp(const char* Mmm_dd_yyyy, const char* hh_mm_ss, const char* cStr);
#define APPEND_TIMESTAMP(str) AppendTimestamp(__DATE__, __TIME__, str)

#endif

