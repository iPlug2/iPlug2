/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for more info.
 
 ==============================================================================
*/

#pragma once


//---------------------------------------------------------
// Configure and include windows.h

#ifdef _WINDOWS_
	#error \
		"<windows.h> already included without proper configuration. Make sure to include Platform.h before any other file that includes windows.h"
#endif

// Undefine all
#undef NOGDICAPMASKS
#undef NOVIRTUALKEYCODES
#undef NOWINMESSAGES
#undef NOWINSTYLES
#undef NOSYSMETRICS
#undef NOMENUS
#undef NOICONS
#undef NOKEYSTATES
#undef NOSYSCOMMANDS
#undef NORASTEROPS
#undef NOSHOWWINDOW
#undef OEMRESOURCE
#undef NOATOM
#undef NOCLIPBOARD
#undef NOCOLOR
#undef NOCTLMGR
#undef NODRAWTEXT
#undef NOGDI
#undef NOKERNEL
#undef NOUSER
#undef NONLS
#undef NOMB
#undef NOMEMMGR
#undef NOMETAFILE
#undef NOMINMAX
#undef NOMSG
#undef NOOPENFILE
#undef NOSCROLL
#undef NOSERVICE
#undef NOSOUND
#undef NOTEXTMETRIC
#undef NOWH
#undef NOWINOFFSETS
#undef NOCOMM
#undef NOKANJI
#undef NOHELP
#undef NOPROFILER
#undef NODEFERWINDOWPOS
#undef NOMCX

// The following flags inhibit definition of the indicated items.
#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
#define OEMRESOURCE       // OEM Resource values
#define NOATOM            // Atom Manager routines
#define NODRAWTEXT        // DrawText() and DT_*
#define NOKERNEL          // All KERNEL defines and routines
#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE        // typedef METAFILEPICT
#define NOMINMAX          // Macros min(a,b) and max(a,b)
#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL          // SB_* and scrolling routines
#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
#define NOSOUND           // Sound driver routines
#define NOCOMM            // COMM driver routines
#define NOKANJI           // Kanji support stuff.
#define NOHELP            // Help engine interface.
#define NOPROFILER        // Profiler interface.
#define NODEFERWINDOWPOS  // DeferWindowPos routines
#define NOMCX             // Modem Configuration Extensions

#define WIN32_LEAN_AND_MEAN  // Exclude alot of stuff
#define STRICT               // Should be default when using WIN32_LEAN_AND_MEAN. but just to be sure

// Are we supposed to set WINAPI_FAMILY flags with flags defined in winapifamily.h
// which sets the WINAPI_FAMILY to a default value if flags are not defined because we needed to
// include winapifamily.h to get the flags defined to set WINAPI_FAMILY ... which gets set to default...
#include <winapifamily.h>
#undef WINAPI_FAMILY
#undef _INC_WINAPIFAMILY

// WINAPI_FAMILY_PC_APP        // Windows Store Applications
// WINAPI_FAMILY_PHONE_APP     // Windows Phone Applications
// WINAPI_FAMILY_SYSTEM        // Windows Drivers and Tools
// WINAPI_FAMILY_SERVER        // Windows Server Applications
// WINAPI_FAMILY_GAMES         // Windows Games and Applications
// WINAPI_FAMILY_DESKTOP_APP   // Windows Desktop Applications
#define WINAPI_FAMILY (WINAPI_FAMILY_DESKTOP_APP)

// Restore default packing before including windows.h incase custom alignment
// has been set with /Zp(x) compiler switch. Defaults 64bit=16, 32bit=8
#if (PLATFORM_64BIT)
	#pragma pack(push, 16)
#else
	#pragma pack(push, 8)
#endif

#include <windows.h>

// Additional windows headers
#include <intrin.h>   // Intrinsic functions
#include <stdint.h>   // C Standard Library
#include <intsafe.h>  // Helper functions to prevent integer overflow bugs
#include <strsafe.h>  // Safer C library string routine replacements

#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <Shlwapi.h>
#include <VersionHelpers.h>
#include <WindowsX.h>
#include <wininet.h>

#pragma pack(pop)
