//
//    ██████ ██   ██  ██████   ██████
//   ██      ██   ██ ██    ██ ██            ** Classy Header-Only Classes **
//   ██      ███████ ██    ██ ██
//   ██      ██   ██ ██    ██ ██           https://github.com/Tracktion/choc
//    ██████ ██   ██  ██████   ██████
//
//   CHOC is (C)2022 Tracktion Corporation, and is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef CHOC_PLATFORM_DETECT_DEBUGGER_HEADER_INCLUDED
#define CHOC_PLATFORM_DETECT_DEBUGGER_HEADER_INCLUDED

#include "choc_Platform.h"

namespace choc
{

/// Returns true if the current process is running in a debugger.
inline bool isDebuggerActive();

} // namespace choc



//==============================================================================
//        _        _           _  _
//     __| |  ___ | |_   __ _ (_)| | ___
//    / _` | / _ \| __| / _` || || |/ __|
//   | (_| ||  __/| |_ | (_| || || |\__ \ _  _  _
//    \__,_| \___| \__| \__,_||_||_||___/(_)(_)(_)
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

#if CHOC_WINDOWS

#include "choc_DynamicLibrary.h"
#include "choc_Assert.h"

inline bool choc::isDebuggerActive()
{
    choc::file::DynamicLibrary kernelLib ("kernel32.dll");
    using IsDebuggerPresentFn = int(__stdcall *)();
    auto fn = reinterpret_cast<IsDebuggerPresentFn> (kernelLib.findFunction ("IsDebuggerPresent"));
    return fn != nullptr && fn() != 0;
}

//==============================================================================
#elif CHOC_APPLE || CHOC_BSD

#include <sys/sysctl.h>
#include <unistd.h>

inline bool choc::isDebuggerActive()
{
    struct ::kinfo_proc result;
    size_t resultSize = sizeof (result);
    int params[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, ::getpid() };
    ::sysctl (params, 4, &result, &resultSize, nullptr, 0);
    return (result.kp_proc.p_flag & P_TRACED) != 0;
}

//==============================================================================
#elif CHOC_LINUX

#include "../text/choc_Files.h"
#include "../text/choc_StringUtilities.h"

inline bool choc::isDebuggerActive()
{
    auto readStatusFileItem = [] (std::string_view filename, std::string_view item) -> std::string
    {
        auto lines = choc::text::splitIntoLines (choc::file::loadFileAsString (std::string (filename)), false);

        for (auto i = lines.rbegin(); i != lines.rend(); ++i)
        {
            auto line = choc::text::trimStart (std::string_view (*i));

            if (choc::text::startsWith (line, item))
            {
                auto remainder = choc::text::trimStart (item.substr (item.length()));

                if (! remainder.empty() && remainder[0] == ':')
                    return std::string (choc::text::trim (remainder.substr (1)));
            }
        }

        return {};
    };

    try
    {
        return std::stoi (readStatusFileItem ("/proc/self/status", "TracerPid")) > 0;
    }
    // an exception will mean that the file wasn't there, or the entry was missing
    // or malformed, so just shrug and assume it's not in the debugger
    catch (const std::exception&) {}

    return false;
}

#else
 #error "Unknown or unsupported OS!"
#endif

#endif  // CHOC_PLATFORM_DETECT_DEBUGGER_HEADER_INCLUDED
