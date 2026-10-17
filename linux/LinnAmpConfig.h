#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Configuration/ConfigManager.h>
#include <OpenHome/Configuration/IStore.h>
#include <memory>

namespace OpenHome {
namespace Av {

class LinnAmpConfig
{
public:
    static const Brn kKeyVolumeControl;
    static const Brn kKeySerialPort;
    static const TUint kVolumeControlAlsa;
    static const TUint kVolumeControlLinnAmp;
    static const TUint kMaxSerialPortBytes = 64;

    // Registers the web config parameters with the MediaPlayer's ConfigManager.
    // Must be called after MediaPlayer creation, before Start().
    LinnAmpConfig(Configuration::IConfigInitialiser& aConfigInit);
    ~LinnAmpConfig();

    // Reads startup values from the store (before MediaPlayer creation).
    static TBool ReadFromStore(Configuration::IStoreReadWrite& aStore, Bwx& aSerialPort,
                               TBool& aUseLinnAmp);

private:
    std::unique_ptr<Configuration::ConfigChoice> iConfigVolumeControl;
    std::unique_ptr<Configuration::ConfigText> iConfigSerialPort;
};

} // namespace Av
} // namespace OpenHome
