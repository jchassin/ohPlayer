#ifndef __OHP_PRINTER
#define __OHP_PRINTER
#include <OpenHome/Types.h>
#include <OpenHome/Private/Printer.h>
#include <stdio.h>
#include <stdarg.h>
#include <cstring>
#include <cstdlib>
#include <OpenHome/Private/Standard.h>

namespace OpenHome {
    class OhLog : public Log
    {
      public:
        static TInt Print(const TChar* aFormat, ...);
        static TInt PrintDebug(const TChar* aFormat, ...);
        static TInt PrintError(const TChar* aFormat, ...);
        static TInt PrintWarning(const TChar* aFormat, ...);
        static TBool IsDebugEnabled();
    };
}

#endif //
