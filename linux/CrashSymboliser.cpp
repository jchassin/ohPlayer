#include "CrashSymboliser.h"
#include <OpenHome/Private/Printer.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace OpenHome;

namespace OpenHome {
namespace Av {

// Extract "(+0xHEX)" offsets from the crash text, then run one addr2line
// invocation over all of them against /proc/self/exe (the running binary,
// which embeds the debug info when built with -g).
CrashSymboliser& CrashSymboliser::Instance()
{
    static CrashSymboliser s;
    return s;
}

void CrashSymboliser::FatalErrorHandler(const char* aMsg)
{
    if (aMsg != nullptr) {
        Log::Print("%s", aMsg);
    }
    Symbolise(aMsg);
}

void CrashSymboliser::Symbolise(const char* aCrashText)
{
    if (aCrashText == nullptr) {
        return;
    }

    // Collect offsets (up to 32 frames).
    const TUint kMaxFrames = 32;
    unsigned long offsets[kMaxFrames];
    TUint count = 0;
    const char* p = aCrashText;
    while (count < kMaxFrames) {
        p = strstr(p, "(+0x");
        if (p == nullptr) {
            break;
        }
        char* end = nullptr;
        const unsigned long val = strtoul(p + 4, &end, 16);
        if (end == p + 4 || *end != ')') {
            p += 4;
            continue;
        }
        offsets[count++] = val;
        p = end;
    }
    if (count == 0) {
        return;
    }

    // Build the addr2line command: addr2line -e /proc/self/exe -f -C 0xA 0xB ...
    // (command length bounded: 32 frames x ~14 chars + fixed parts)
    char cmd[1024];
    int written = snprintf(cmd, sizeof(cmd), "addr2line -e /proc/self/exe -f -C");
    if (written <= 0 || written >= (int)sizeof(cmd) - 16) {
        return;
    }
    for (TUint i = 0; i < count; i++) {
        const int n = snprintf(cmd + written, sizeof(cmd) - written, " 0x%lx", offsets[i]);
        if (n <= 0 || written + n >= (int)sizeof(cmd) - 1) {
            break;
        }
        written += n;
    }

    Log::Print("\n==== symbolicated stack (addr2line) ====\n");
    FILE* pipe = popen(cmd, "r");
    if (pipe == nullptr) {
        Log::Print("(crash symboliser: popen failed)\n");
        return;
    }
    // addr2line prints, per address: function name line, then file:line.
    // Print everything verbatim; the interleaving matches the offsets.
    char line[512];
    while (fgets(line, sizeof(line), pipe) != nullptr) {
        const size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        Log::Print("%s\n", line);
    }
    const int rc = pclose(pipe);
    if (rc != 0) {
        Log::Print("(crash symboliser: addr2line exited with %d - is it installed?)\n", rc);
    }
    Log::Print("==== end symbolicated stack ====\n");
}

} // namespace Av
} // namespace OpenHome
