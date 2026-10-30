#include "OhLog.h"
#include "OpenHomePlayer.h"
#include "TestMediaPlayer.h"
#include <OpenHome/Types.h>
#include <OpenHome/Media/Tests/Cdecl.h>
#include <OpenHome/Media/Tests/GetCh.h>
#include <OpenHome/Net/Private/DviStack.h>
#include <OpenHome/Media/Utils/AnimatorBasic.h>
#include <OpenHome/Media/PipelineManager.h>
#include <OpenHome/Private/Thread.h>
#include <OpenHome/Functor.h>
//#include "DriverAlsa.h"
#include <stdlib.h>

using namespace OpenHome;
using namespace OpenHome::Av;
using namespace OpenHome::Av::Test;
using namespace OpenHome::Net;
//using namespace OpenHome::Media;

class TestMediaPlayerThread
{
public:
    TestMediaPlayerThread(const TestMediaPlayerOptions& aOptions);
    ~TestMediaPlayerThread();
    void Run();
private:
    void RunInThread();
private:
    const TestMediaPlayerOptions& iOptions;
    Library* iLib;
    Media::PriorityArbitratorAnimator iArbAnimator;
    Media::PriorityArbitratorPipeline iArbPipeline;
};

TestMediaPlayerThread::TestMediaPlayerThread(const TestMediaPlayerOptions& aOptions)
    : iOptions(aOptions)
    , iArbAnimator(kPrioritySystemHighest)
    , iArbPipeline(kPrioritySystemHighest-1)
{
    iLib = TestMediaPlayerInit::CreateLibrary(iOptions.Room().CString(), iOptions.Loopback().Value(), iOptions.Adapter().Value(), iOptions.Shell().Value());
    ThreadPriorityArbitrator& priorityArbitrator = iLib->Env().PriorityArbitrator();
    priorityArbitrator.Add(iArbAnimator);
    priorityArbitrator.Add(iArbPipeline);
}

TestMediaPlayerThread::~TestMediaPlayerThread()
{
    delete iLib;
}

void TestMediaPlayerThread::Run()
{
    ThreadFunctor* th = new ThreadFunctor("TestMediaPlayer", MakeFunctor(*this, &TestMediaPlayerThread::RunInThread));
    th->Start();
    delete th;
}
TestMediaPlayer* tmp;

void TestMediaPlayerThread::RunInThread()
{
    const TChar* cookie ="TestMediaPlayerMain";
    NetworkAdapter* adapter = iLib->CurrentSubnetAdapter(cookie);
    CpStack* cpStack = nullptr;
    DvStack* dvStack = nullptr;
//    OpenHome::Media::DriverAlsa     *driver  = NULL;
    iLib->StartCombined(adapter->Subnet(), cpStack, dvStack);


    // Seed random number generator.
    TestMediaPlayerInit::SeedRandomNumberGenerator(dvStack->Env(), iOptions.Room().Value(), adapter->Address(), dvStack->ServerUpnp());
    adapter->RemoveRef(cookie);

    // Set/construct UDN.
    Bwh udn;
    if (iOptions.Udn().Value().Bytes() > 0) {
        // Explicit -u/--udn: use as provided.
        udn.Replace(iOptions.Udn().Value());
    }
    else {
        // Derive a canonical UPnP uuid (8-4-4-4-12) from the room name so
        // the device identity is stable across reboots and IP changes, and
        // well-formed for control points that reject non-uuid UDNs.
        TestMediaPlayerInit::CreateCanonicalUuid(iOptions.Room().Value(), udn);
    }
    OhLog::Print("UDN is %.*s\n", PBUF(udn));

    OhLog::Print("Room: %s, Name: %s\n", iOptions.Room().CString(), iOptions.Name().CString());
    OhLog::Print("TuneIn: %s, Tidal: %s, Qobuz: %s\n",
        iOptions.TuneIn().CString(), iOptions.Tidal().CString(), iOptions.Qobuz().CString());
    OhLog::Print("UserAgent: %s, StoreFile: %s\n",
        iOptions.UserAgent().CString(), iOptions.StoreFile().CString());
    OhLog::Print("ODP port: %u, Web UI port: %u\n",
        iOptions.OptionOdp().Value(), iOptions.OptionWebUi().Value());


    // Create TestMediaPlayer.
    tmp = new TestMediaPlayer(*dvStack, *cpStack, udn, iOptions.Room().CString(), iOptions.Name().CString(),
        iOptions.TuneIn().Value(), iOptions.Tidal().Value(), iOptions.Qobuz().Value(),
        iOptions.UserAgent().Value(), iOptions.StoreFile().CString(), iOptions.OptionOdp().Value(), iOptions.OptionWebUi().Value(), iOptions.RadioBrowser().Value());
    tmp->Run();
    OhLog::Print("Shutting down pipeline\n");
    tmp->StopPipeline();
    delete tmp;
}
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
void CDECL sig_handler(int signo);
const gchar        *g_appName   = "OpenHomePlayer";
int CDECL main(int aArgc, char* aArgv[])
{
    // Enable the ohPlayer application log level; OHP_DEBUG additionally
    // enables debug-severity messages.
    OhLog::SetDebugEnabled(false);
    Debug::AddLevel(OhLog::kLevel);
    {
        const char* dbg = getenv("OHP_DEBUG");
        if (dbg != nullptr && dbg[0] != '\0' && strcmp(dbg, "0") != 0) {
            OhLog::SetDebugEnabled(true);
        }
    }
#ifdef _WIN32
    char* noErrDlgs = getenv("NO_ERROR_DIALOGS");
    if (noErrDlgs != nullptr && strcmp(noErrDlgs, "1") == 0) {
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    }
#endif // _WIN32
    //if (signal(SIGINT, sig_handler) == SIG_ERR) {}
    // Parse options.
    TestMediaPlayerOptions options;
    if (!options.Parse(aArgc, aArgv)) {
        return 1;
    }

    TestMediaPlayerThread* th = new TestMediaPlayerThread(options);
    th->Run();
    delete th;

    return 0;
}

void CDECL sig_handler(int signo)
{
  if (signo == SIGINT)
  {
    tmp->RequestExit();
    OhLog::Print("Received SIGINT\n");
  }
}

