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

static TInt OhLog::PrintError(const TChar* aFormat, ...)
{
    TInt ret;
    va_list arg;
    va_start(arg,aFormat);

   Log::Print("%s", AC_RED);
   ret = Log::Print(aFormat, arg);
   Log::Print("%s", AC_NORMAL);
   va_end(arg);
   return (ret);
}
static TInt OhLog::PrintWarning(const TChar* aFormat, ...)
{
    TInt ret;
    va_list arg;
    va_start(arg,aFormat);

   Log::Print("%s", AC_YELLOW);
   ret = Log::Print(aFormat, arg);
   Log::Print("%s", AC_NORMAL);
   va_end(arg);
   return (ret);
}
