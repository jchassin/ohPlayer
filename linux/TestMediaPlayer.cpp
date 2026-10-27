#include "OhLog.h"
#include "TestMediaPlayer.h"
#include "RaspdacMiniIF.h"
#include "OhLinnAmpVolume.h"
#include "LinnAmpConfig.h"
#include "PipelineConfig.h"
#include "RadioPresetsStatic.h"
#include "StoreWithOverriddenValue.h"
#include "ConfigAppPlayerLinn.h"
#include <OpenHome/Types.h>
#include <OpenHome/Net/Private/DviStack.h>
#include <cstdlib>
#include <unistd.h>
#include <OpenHome/Av/MediaPlayer.h>
#include <OpenHome/Av/Product.h>
#include <OpenHome/Net/Core/DvDevice.h>
#include <OpenHome/Media/PipelineManager.h>
#include <OpenHome/Media/Pipeline/StarterTimed.h>
#include <OpenHome/Media/Utils/AllocatorInfoLogger.h>
#include <OpenHome/Private/Printer.h>
#include <OpenHome/Private/TestFramework.h>
#include <OpenHome/Media/Codec/CodecFactory.h>
#include <OpenHome/Media/Codec/ContainerFactory.h>
#include <OpenHome/Media/Protocol/ProtocolFactory.h>
#include <OpenHome/Av/SourceFactory.h>
#include <OpenHome/Av/Radio/RadioPins.h>
#include <OpenHome/Av/Pins/UrlPins.h>
#include <OpenHome/Av/KvpStore.h>
//#include "RamStore.h"
#include <OpenHome/Av/UpnpAv/UpnpAv.h>
#include <OpenHome/Av/Playlist/Playlist.h>
#include <OpenHome/Configuration/ConfigManager.h>
#include <OpenHome/Configuration/Tests/ConfigRamStore.h>
#include <OpenHome/Av/Utils/IconDriverSongcastSender.h>
#include "IconOpenHome.h"
#include <OpenHome/Private/Parser.h>
#include <OpenHome/Media/Debug.h>
#include <OpenHome/Av/Debug.h>
#include <OpenHome/Av/Credentials.h>
#include <OpenHome/Media/Pipeline/Pipeline.h>
#include <OpenHome/Web/WebAppFramework.h>
#include <OpenHome/Web/ConfigUi/ConfigUiMediaPlayer.h>
#include <OpenHome/Web/ConfigUi/FileResourceHandler.h>
#include <OpenHome/Av/UpnpAv/FriendlyNameUpnpAv.h>
#include <OpenHome/Net/Odp/DviServerOdp.h>
#include <OpenHome/Net/Odp/DviProtocolOdp.h>
#include <OpenHome/Private/Debug.h>
#include "DriverAlsa.h"

#include <vector>

#undef LPEC_ENABLE

using namespace OpenHome;
using namespace OpenHome::Av;
using namespace OpenHome::Av::Test;
using namespace OpenHome::Configuration;
using namespace OpenHome::Media;
using namespace OpenHome::Net;
using namespace OpenHome::TestFramework;
using namespace OpenHome::Web;

// VolumeProfile

TUint VolumeProfile::VolumeMax() const
{
    return kVolumeMax;
}

TUint VolumeProfile::VolumeDefault() const
{
    return kVolumeDefault;
}

TUint VolumeProfile::VolumeUnity() const
{
    return kVolumeUnity;
}

TUint VolumeProfile::VolumeDefaultLimit() const
{
    return kVolumeDefaultLimit;
}

TUint VolumeProfile::VolumeStep() const
{
    return kVolumeStep;
}

TUint VolumeProfile::VolumeMilliDbPerStep() const
{
    return kVolumeMilliDbPerStep;
}

TUint VolumeProfile::ThreadPriority() const
{
    return kThreadPriority;
}

TUint VolumeProfile::BalanceMax() const
{
    return kBalanceMax;
}

TUint VolumeProfile::FadeMax() const
{
    return kFadeMax;
}

TUint VolumeProfile::OffsetMax() const
{
    return kOffsetMax;
}

TBool VolumeProfile::AlwaysOn() const
{
    return kAlwaysOn;
}

IVolumeProfile::StartupVolume VolumeProfile::StartupVolumeConfig() const
{
    return StartupVolume::LastUsed;
    //return IVolumeProfile::StartupVolume::Both;
}


// VolumeSinkLogger

void VolumeSinkLogger::SetVolume(TUint aVolume)
{
    Log::Print("SetVolume: %u\n", aVolume);
}

void VolumeSinkLogger::SetBalance(TInt aBalance)
{
    Log::Print("SetBalance: %d\n", aBalance);
}

void VolumeSinkLogger::SetFade(TInt aFade)
{
    Log::Print("SetFade: %d\n", aFade);
}


// RebootLogger

void RebootLogger::Reboot(const Brx& aReason)
{
    Log::Print("\n\n\nRebootLogger::Reboot. Reason:\n%.*s\n\n\n", PBUF(aReason));
    // Exit the process; systemd (ohPlayer.service, Restart=always) restarts it.
    fflush(stdout);
    _exit(0);
}


// DummyRaatSignalPath

void DummyRaatSignalPath::RegisterObserver(IRaatSignalPathObserver& aObserver)
{
    aObserver.SignalPathChanged(*this);
}



// TestMediaPlayer

const Brn TestMediaPlayer::kSongcastSenderIconFileName("SongcastSenderIcon");
const TUint TestMediaPlayer::kTrackCount;
const TUint TestMediaPlayer::kNumOdpSessions;
const TUint TestMediaPlayer::kMinWebUiResourceThreads;
const TUint TestMediaPlayer::kMaxWebUiTabs;
const TUint TestMediaPlayer::kUiSendQueueSize;
const TUint TestMediaPlayer::kUiMsgBufCount;
const TUint TestMediaPlayer::kUiMsgBufBytes;
const TUint TestMediaPlayer::kMaxPinsDevice;
const TUint TestMediaPlayer::kFsFlushFreqMs;
const TUint TestMediaPlayer::kDsdMaxSampleRate;
const TUint TestMediaPlayer::kDsdSampleBlockWords;
const TUint TestMediaPlayer::kDsdPadBytesPerChunk;

