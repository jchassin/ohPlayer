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

TBool OhLog::IsDebugEnabled()
{
    static TBool checked = false;
    static TBool enabled = false;
    if (!checked) {
        checked = true;
        const char* env = getenv("OHP_DEBUG");
        enabled = (env != nullptr && env[0] != '\0' && strcmp(env, "0") != 0);
    }
    return enabled;
}

TInt OhLog::Print(const TChar* aFormat, ...)
{
    va_list arg;
    va_start(arg, aFormat);
    TInt ret = Log::PrintVA(aFormat, arg);
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
    TInt ret = Log::PrintVA(aFormat, arg);
    Log::Print("%s", AC_NORMAL);
    va_end(arg);
    return ret;
}

TInt OhLog::PrintError(const TChar* aFormat, ...)
{
    va_list arg;
    va_start(arg, aFormat);

    Log::Print("%s", AC_RED);
    TInt ret = Log::PrintVA(aFormat, arg);
    Log::Print("%s", AC_NORMAL);

    va_end(arg);
    return ret;
}

TInt OhLog::PrintWarning(const TChar* aFormat, ...)
{
    va_list arg;
    va_start(arg, aFormat);

    Log::Print("%s", AC_YELLOW);
    TInt ret = Log::PrintVA(aFormat, arg);
    Log::Print("%s", AC_NORMAL);

    va_end(arg);
    return ret;
}
