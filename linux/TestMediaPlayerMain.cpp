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
    // Note: prefix udn with 4c494e4e- to get older versions of Linn Konfig to recognise our devices
    TestMediaPlayerInit::AppendUniqueId(dvStack->Env(), iOptions.Udn().Value(), Brn("TestMediaPlayer"), udn);
    Log::Print("UDN is %.*s\n", PBUF(udn));

    Log::Print("%s:%d - %s\n", __FILE__, __LINE__, iOptions.Room().CString());
    Log::Print("%s:%d - %s\n", __FILE__, __LINE__, iOptions.Name().CString());
    Log::Print("%s:%d - %s\n", __FILE__, __LINE__, iOptions.TuneIn().CString());
    Log::Print("%s:%d - %s\n", __FILE__, __LINE__, iOptions.Tidal().CString());
    Log::Print("%s:%d - %s\n", __FILE__, __LINE__, iOptions.Qobuz().CString());
    Log::Print("%s:%d - %s\n", __FILE__, __LINE__, iOptions.UserAgent().CString());
    Log::Print("%s:%d - %s\n", __FILE__, __LINE__, iOptions.StoreFile().CString());
    Log::Print("%s:%d - %s\n", __FILE__, __LINE__, iOptions.OptionOdp().Value());
    Log::Print("%s:%d - %s\n", __FILE__, __LINE__, iOptions.OptionWebUi().Value());


    // Create TestMediaPlayer.
    tmp = new TestMediaPlayer(*dvStack, *cpStack, udn, iOptions.Room().CString(), iOptions.Name().CString(),
        iOptions.TuneIn().Value(), iOptions.Tidal().Value(), iOptions.Qobuz().Value(),
        iOptions.UserAgent().Value(), iOptions.StoreFile().CString(), iOptions.OptionOdp().Value(), iOptions.OptionWebUi().Value());
    Log::Print("%s:%d\n", __FILE__, __LINE__);
#if 0
    // Add the audio driver to the pipeline.
    //
    // The 22052ms value a is a bit of a magic number which get's
    // things going for the Hifiberry Digi+ card.
    //
    // FIXME This should be calculated.
    driver = new DriverAlsa(tmp->Pipeline(), 22052);
    if (driver == NULL)
    {
        Log::Print("%s:%d - Unable to start DriverAlsa\n", __FILE__, __LINE__);
    }
    else
    {
        Log::Print("%s:%d - DriverAlsa started !\n", __FILE__, __LINE__);
    }
    Log::Print("%s:%d\n", __FILE__, __LINE__);
    Media::AnimatorBasic* animator = new Media::AnimatorBasic(
        dvStack->Env(),
        tmp->Pipeline(),
        iOptions.ClockPull().Value(),
        tmp->DsdMaxSampleRate(),
        tmp->DsdSampleBlockWords(),
        tmp->DsdPadBytesPerChunk());
    Log::Print("%s:%d\n", __FILE__, __LINE__);
    tmp->SetPullableClock(*animator);
 #endif
    Log::Print("%s:%d\n", __FILE__, __LINE__);
    tmp->Run();
    Log::Print("%s:%d\n", __FILE__, __LINE__);
    tmp->StopPipeline();
    Log::Print("%s:%d\n", __FILE__, __LINE__);
    //delete animator;
    delete tmp;
}
#include<stdio.h>
#include<signal.h>
#include<unistd.h>
void CDECL sig_handler(int signo);
const gchar        *g_appName   = "OpenHomePlayer";
int CDECL main(int aArgc, char* aArgv[])
{
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
    printf("received SIGINT\n");
  }
}