TestMediaPlayer::TestMediaPlayer(Net::DvStack& aDvStack, Net::CpStack& aCpStack, const Brx& aUdn, const TChar* aRoom, const TChar* aProductName,
                                 const Brx& aTuneInPartnerId, const Brx& aTidalId, const Brx& aQobuzIdSecret, const Brx& aUserAgent,
                                 const TChar* aStoreFile, TUint aOdpPort, TUint aWebUiPort,
                                 TUint aMinWebUiResourceThreads, TUint aMaxWebUiTabs, TUint aUiSendQueueSize,
                                 TUint aUiMsgBufCount, TUint aUiMsgBufBytes)
    : iPullableClock(nullptr)
    , iPlaylistLoader(nullptr)
    , iRaatSignalPathObservable(nullptr)
    , iSemShutdown("TMPS", 0)
    , iDisabled("test", 0)
    , iTuneInPartnerId(aTuneInPartnerId)
    , iTidalValues(aTidalId)
    , iQobuzIdSecret(aQobuzIdSecret)
    , iUserAgent(aUserAgent)
    , iTxTimestamper(nullptr)
    , iRxTimestamper(nullptr)
    , iStoreFileWriter(nullptr)
    , iOdpPort(aOdpPort)
    , iOdpZeroConf(nullptr)
    , iServerOdp(nullptr)
    , iMinWebUiResourceThreads(aMinWebUiResourceThreads)
    , iMaxWebUiTabs(aMaxWebUiTabs)
    , iUiSendQueueSize(aUiSendQueueSize)
    , iUiMsgBufCount(aUiMsgBufCount)
    , iUiMsgBufBytes(aUiMsgBufBytes)
    , iLinnAmpVolume(nullptr)
{
    Log::Print("Shell running on port %u\n", aDvStack.Env().Shell()->Port());
    iInfoLogger = std::make_unique<Media::AllocatorInfoLogger>();

    // Do NOT set UPnP friendly name attributes at this stage.
    // (Wait until MediaPlayer is created so that friendly name can be observed.)

    aDvStack.AddProtocolFactory(new DviProtocolFactoryOdp());

    // Create UPnP device.
    // Friendly name not set here.
    iDevice = std::make_unique<DvDeviceStandard>(aDvStack, aUdn, static_cast<Net::IResourceManager&>(*this));
    iDevice->SetAttribute("Upnp.Domain", "av.openhome.org");
    iDevice->SetAttribute("Upnp.Type", "Source");
    iDevice->SetAttribute("Upnp.Version", "1");
    iDevice->SetAttribute("Upnp.Manufacturer", "OpenHome");
    iDevice->SetAttribute("Upnp.ModelName", "ohPlayer");
#ifdef LPEC_ENABLE
    iDevice->SetAttribute("Lpec.Name", "ohPipeline");
#endif
    iDevice->SetAttribute("Odp.Name", "Ds");

    // Create separate UPnP device for standard MediaRenderer.
    Bws<256> buf(aUdn);
    buf.Append("-MediaRenderer");
    iDeviceUpnpAv = std::make_unique<DvDeviceStandard>(aDvStack, buf);
    // Friendly name not set here.
    iDeviceUpnpAv->SetAttribute("Upnp.Domain", "upnp.org");
    iDeviceUpnpAv->SetAttribute("Upnp.Type", "MediaRenderer");
    iDeviceUpnpAv->SetAttribute("Upnp.Version", "1");
    iDeviceUpnpAv->SetAttribute("Upnp.Manufacturer", "OpenHome");
    iDeviceUpnpAv->SetAttribute("Upnp.ModelName", "ohPlayer");
#ifdef LPEC_ENABLE
    iDeviceUpnpAv->SetAttribute("Lpec.Name", "MediaRenderer");
#endif
    iDeviceUpnpAv->SetAttribute("Odp.Name", "MediaRenderer");

    // create read/write store.  This creates a number of static (constant) entries automatically
    iRamStore = std::make_unique<RamStore>(kSongcastSenderIconFileName);

    // create a read/write store using the new config framework
    iConfigRamStore = std::make_unique<ConfigRamStore>();
    if (Brn(aStoreFile).Bytes() > 0) {
        StoreFileReaderJson storeFileReader(aStoreFile);
        storeFileReader.Read(*iConfigRamStore);

        iStoreFileWriter = std::make_unique<StoreFileWriterJson>(aStoreFile);
        iConfigRamStore->AddStoreObserver(*iStoreFileWriter);
    }
    else {
        Log::Print("No store file parameter specified - will not attempt to load store values from file, and changes to store values will not be persisted.\n");
    }

    VolumeProfile volumeProfile;
    VolumeConsumer volumeInit;

    // Read Linn Amp settings from the config store (web config)
    Bws<LinnAmpConfig::kMaxSerialPortBytes> serialPortBuf;
    LinnAmpConfig::ReadSerialPortFromStore(*iConfigRamStore, serialPortBuf);

    // Volume routing, matching official Linn DS behaviour:
    //  - Volume.Enabled = Yes (default when unset): internal volume control,
    //    using the ALSA mixer. If the sound card has no mixer the volume is
    //    greyed out in control point apps (Volume.Enabled forced to No).
    //  - Volume.Enabled = No with a serial port configured: volume is
    //    delegated to an external Linn amplifier. If no amplifier responds,
    //    the volume stays greyed out.
    // While an amplifier is active, the MediaPlayer is given a store wrapper
    // that reports Volume.Enabled as Yes (ohPipeline only creates the Volume
    // provider when internal volume control is enabled). The real store keeps
    // the user's No, so the web ui reflects the intent and the next start
    // selects the amplifier branch again. No store value is ever forced.
    const TBool volumeEnabled = LinnAmpConfig::ReadVolumeControlEnabledFromStore(*iConfigRamStore, true);
    TBool linnAmpUsed = false;

    if (!volumeEnabled && serialPortBuf.Bytes() > 0) {
        // External Linn amplifier over RS232.
        std::string serialPortStr(reinterpret_cast<const char*>(serialPortBuf.Ptr()), serialPortBuf.Bytes());
        const TUint volumeLimit = LinnAmpConfig::ReadVolumeLimitFromStore(*iConfigRamStore, 100);
        iLinnAmpVolume = new OhLinnAmpVolume(serialPortStr, volumeLimit);
        if (iLinnAmpVolume->IsVolumeSupported()) {
            // Set the ALSA output level: attenuation is handled by the Linn amp
            const TUint alsaVolume = LinnAmpConfig::ReadAlsaVolumeFromStore(*iConfigRamStore, 100);
            iVolumeLogger.SetVolumeFixed(alsaVolume * 1024);
            volumeInit.SetVolume(*iLinnAmpVolume);
            volumeInit.SetBalance(*iLinnAmpVolume);
            volumeInit.SetFade(*iLinnAmpVolume);
            // Apply tone settings from the config store
            const TInt bass = LinnAmpConfig::ReadToneFromStore(*iConfigRamStore, LinnAmpConfig::kKeyBass, 0);
            const TInt treble = LinnAmpConfig::ReadToneFromStore(*iConfigRamStore, LinnAmpConfig::kKeyTreble, 0);
            iLinnAmpVolume->SetBass(bass);
            iLinnAmpVolume->SetTreble(treble);
            linnAmpUsed = true;
            Log::Print("Using Linn Amp volume control on %s\n", serialPortStr.c_str());
        } else {
            delete iLinnAmpVolume;
            iLinnAmpVolume = nullptr;
            // No amplifier detected: grey out the volume in control points.
            // Volume.Enabled already reads No, so the greyed-out state is
            // announced and persists to the next start.
            Log::Print("Linn Amp volume control failed - volume disabled\n");
            volumeInit.SetVolume(iVolumeSinkLogger);
            volumeInit.SetBalance(iVolumeSinkLogger);
            volumeInit.SetFade(iVolumeSinkLogger);
        }
    } else if (volumeEnabled) {
        // Internal volume control: use the ALSA mixer.
        if (iVolumeLogger.IsVolumeSupported()) {
            volumeInit.SetVolume(iVolumeLogger);
            volumeInit.SetBalance(iVolumeLogger);
            volumeInit.SetFade(iVolumeLogger);
        } else {
            // No mixer on the sound card: no volume control at all.
            // Grey out the volume in control points by disabling Volume.Enabled
            // before the MediaPlayer is created (ohPipeline only announces the
            // Volume attribute when internal volume control is enabled).
            Log::Print("Internal volume control enabled but sound card has no mixer - volume disabled\n");
            LinnAmpConfig::WriteVolumeControlEnabled(*iConfigRamStore, false);
            volumeInit.SetVolume(iVolumeSinkLogger);
            volumeInit.SetBalance(iVolumeSinkLogger);
            volumeInit.SetFade(iVolumeSinkLogger);
        }
    } else {
        // Volume control disabled and no external amplifier: greyed out.
        Log::Print("Internal volume control disabled - volume greyed out\n");
        volumeInit.SetVolume(iVolumeSinkLogger);
        volumeInit.SetBalance(iVolumeSinkLogger);
        volumeInit.SetFade(iVolumeSinkLogger);
    }

    // Create MediaPlayer.
    // NOTE: If values for Room.Name and Product.Name already exist in the Store,
    //       the aRoom and aProductName default values will be ignored.
    // Pipeline buffer tuning (web config UI, Songcast tab). Values are read from
    // the store so that changes made in the web config UI are applied at the next
    // start.
    const TUint starvationRamperMinJiffies = PipelineConfig::ReadStarvationRamperMinSizeJiffiesFromStore(*iConfigRamStore);
    const TUint encodedReservoirBytes = PipelineConfig::ReadEncodedReservoirSizeBytesFromStore(*iConfigRamStore);
    Log::Print("Pipeline buffers: StarvationRamperMinSize=%u jiffies (~%ums), EncodedReservoirSize=%u bytes\n",
               starvationRamperMinJiffies, starvationRamperMinJiffies / Jiffies::kPerMs,
               encodedReservoirBytes);
    auto pipelineInit = PipelineInitParams::New();
    pipelineInit->SetStarvationRamperMinSize(starvationRamperMinJiffies); // larger StarvationRamper size useful for desktop
                                                                     // platforms with slightly unpredictable thread scheduling
    pipelineInit->SetEncodedReservoirSize(encodedReservoirBytes);
    pipelineInit->SetGorgerDuration(pipelineInit->DecodedReservoirJiffies());
    pipelineInit->SetDsdMaxSampleRate(kDsdMaxSampleRate);
    pipelineInit->SetSupportElements(Media::EPipelineSupportElementsValidatorMinimal | Media::EPipelineSupportElementsDecodedAudioValidator | Media::EPipelineSupportElementsRampValidator);
    const Brn kFriendlyNamePrefix("OpenHome ");
    iAudioTime = std::make_unique<AudioTimeCpu>(aDvStack.Env());
    auto mpInit = MediaPlayerInitParams::New(Brn(aRoom), Brn(aProductName), kFriendlyNamePrefix);
    mpInit->EnableConfigApp();
    mpInit->EnablePins(kMaxPinsDevice);
    // When a Linn amp handles volume, report Volume.Enabled as Yes to the
    // MediaPlayer only (see StoreWithOverriddenValue); the real store keeps
    // the user's setting so the web ui and the next start are unaffected.
    static const TByte kVolumeEnabledYes[] = { 0, 0, 0, 1 }; // big-endian eStringIdYes
    if (linnAmpUsed) {
        iMediaPlayerStore = std::make_unique<StoreWithOverriddenValue>(
            *iConfigRamStore, Brn("Volume.Enabled"), Brn(kVolumeEnabledYes, sizeof(kVolumeEnabledYes)));
    }
    else {
        iMediaPlayerStore = nullptr;
    }
    IStoreReadWrite& mediaPlayerStore = (iMediaPlayerStore != nullptr)
        ? static_cast<IStoreReadWrite&>(*iMediaPlayerStore)
        : static_cast<IStoreReadWrite&>(*iConfigRamStore);
    iMediaPlayer = std::make_unique<MediaPlayer>(aDvStack, aCpStack, *iDevice, *iRamStore,
                                   mediaPlayerStore, pipelineInit, *iAudioTime,
                                   volumeInit, volumeProfile,
                                   *iInfoLogger,
                                   aUdn, mpInit);
    delete mpInit;

    // Radio Browser media server: exposes the community radio directory
    // (radio-browser.info) to UPnP/DLNA control points for browsing/search;
    // stations play on this renderer through its usual protocols (http/hls).
    // UPnP MediaServer UDN must be a canonical uuid (8-4-4-4-12 hex);
    // some control points (e.g. Linn app) ignore devices whose UDN is not a
    // well-formed uuid. Derive a stable one from the renderer udn.
    // DvDeviceStandard adds the "uuid:" prefix itself; pass the bare uuid.
    Bws<64> radioBrowserUdn;
    {
        // simple deterministic 128-bit hash of the renderer udn (FNV-1a x2)
        TUint64 h1 = 0xcbf29ce484222325ULL;
        TUint64 h2 = 0x9e3779b97f4a7c15ULL;
        for (TUint i = 0; i < aUdn.Bytes(); i++) {
            h1 ^= aUdn[i]; h1 *= 0x100000001b3ULL;
            h2 ^= (TUint64(aUdn[i]) << 32) | aUdn[i]; h2 *= 0x100000001b3ULL;
        }
        const TUint64 ms[] = { h1, h2 };
        const TByte* p = reinterpret_cast<const TByte*>(ms);
        for (TUint i = 0; i < 16; i++) {
            if (i == 4 || i == 6 || i == 8 || i == 10) {
                radioBrowserUdn.Append('-');
            }
            Ascii::AppendHex(radioBrowserUdn, p[i]);
        }
    }
    iRadioBrowserMediaServer = std::make_unique<RadioBrowserMediaServer>(
        aDvStack, iMediaPlayer->Env(), iMediaPlayer->Ssl(), iUserAgent,
        radioBrowserUdn, Brn(aProductName));
    iRadioBrowserMediaServer->Start();

    // Register Linn Amp web config parameters (volume control choice and serial port)
    iLinnAmpConfig = std::make_unique<LinnAmpConfig>(iMediaPlayer->ConfigInitialiser());

    // Register pipeline buffer web config parameters (applied at next start)
    iPipelineConfig = std::make_unique<PipelineConfig>(iMediaPlayer->ConfigInitialiser());

    // Create the static radio preset provider here so its config values are
    // registered before the web config app is created (AddConfigApp runs in
    // Run()). The provider itself is registered with MediaPlayer's
    // RadioPresets in InitialiseSubsystems, after the Radio source is added.
    iRadioPresetsStatic = std::make_unique<RadioPresetsStatic>(iMediaPlayer->ConfigInitialiser());
    
    iDriver = std::make_unique<DriverAlsa>(iMediaPlayer->Pipeline(), 22052);
#if 1
    if (iDriver == NULL)
    {
        Log::Print("%s:%d - Unable to start DriverAlsa\n", __FILE__, __LINE__);
    }
    else
    {
        Log::Print("%s:%d - DriverAlsa started !\n", __FILE__, __LINE__);
    }
#endif
    iPipelineObserver = std::make_unique<LoggingPipelineObserver>();
    iMediaPlayer->Pipeline().AddObserver(*iPipelineObserver);
    Log::Print("%s:%d\n", __FILE__, __LINE__);
    iFnUpdaterStandard = std::make_unique<FriendlyNameAttributeUpdater>(iMediaPlayer->FriendlyNameObservable(), iMediaPlayer->ThreadPool(), *iDevice);
    iFnManagerUpnpAv =  std::make_unique<FriendlyNameManagerUpnpAv>(kFriendlyNamePrefix, iMediaPlayer->Product());
    iFnUpdaterUpnpAv = std::make_unique<FriendlyNameAttributeUpdater>(*iFnManagerUpnpAv, iMediaPlayer->ThreadPool(), *iDeviceUpnpAv);
    iFsFlushPeriodic = std::make_unique<FsFlushPeriodic>(iMediaPlayer->Env(), iMediaPlayer->PowerManager(), iMediaPlayer->ThreadPool(), kFsFlushFreqMs);
    Log::Print("%s:%d\n", __FILE__, __LINE__);
    // Register with the PowerManager
    IPowerManager& powerManager = iMediaPlayer->PowerManager();
    iPowerObserver = powerManager.RegisterPowerHandler(*this, kPowerPriorityLowest, "TestMediaPlayer");
    iStandbyObserver = powerManager.RegisterStandbyHandler(*this, kStandbyHandlerPriorityNormal, "TestMediaPlayer");
    Log::Print("%s:%d\n", __FILE__, __LINE__);
    // Register Raspdac interface notification
    iRaspdacObserver = std::make_unique<RaspdacObserver>();
    iMediaPlayer->Pipeline().AddObserver(*iRaspdacObserver);
    iRaspdacVolumeObserver = std::make_unique<RaspdacVolumeObserver>();
    iMediaPlayer->VolumeManager().AddVolumeObserver(*iRaspdacVolumeObserver);


    // Set up config app.
    WebAppFrameworkInitParams* initParams = new WebAppFrameworkInitParams();
    initParams->SetServerPort(aWebUiPort);
    initParams->SetMinServerThreadsResources(aMinWebUiResourceThreads);
    initParams->SetMaxServerThreadsLongPoll(aMaxWebUiTabs);
    initParams->SetSendQueueSize(aUiSendQueueSize);
    iAppFramework = std::make_unique<WebAppFramework>(aDvStack.Env(), initParams, iMediaPlayer->ThreadPool());
    Log::Print("%s:%d\n", __FILE__, __LINE__);
}

