#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>

namespace OpenHome {
namespace Av {

// Crash symbolisation helper.
//
// ohNet's unhandled-exception path prints the raw stack (module+0xoffset
// addresses); analysing a crash requires copying each address into
// addr2line by hand. This helper registers a fatal error handler that
// pipes the offsets through addr2line (via popen) and prints the
// symbolised stack directly in the log, in thread and function form:
//
//   ==== symbolicated stack ====
//   0x203854: OpenHome::CallAssertHandler(...) at Exception.cpp:51
//   ...
//
// Register from TestMediaPlayerInit::CreateLibrary (after the binary is
// running; addr2line is executed with the /proc/self/exe image).
// Can be compiled out by not calling RegisterCrashSymboliser().
class CrashSymboliser
{
public:
    // Parses the crash text for "(+0xNNN) [0x...]" offsets and prints the
    // symbolised frames. Safe to call from a signal/fatal handler (uses
    // only async-signal-safe libc calls: popen/system are not, so this
    // is intended for the ohNet fatal error path, which runs on a normal
    // thread).
    // ohNet fatal error handler: prints the crash message (exception type,
    // file:line, thread, raw stack) then the symbolicated stack.
    static void FatalErrorHandler(const char* aMsg);
    static void Symbolise(const char* aCrashText);
};

} // namespace Av
} // namespace OpenHome
