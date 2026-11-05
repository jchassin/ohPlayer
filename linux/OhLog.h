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
        // kApplication0: unused by the linked ohNet/ohPipeline libraries
        // (ohPipeline aliases start at kApplication1: kMedia=1, kCodec=2, ...),
        // so ohPlayer logs don't collide with a library subsystem - e.g.
        // kApplication2 is kCodec, which logs one line per ADTS frame.
        // The level can be toggled at runtime from the debug shell
        // ('debug set Application0' / 'debug clear Application0', plus
        // 'debug severity Debug' to raise the severity threshold).
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