TestMediaPlayer::~TestMediaPlayer()
{
    if (iDevice) {
        ASSERT(!iDevice->Enabled());
    }

    // Clean up Linn Amp volume control
    if (iLinnAmpVolume != nullptr) {
        delete iLinnAmpVolume;
    }

    // 2. Clean up dependencies before iStoreFileWriter and iConfigRamStore drop
    if (iConfigRamStore && iStoreFileWriter) {
        iConfigRamStore->RemoveStoreObserver(*iStoreFileWriter);
    }
}

void TestMediaPlayer::SetPullableClock(Media::IPullableClock& aPullableClock)
{
    iPullableClock = &aPullableClock;
}

void TestMediaPlayer::SetSongcastTimestampers(IOhmTimestamper& aTxTimestamper, IOhmTimestamper& aRxTimestamper)
{
    iTxTimestamper = &aTxTimestamper;
    iRxTimestamper = &aRxTimestamper;
}

void TestMediaPlayer::StopPipeline()
{
    TUint waitCount = 0;
    if (TryDisable(*iDevice)) {
        waitCount++;
    }
    if (TryDisable(*iDeviceUpnpAv)) {
        waitCount++;
    }
    while (waitCount > 0) {
        iDisabled.Wait();
        waitCount--;
    }
    iMediaPlayer->Quit();
    iSemShutdown.Signal();
}

