#ifndef __OHP_PRINTER
#define __OHP_PRINTER
#include <OpenHome/Types.h>
#include <OpenHome/Private/Printer.h>
#include <OpenHome/Private/Debug.h>
#include <stdio.h>
#include <stdarg.h>
#include <cstring>
#include <cstdlib>
#include <OpenHome/Private/Standard.h>

namespace OpenHome {
    class OhLog : public Log
    {
      public:
        // Application log level (one of the ohNet kApplication* subsystem bits).
        static const TUint64 kLevel;

        static TInt Print(const TChar* aFormat, ...);
        static TInt PrintDebug(const TChar* aFormat, ...);
        static TInt PrintError(const TChar* aFormat, ...);
        static TInt PrintWarning(const TChar* aFormat, ...);
        static TBool IsDebugEnabled();
        static void SetDebugEnabled(TBool aEnabled);
    };
}

#endif //
