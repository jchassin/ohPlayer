#pragma once

#include <OpenHome/Av/MediaPlayer.h>
#include <OpenHome/Av/FriendlyNameAdapter.h>
#include <OpenHome/Media/PipelineManager.h>
#include <OpenHome/Av/RebootHandler.h>
#include <OpenHome/Av/Songcast/OhmTimestamp.h>
#include <OpenHome/Av/UpnpAv/FriendlyNameUpnpAv.h>
#include <OpenHome/Av/VolumeManager.h>
#include <OpenHome/Web/ConfigUi/FileResourceHandler.h>
#include <OpenHome/Web/WebAppFramework.h>
#include "Volume.h"
#include "RaspdacMiniIF.h"

namespace OpenHome {
    class Shell;
    class ShellCommandDebug;
    
namespace Net {
    class DviServerUpnp;
    class DvStack;
    class CpStack;
    class DvDevice;
}
namespace Media {
    class PipelineManager;
    class DriverSongcastSender;
    class AllocatorInfoLogger;
}
namespace Configuration {
    class ConfigGTKKeyStore;
    class ConfigManager;
}
namespace Web {
    class ConfigAppMediaPlayer;
}
namespace Av {
    class RamStore;
    class ControlPointProxy;

// Helpers
static TBool CompareIPv4Addrs(const TIpAddress addr1,
                              const TIpAddress addr2)
{
    return addr1.iFamily == kFamilyV4
        && addr2.iFamily == kFamilyV4
        && addr1.iV4 == addr2.iV4;
}

static TBool CompareIPv6Addrs(const TIpAddress addr1,
                              const TIpAddress addr2)
{
    return addr1.iFamily == kFamilyV6
        && addr2.iFamily == kFamilyV6
        && memcmp((TByte*)addr1.iV6[0], (TByte*)addr2.iV6[0], 16) == 0;
}

    

class ExampleMediaPlayer : private Net::IResourceManager
{
    static const Brn   kIconOpenHomeFileName;
    static const TUint kMaxUiTabs       = 4;
    static const TUint kUiSendQueueSize = kMaxUiTabs * 200;
    static const TUint kShellPort       = 2323;
    static const TUint kOhMdpPort       = 4444;

public:
    ExampleMediaPlayer(Net::DvStack& aDvStack, Net::CpStack& aCpStack,
					   const Brx& aUdn,
                       const TChar* aRoom, const TChar* aProductName,
                       const Brx& aUserAgent);
    virtual ~ExampleMediaPlayer();

    Environment            &Env();
    void                    StopPipeline();
    TBool                   CanPlay();
    void                    PlayPipeline();
    TBool                   CanPause();
    void                    PausePipeline();
    TBool                   CanHalt();
    void                    HaltPipeline();
    void                    AddAttribute(const TChar* aAttribute);
    virtual void            RunWithSemaphore(Net::CpStack& aCpStack);
    void                    SetSongcastTimestampers(IOhmTimestamper& aTxTimestamper, IOhmTimestamper& aRxTimestamper);
    void                    SetSongcastTimestampMappers(IOhmTimestamper& aTxTsMapper, IOhmTimestamper& aRxTsMapper);
    Media::PipelineManager &Pipeline();
    Net::DvDeviceStandard  *Device();
    Net::DvDevice          *UpnpAvDevice();
private: // from Net::IResourceManager
    void WriteResource(const Brx& aUriTail, 
                       const TIpAddress& aInterface,
                       std::vector<char*>& aLanguageList,
                       Net::IResourceWriter& aResourceWriter) override;
private:
    void  RegisterPlugins(Environment& aEnv);
    void  AddConfigApp();
    void  PresentationUrlChanged(const Brx& aUrl);
    TBool TryDisable(Net::DvDevice& aDevice);
    void  Disabled();
protected:
    MediaPlayer                      *iMediaPlayer;
#ifdef DEBUG
    Media::IPipelineObserver         *iPipelineStateLogger;
#endif // DEBUG
    Media::IPipelineObserver           *iRaspdacObserver;
    RaspdacVolumeObserver        *iRaspdacVolumeObserver;
    Media::PipelineInitParams        *iInitParams;
    Media::AllocatorInfoLogger       *iInfoLogger;
    Net::DvDeviceStandard            *iDevice;
    Net::DvDevice                    *iDeviceUpnpAv;
    Av::FriendlyNameAttributeUpdater *iFnUpdaterStandard;
    FriendlyNameManagerUpnpAv        *iFnManagerUpnpAv;
    Av::FriendlyNameAttributeUpdater *iFnUpdaterUpnpAv;
    RamStore                         *iRamStore;
    Configuration::ConfigGTKKeyStore *iConfigStore;
    Semaphore                         iSemShutdown;
    Web::WebAppFramework             *iAppFramework;
    RebootLogger                      iRebootHandler;
private:
    Semaphore                  iDisabled;
    Av::VolumeControl          iVolume;
    ControlPointProxy         *iCpProxy;
    IOhmTimestamper           *iTxTimestamper;
    IOhmTimestamper           *iRxTimestamper;
    IOhmTimestamper           *iTxTsMapper;
    IOhmTimestamper           *iRxTsMapper;
    const Brx                 &iUserAgent;
    Web::FileResourceHandlerFactory iFileResourceHandlerFactory;
    Web::ConfigAppMediaPlayer *iConfigApp;
    Bws<Uri::kMaxUriBytes+1>   iPresentationUrl;
    Shell* iShell;
    ShellCommandDebug* iShellDebug;
};

class ExampleMediaPlayerInit
{
public:
    static OpenHome::Net::Library* CreateLibrary(TIpAddress preferredSubnet);
};

} // namespace Av
} // namespace OpenHome