void TestMediaPlayer::AddAttribute(const TChar* aAttribute)
{
    iMediaPlayer->AddAttribute(aAttribute);
}
void TestMediaPlayer::RequestExit()
{
	iExitRequested=1;
}

void TestMediaPlayer::Run()
{
    RegisterPlugins(iMediaPlayer->Env());
    AddConfigApp();

    InitialiseLogger();
    iMediaPlayer->Start(iRebootHandler);
    InitialiseSubsystems();

    // Debugging for ConfigManager.
    // NOTE: do NOT call configManager.DumpToStore() here: it rewrites every
    // in-memory ConfigVal back to the store, which would clobber the
    // Volume.Enabled=No restoration done for Linn amp setups (see the volume
    // routing in the constructor).
    IConfigManager& configManager = iMediaPlayer->ConfigManager();
    configManager.Print();

    iAppFramework->Start();

    iServerOdp = std::make_unique<DviServerOdp>(iMediaPlayer->DvStack(), kNumOdpSessions, iOdpPort);
    iServerOdp->Start();
    Log::Print("ODP server running on port %u\n", iServerOdp->Port()); // don't use iOdpPort here - if it is 0, iServerOdp->Port() tells us the host assigned port
    iOdpZeroConf =std::make_unique<OdpZeroConf>(iMediaPlayer->Env(), *iServerOdp, iMediaPlayer->FriendlyNameObservable());
    iOdpZeroConf->SetZeroConfEnabled(true);

    EnableDevices();
    iFsFlushPeriodic->Start();

    StorePrinter storePrinter(*iConfigRamStore);
    storePrinter.Print();

    Log::Print("\nFull (software) media player\n");
    Log::Print("Intended to be controlled via a separate, standard CP (Kazoo etc.)\n");

    Log::Print("Press CTRL+C to quit:\n");
    Log::Print("\n");
#if 1
    iSemShutdown.Wait();
#else    
    while (iExitRequested==0)    // getchar catches stdin, getch does not
    {
	    sleep(1);

    }
#endif
    //IPowerManager& powerManager = iMediaPlayer->PowerManager();
    //powerManager.PowerDown(); // FIXME - this should probably be replaced by a normal shutdown procedure
    storePrinter.Print();
}

