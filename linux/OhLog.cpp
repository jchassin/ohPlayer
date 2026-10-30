#include "OhLog.h"



#define AC_BLACK "\x1b[30m"
#define AC_RED "\x1b[31m"
#define AC_GREEN "\x1b[32m"
#define AC_YELLOW "\x1b[33m"
#define AC_BLUE "\x1b[34m"
#define AC_MAGENTA "\x1b[35m"
#define AC_CYAN "\x1b[36m"
#define AC_WHITE "\x1b[37m"
#define AC_NORMAL "\x1b[m"


using namespace OpenHome;

const TUint64 OhLog::kLevel = Debug::kApplication2;

TBool OhLog::IsDebugEnabled()
{
    return Debug::TestLevel(kLevel) && Debug::TestSeverity(Debug::kSeverityDebug);
}

void OhLog::SetDebugEnabled(TBool aEnabled)
{
    if (aEnabled) {
        Debug::AddLevel(kLevel);
        Debug::SetSeverity(Debug::kSeverityDebug);
    }
    else {
        Debug::RemoveLevel(kLevel);
        if (Debug::Severity() < Debug::kSeverityInfo) {
            Debug::SetSeverity(Debug::kSeverityInfo);
        }
    }
}

static TInt PrintWithContextVA(TUint64 aLevel, TUint aSeverity, const TChar* aFormat, va_list aArgs)
{
    TChar temp[Log::kMaxPrintBytes + 1];
    const TInt n = vsnprintf(temp, sizeof(temp), aFormat, aArgs);
    if (n < 0) {
        return 0;
    }
    Brn message(reinterpret_cast<const TByte*>(temp),
                (n >= (TInt)sizeof(temp)) ? sizeof(temp) - 1 : n);
    return Log::PrintWithContext(aLevel, aSeverity, message);
}

TInt OhLog::Print(const TChar* aFormat, ...)
{
    if (!Debug::TestLevel(kLevel)) {
        return 0;
    }
    va_list arg;
    va_start(arg, aFormat);
    TInt ret = PrintWithContextVA(kLevel, Debug::kSeverityInfo, aFormat, arg);
    va_end(arg);
    return ret;
}

TInt OhLog::PrintDebug(const TChar* aFormat, ...)
{
    if (!IsDebugEnabled()) {
        return 0;
    }
    va_list arg;
    va_start(arg, aFormat);
    Log::Print("%s", AC_CYAN);
    TInt ret = PrintWithContextVA(kLevel, Debug::kSeverityDebug, aFormat, arg);
    Log::Print("%s", AC_NORMAL);
    va_end(arg);
    return ret;
}

TInt OhLog::PrintError(const TChar* aFormat, ...)
{
    if (!Debug::TestLevel(kLevel)) {
        return 0;
    }
    va_list arg;
    va_start(arg, aFormat);
    Log::Print("%s", AC_RED);
    TInt ret = PrintWithContextVA(kLevel, Debug::kSeverityError, aFormat, arg);
    Log::Print("%s", AC_NORMAL);
    va_end(arg);
    return ret;
}

TInt OhLog::PrintWarning(const TChar* aFormat, ...)
{
    if (!Debug::TestLevel(kLevel)) {
        return 0;
    }
    va_list arg;
    va_start(arg, aFormat);
    Log::Print("%s", AC_YELLOW);
    TInt ret = PrintWithContextVA(kLevel, Debug::kSeverityWarning, aFormat, arg);
    Log::Print("%s", AC_NORMAL);
    va_end(arg);
    return ret;
}
