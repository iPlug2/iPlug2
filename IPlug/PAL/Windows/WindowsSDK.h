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

// Restore default packing before including windows.h incase custom alignment
// has been set with /Zp(x) compiler switch. Defaults 64bit=16, 32bit=8
#if (PLATFORM_64BIT)
	#pragma pack(push, 16)
#else
	#pragma pack(push, 8)
#endif

#include <SDKDDKVer.h>

// clang-format off

// The following flags inhibit definition of the indicated items.
	#define NOGDICAPMASKS     // CC_*, LC_*, PC_*, CP_*, TC_*, RC_
//	#define NOVIRTUALKEYCODES // VK_*
//	#define NOWINMESSAGES     // WM_*, EM_*, LB_*, CB_*
//	#define NOWINSTYLES       // WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
//	#define NOSYSMETRICS      // SM_*
//	#define NOMENUS           // MF_*
//	#define NOICONS           // IDI_*
//	#define NOKEYSTATES       // MK_*
//	#define NOSYSCOMMANDS     // SC_*
//	#define NORASTEROPS       // Binary and Tertiary raster ops
//	#define NOSHOWWINDOW      // SW_*
	#define OEMRESOURCE       // OEM Resource values
	#define NOATOM            // Atom Manager routines
//	#define NOCLIPBOARD       // Clipboard routines
//	#define NOCOLOR           // Screen colors
//	#define NOCTLMGR          // Control and Dialog routines
	#define NODRAWTEXT        // DrawText() and DT_*
//	#define NOGDI             // All GDI defines and routines
	#define NOKERNEL          // All KERNEL defines and routines
//	#define NOUSER            // All USER defines and routines
//	#define NONLS             // All NLS defines and routines
//	#define NOMB              // MB_* and MessageBox()
	#define NOMEMMGR          // GMEM_*, LMEM_*, GHND, LHND, associated routines
//	#define NOMETAFILE        // typedef METAFILEPICT
	#define NOMINMAX          // Macros min(a,b) and max(a,b)
//	#define NOMSG             // typedef MSG and associated routines
	#define NOOPENFILE        // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
	#define NOSCROLL          // SB_* and scrolling routines
	#define NOSERVICE         // All Service Controller routines, SERVICE_ equates, etc.
	#define NOSOUND           // Sound driver routines
//	#define NOTEXTMETRIC      // typedef TEXTMETRIC and associated routines
//	#define NOWH              // SetWindowsHook and WH_*
//	#define NOWINOFFSETS      // GWL_*, GCL_*, associated routines
	#define NOCOMM            // COMM driver routines
	#define NOKANJI           // Kanji support stuff.
	#define NOHELP            // Help engine interface.
	#define NOPROFILER        // Profiler interface.
	#define NODEFERWINDOWPOS  // DeferWindowPos routines
	#define NOMCX             // Modem Configuration Extensions

// clang-format on

#define WIN32_LEAN_AND_MEAN  // Exclude alot of stuff
#define STRICT               // Should be default when using WIN32_LEAN_AND_MEAN. but just to be sure

#include <windows.h>

// Make sure NTSTATUS is defined since we're using WIN32_LEAN_AND_MEAN
using NTSTATUS  = _Return_type_success_(return >= 0) long;
using PNTSTATUS = NTSTATUS*;

// Additional windows headers
#include <VersionHelpers.h>

#include <intrin.h>   // Intrinsic functions
#include <intsafe.h>  // Helper functions to prevent integer overflow bugs
#include <strsafe.h>  // Safer C library string routine replacements

//#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <Shlwapi.h>
#include <WindowsX.h>
#include <wininet.h>
#include <winsock.h>
#include <d3dkmthk.h>
#include <sys/stat.h>

#pragma pack(pop)