void TestMediaPlayer::RunWithSemaphore()
{
    RegisterPlugins(iMediaPlayer->Env());
    AddConfigApp();
    iMediaPlayer->Start(iRebootHandler);
    InitialiseSubsystems();

    // Debugging for ConfigManager.
    // NOTE: do NOT call configManager.DumpToStore() here: it rewrites every
    // in-memory ConfigVal back to the store, which would clobber the
    // Volume.Enabled=No restoration done for Linn amp setups (see the volume
    // routing in the constructor).
    IConfigManager& configManager = iMediaPlayer->ConfigManager();
    configManager.Print();

    iAppFramework->Start();
    EnableDevices();

    StorePrinter storePrinter(*iConfigRamStore);
    storePrinter.Print();

    iSemShutdown.Wait();    // FIXME - can Run() and RunWithSemaphore() be refactored out? only difference is how they wait for termination signal

    //IPowerManager& powerManager = iMediaPlayer->PowerManager();
    //powerManager.PowerDown(); // FIXME - this should probably be replaced by a normal shutdown procedure
    storePrinter.Print();
}

PipelineManager& TestMediaPlayer::Pipeline()
{
    return iMediaPlayer->Pipeline();
}

DvDeviceStandard* TestMediaPlayer::Device()
{
    return iDevice.get();
}

TUint TestMediaPlayer::DsdMaxSampleRate() const
{
    return kDsdMaxSampleRate;
}

TUint TestMediaPlayer::DsdSampleBlockWords() const
{
    return kDsdSampleBlockWords;
}

TUint TestMediaPlayer::DsdPadBytesPerChunk() const
{
    return kDsdPadBytesPerChunk;
}

void TestMediaPlayer::TryRegisterVorbis()
{
    iMediaPlayer->Add(Codec::CodecFactory::NewVorbis(iMediaPlayer->MimeTypes()));
}

void TestMediaPlayer::RegisterPlugins(Environment& aEnv)
{
    // Add containers
    iMediaPlayer->Add(Codec::ContainerFactory::NewId3v2());
    iMediaPlayer->Add(Codec::ContainerFactory::NewMpeg4(iMediaPlayer->MimeTypes(), Optional<Codec::IMpegDRMProvider>()));
    iMediaPlayer->Add(Codec::ContainerFactory::NewMpegTs(iMediaPlayer->MimeTypes()));

    // Add codecs
    iMediaPlayer->Add(Codec::CodecFactory::NewFlac(iMediaPlayer->MimeTypes()));
    iMediaPlayer->Add(Codec::CodecFactory::NewWav(iMediaPlayer->MimeTypes()));
    iMediaPlayer->Add(Codec::CodecFactory::NewAiff(iMediaPlayer->MimeTypes()));
    iMediaPlayer->Add(Codec::CodecFactory::NewAifc(iMediaPlayer->MimeTypes()));
    iMediaPlayer->Add(Codec::CodecFactory::NewAacFdkMp4(iMediaPlayer->MimeTypes()));
    iMediaPlayer->Add(Codec::CodecFactory::NewAacFdkAdts(iMediaPlayer->MimeTypes()));
    iMediaPlayer->Add(Codec::CodecFactory::NewAlacApple(iMediaPlayer->MimeTypes()));
    iMediaPlayer->Add(Codec::CodecFactory::NewDsdDsf(iMediaPlayer->MimeTypes(), kDsdSampleBlockWords, kDsdPadBytesPerChunk));
    iMediaPlayer->Add(Codec::CodecFactory::NewDsdDff(iMediaPlayer->MimeTypes(), kDsdSampleBlockWords, kDsdPadBytesPerChunk));
    iMediaPlayer->Add(Codec::CodecFactory::NewPcm());
    iMediaPlayer->Add(Codec::CodecFactory::NewDsdRaw(kDsdSampleBlockWords, kDsdPadBytesPerChunk));
    TryRegisterVorbis();
    // RAOP source must be added towards end of source list.
    // However, must add RAOP codec before MP3 codec to avoid false-positives.
    iMediaPlayer->Add(Codec::CodecFactory::NewRaop());
    // Add MP3 codec last, as it can cause false-positives (with RAOP in particular).
    iMediaPlayer->Add(Codec::CodecFactory::NewMp3(iMediaPlayer->MimeTypes()));
    //iMediaPlayer->Add(Codec::CodecFactory::NewDsdDff(iMediaPlayer->MimeTypes())); This line was included when modification began, but is defined above.

    // Add protocol modules (Radio source can require several stacked Http instances)
    auto& ssl = iMediaPlayer->Ssl();
    static const TUint kNumHttpProtocols = 5;
    for (TUint i=0; i<kNumHttpProtocols; i++) {
        iMediaPlayer->Add(ProtocolFactory::NewHttp(aEnv, ssl, iUserAgent));
    }
    iMediaPlayer->Add(ProtocolFactory::NewHls(aEnv, ssl, iUserAgent));

    // only add Tidal if we have a token to use with login
    if (iTidalValues.Bytes() > 0) {
        Parser p(iTidalValues);
        Brn clientId(p.Next(':'));
        Brn clientSecret(p.Next(':'));
        std::vector<OAuthAppDetails> apps;

        while(!p.Finished())
        {
            Brn appId = p.Next(':');
            Brn appClientId = p.Next(':');
            Brn appClientSecret = p.Next(':');

            apps.push_back(OAuthAppDetails(appId, appClientId, appClientSecret));
        }

        Log::Print("TIDAL: clientId = %.*s, clientSecret = %.*s\n", PBUF(clientId), PBUF(clientSecret));
        for(const auto& v : apps)
        {
            Log::Print("    App: ID: %.*s - ClientId = %.*s, Secret = %.*s\n", PBUF(v.AppId()), PBUF(v.ClientId()), PBUF(v.ClientSecret()));
        }

        iMediaPlayer->Add(ProtocolFactory::NewTidal(ssl, clientId, clientSecret, apps, *iMediaPlayer));
    }
    // ...likewise, only add Qobuz if we have ids for login
    if (iQobuzIdSecret.Bytes() > 0) {
        Parser p(iQobuzIdSecret);
        Brn appId(p.Next(':'));
        Brn appSecret(p.Remaining());
        Log::Print("Qobuz: appId = ");
        Log::Print(appId);
        Log::Print(", appSecret = ");
        Log::Print(appSecret);
        Log::Print("\n");
        iMediaPlayer->Add(ProtocolFactory::NewQobuz(appId, appSecret, *iMediaPlayer, iUserAgent));
    }
    iMediaPlayer->Add(ProtocolFactory::NewCalmRadio(aEnv, ssl, iUserAgent, *iMediaPlayer));

    // Add sources
    iMediaPlayer->Add(SourceFactory::NewPlaylist(*iMediaPlayer, Optional<IPlaylistLoader>(iPlaylistLoader.get())));
    if (iTuneInPartnerId.Bytes() == 0) {
        iMediaPlayer->Add(SourceFactory::NewRadio(*iMediaPlayer));
    }
    else {
        iMediaPlayer->Add(SourceFactory::NewRadio(*iMediaPlayer, iTuneInPartnerId));
    }
    // Register the static station list (Radio France) as a radio preset provider.
    // Selectable in the web config ui (Radio.PresetProvider) alongside TuneIn
    // when configured. Must be added after the Radio source and before the
    // pipeline starts (RadioPresets::Start runs on SourceRadio::Started).
    auto radioPresets = iMediaPlayer->RadioPresets();
    if (radioPresets.Ok()) {
        // RadioPresets takes ownership of the provider (deletes it in its
        // destructor), so release our unique_ptr when handing it over.
        radioPresets.Unwrap().AddProvider(iRadioPresetsStatic.release());
    }

    // Register radio/url pin invokers so radio presets and stream urls can be
    // pinned from control point apps. ohPipeline only registers these when a
    // TuneIn partner id is configured (SourceRadio.cpp); register them here
    // so pinning works without TuneIn. PinsInvocable takes ownership of the
    // invokers (deletes them in its destructor).
    auto pinsInvocable = iMediaPlayer->PinsInvocable();
    if (pinsInvocable.Ok()) {
        pinsInvocable.Unwrap().Add(new RadioPins(*iDevice, iMediaPlayer->CpStack()));
        pinsInvocable.Unwrap().Add(new UrlPins(*iDevice, iMediaPlayer->CpStack(), iMediaPlayer->ThreadPool()));
    }

    iMediaPlayer->Add(SourceFactory::NewUpnpAv(*iMediaPlayer, *iDeviceUpnpAv));

    Bwh hostName(iDevice->Udn().Bytes()+1); // space for null terminator
    hostName.Replace(iDevice->Udn());
    Bws<12> macAddr;
    MacAddrFromUdn(aEnv, macAddr);

    TUint priorityFiller = 0;
    TUint priorityFlywheelRamper = 0;
    TUint priorityStarvationRamper = 0;
    TUint priorityCodec = 0;
    TUint priorityEvent = 0;
    iMediaPlayer->Pipeline().GetThreadPriorities(priorityFiller, priorityFlywheelRamper, priorityStarvationRamper, priorityCodec, priorityEvent);
    const TUint raopServerPriority = priorityFiller;
    iMediaPlayer->Add(SourceFactory::NewRaop(*iMediaPlayer, Optional<IClockPuller>(nullptr), macAddr, raopServerPriority, *iMediaPlayer->Env().MdnsProvider()));
    iMediaPlayer->Add(SourceFactory::NewReceiver(*iMediaPlayer,
                                                 Optional<IClockPuller>(nullptr),
                                                 Optional<IOhmTimestamper>(iTxTimestamper),
                                                 Optional<IOhmTimestamper>(iRxTimestamper),
                                                 Optional<IOhmMsgProcessor>()));

#ifdef RAAT_ENABLE
    ASSERT(iPullableClock != nullptr);
    iMediaPlayer->Add(SourceFactory::NewRoon(
        *iMediaPlayer,
        *iAudioTime,
        *iPullableClock,
        iRaatSignalPathObservable != nullptr ? iRaatSignalPathObservable : new DummyRaatSignalPath(),
        Brn("12345"),
        Brn("0.0.1"),
        Brn("http://linn.co.uk/account")));
    iRaatSignalPathObservable = nullptr;
#else
    iMediaPlayer->Add(SourceFactory::NewScd(*iMediaPlayer, nullptr));
#endif
}

void TestMediaPlayer::InitialiseSubsystems()
{
}

IWebApp* TestMediaPlayer::CreateConfigApp(const std::vector<const Brx*>& aSources, const Brx& aResourceDir, TUint aMinWebUiResourceThreads, TUint aMaxWebUiTabs, TUint aMaxSendQueueSize, TUint aMsgBufCount, TUint aMsgBufBytes)
{
    FileResourceHandlerFactory resourceHandlerFactory;
    Brn linnAmpModel(iLinnAmpVolume != nullptr ? iLinnAmpVolume->DeviceModel() : Brx::Empty());
    Brn linnAmpSerial(iLinnAmpVolume != nullptr ? iLinnAmpVolume->DeviceId() : Brx::Empty());
    return new ConfigAppPlayerLinn(*iInfoLogger, iMediaPlayer->Env(), iMediaPlayer->Product(),
                                   iMediaPlayer->ConfigManager(), resourceHandlerFactory, aSources,
                                   Brn("Softplayer"), aResourceDir,
                                   aMinWebUiResourceThreads, aMaxWebUiTabs, aMaxSendQueueSize,
                                   aMsgBufCount, aMsgBufBytes, iRebootHandler,
                                   linnAmpModel, linnAmpSerial,
                                   iLinnAmpVolume != nullptr);
}

void TestMediaPlayer::InitialiseLogger()
{
    (void)iMediaPlayer->BufferLogOutput(128 * 1024, *(iMediaPlayer->Env().Shell()), Optional<ILogPoster>(nullptr));
}

void TestMediaPlayer::EnableDevices()
{
    iDevice->SetEnabled();
    iDeviceUpnpAv->SetEnabled();
}

void TestMediaPlayer::DestroyAppFramework()
{
    iAppFramework.reset();
}

void TestMediaPlayer::WriteResource(const Brx& aUriTail, const TIpAddress& /*aInterface*/, std::vector<char*>& /*aLanguageList*/, IResourceWriter& aResourceWriter)
{
    if (aUriTail == kSongcastSenderIconFileName) {
        aResourceWriter.WriteResourceBegin(sizeof(kIconOpenHome), kIconOpenHomeMimeType);
        aResourceWriter.WriteResource(kIconOpenHome, sizeof(kIconOpenHome));
        aResourceWriter.WriteResourceEnd();
    }
}

void TestMediaPlayer::PowerUp()
{
    // FIXME - enable UPnP devices here?
    // - would need to account for two-stage create->run process either by
    //  - setting a flag here which is checked in Run() OR
    //  - registering with IPowerManager in Run() call
    //iDevice->SetEnabled();
    //iDeviceUpnpAv->SetEnabled();
    Log::Print("TestMediaPlayer::PowerUp\n");
}

void TestMediaPlayer::StandbyEnabled()
{
    Log::Print("TestMediaPlayer::StandbyEnabled\n");
    if (iLinnAmpVolume != nullptr) {
        iLinnAmpVolume->SetStandby(true);
    }
}

void TestMediaPlayer::StandbyTransitioning()
{
}

void TestMediaPlayer::StandbyDisabled(StandbyDisableReason /*aReason*/)
{
    Log::Print("TestMediaPlayer::StandbyDisabled\n");
    if (iLinnAmpVolume != nullptr) {
        iLinnAmpVolume->SetStandby(false);
    }
}

void TestMediaPlayer::PowerDown()
{
    Log::Print("TestMediaPlayer::PowerDown\n");
    PowerDownDisable(*iDevice);
    PowerDownDisable(*iDeviceUpnpAv);
}

//void TestMediaPlayer::Add(IWebApp* aWebApp, FunctorPresentationUrl aFunctor)
//{
//    // Last added WebApp will be set as presentation page.
//    iAppFramework->Add(aWebApp, aFunctor);
//}

void TestMediaPlayer::AddConfigApp()
{
    Product& product = iMediaPlayer->Product();
    const auto sourceCount=product.SourceCount();
    std::vector<const Brx*> sourcesBufs(sourceCount);
    for (TUint i=0; i<sourceCount; i++) {
        Bws<ISource::kMaxSystemNameBytes> systemName;
        Bws<ISource::kMaxSourceNameBytes> name;
        Bws<ISource::kMaxSourceTypeBytes> type;
        TBool visible;
        product.GetSourceDetails(i, systemName, type, name, visible);
        sourcesBufs[i] = new Brh(systemName);
    }
    // FIXME - take resource dir as param or copy res dir to build dir
    auto configUi = CreateConfigApp(sourcesBufs, Brn("/usr/share/openhome-player/res/"), iMinWebUiResourceThreads, iMaxWebUiTabs, iUiSendQueueSize, iUiMsgBufCount, iUiMsgBufBytes);
    iAppFramework->Add(configUi, MakeFunctorGeneric(*this, &TestMediaPlayer::PresentationUrlChanged));
    iAppFramework->SetDefaultApp(configUi->ResourcePrefix());
    for (TUint i=0;i<sourcesBufs.size(); i++) {
        delete sourcesBufs[i];
    }
}

TUint TestMediaPlayer::Hash(const Brx& aBuf)
{
    TUint hash = 0;
    for (TUint i=0; i<aBuf.Bytes(); i++) {
        hash += aBuf[i];
    }
    return hash;
}

void TestMediaPlayer::GenerateMacAddr(Environment& aEnv, TUint aSeed, Bwx& aMacAddr)
{
    // Generate a 48-bit, 12-byte hex string.
    // Method:
    // - Generate two random numbers in the range 0 - 2^24
    // - Get the hex representation of these numbers
    // - Combine the two hex representations into the output buffer, aMacAddr
    const TUint maxLimit = 0x01000000;
    Bws<8> macBuf1;
    Bws<8> macBuf2;

    aEnv.SetRandomSeed(aSeed);
    TUint mac1 = aEnv.Random(maxLimit, 0);
    TUint mac2 = aEnv.Random(maxLimit, 0);

    Ascii::AppendHex(macBuf1, mac1);
    Ascii::AppendHex(macBuf2, mac2);

    aMacAddr.Append(macBuf1.Split(2));
    aMacAddr.Append(macBuf2.Split(2));
}

void TestMediaPlayer::MacAddrFromUdn(Environment& aEnv, Bwx& aMacAddr)
{
    TUint hash = Hash(iDevice->Udn());
    GenerateMacAddr(aEnv, hash, aMacAddr);
}

void TestMediaPlayer::PresentationUrlChanged(const Brx& aUrl)
{
    iPresentationUrl.Replace(aUrl);
    iDevice->SetAttribute("Upnp.PresentationUrl", iPresentationUrl.PtrZ());
    iMediaPlayer->Product().SetConfigAppUrl(iPresentationUrl);
}

void TestMediaPlayer::PowerDownDisable(DvDevice& aDevice)
{
    if (aDevice.Enabled()) {
        aDevice.SetDisabled(MakeFunctor(*this, &TestMediaPlayer::PowerDownUpnpCallback));
    }
}

void TestMediaPlayer::PowerDownUpnpCallback()
{
    // do nothing; only exists to avoid lengthy Upnp shutdown waits during power fail
}

TBool TestMediaPlayer::TryDisable(DvDevice& aDevice)
{
    if (aDevice.Enabled()) {
        aDevice.SetDisabled(MakeFunctor(*this, &TestMediaPlayer::Disabled));
        return true;
    }
    return false;
}

void TestMediaPlayer::Disabled()
{
    iDisabled.Signal();
}


// TestMediaPlayerInit

OpenHome::Net::Library* TestMediaPlayerInit::CreateLibrary(
    const TChar* aRoom,
    TBool aLoopback,
    TUint aAdapter,
    TUint aShellPort
)
{
    InitialisationParams* initParams = InitialisationParams::Create();
    // The mDNS host name must be unique per machine. It used to be the room
    // name (identical on every player unless -r was passed), causing
    // mStatus_NameConflict (-65548) crashes when several players run on the
    // same network. Derive it from the Linux host name (unique per machine)
    // with a distinct suffix so it also cannot clash with the system mDNS
    // daemon (avahi/mdnsd) announcing <hostname>.local.
    Bws<128> mdnsHost;
    mdnsHost.Append("ohp-");
    char linuxHostName[64] = {0};
    if (gethostname(linuxHostName, sizeof(linuxHostName) - 1) == 0 && linuxHostName[0] != '\0') {
        mdnsHost.Append(Brn(linuxHostName));
    }
    else {
        mdnsHost.Append(Brn(aRoom));
    }
    initParams->SetDvEnableBonjour(mdnsHost.PtrZ(), true);
    if (aLoopback == true) {
        initParams->SetUseLoopbackNetworkAdapter();
    }
    initParams->SetDvUpnpServerPort(51000);
    initParams->SetEnableShell(aShellPort);
    initParams->SetIPv6Supported(false);
#ifdef LPEC_ENABLE
    initParams->SetDvNumLpecThreads(4);
    initParams->SetDvLpecServerPort(2324);
#endif
    // Debug log levels. kSources logs track/artist display updates.
    // kMedia and kPipeline are disabled: they are extremely verbose
    // (every HLS segment fetch / pipeline element transition is logged)
    // and were only useful during bring-up.
    Debug::SetLevel(Debug::kSources);
    //Debug::AddLevel(Debug::kPipeline);
    //Debug::AddLevel(Debug::kMedia);
    Debug::AddLevel(Debug::kRaat);
    Debug::AddLevel(Debug::kAdapterChange);
    //Debug::AddLevel(Debug::kSongcast);
    Debug::AddLevel(Debug::kOAuth);
    Debug::SetSeverity(Debug::kSeverityInfo);
    Net::Library* lib = new Net::Library(initParams);
    //Net::DvStack* dvStack = lib->StartDv();
    std::vector<NetworkAdapter*>* subnetList = lib->CreateSubnetList();
    const TUint adapterIndex = aAdapter;
    if (subnetList->size() <= adapterIndex) {
        Log::Print("ERROR: adapter %u doesn't exist\n", adapterIndex);
        ASSERTS();
    }
    Log::Print ("adapter list:\n");
    for (unsigned i=0; i<subnetList->size(); ++i) {
        TIpAddress addr = (*subnetList)[i]->Address();
        Bws<TIpAddressUtils::kMaxAddressBytes> addressBuf;
        TIpAddressUtils::ToString(addr, addressBuf);
        Log::Print ("  %d: %.*s\n", i, PBUF(addressBuf));
    }
    //TIpAddress address = (*subnetList)[adapterIndex]->Address();
    TIpAddress subnet = (*subnetList)[adapterIndex]->Subnet();
    Library::DestroySubnetList(subnetList);
    lib->SetCurrentSubnet(subnet);

    Bws<TIpAddressUtils::kMaxAddressBytes> addressBuf;
    TIpAddressUtils::ToString(subnet, addressBuf);
    Log::Print("using subnet %.*s\n", PBUF(addressBuf));
    return lib;
}

void TestMediaPlayerInit::SeedRandomNumberGenerator(Environment& aEnv, const Brx& aRoom, TIpAddress aAddress, DviServerUpnp& aServer)
{
    if (aRoom == Brx::Empty()) {
        Log::Print("ERROR: room must be set\n");
        ASSERTS();
    }
    // Re-seed random number generator with hash of (unique) room name + UPnP
    // device server port to avoid UDN clashes.
    TUint port = aServer.Port(aAddress);
    Log::Print("UPnP DV server using port: %u\n", port);
    TUint hash = 0;
    for (TUint i=0; i<aRoom.Bytes(); i++) {
        hash += aRoom[i];
    }
    hash += port;
    Log::Print("Seeding random number generator with: %u\n", hash);
    aEnv.SetRandomSeed(hash);
}

void TestMediaPlayerInit::AppendUniqueId(Environment& aEnv, const Brx& aUserUdn, const Brx& aDefaultUdn, Bwh& aOutput)
{
    if (aUserUdn.Bytes() == 0) {
        if (aOutput.MaxBytes() < aDefaultUdn.Bytes()) {
            aOutput.Grow(aDefaultUdn.Bytes());
        }
        aOutput.Replace(aDefaultUdn);
        RandomiseUdn(aEnv, aOutput);
    }
    else {
        if (aUserUdn.Bytes() > aOutput.MaxBytes()) {
            aOutput.Grow(aUserUdn.Bytes());
        }
        aOutput.Replace(aUserUdn);
    }
}
