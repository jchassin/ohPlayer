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
    static const Brn kKeyBass;
    static const Brn kKeyTreble;

    // Tone control range (LR2 BAS/TRB commands)
    static const TInt kToneMin = -15;
    static const TInt kToneMax = 15;
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

    // Reads Volume.Limit (max volume in OpenHome steps) from the store.
    // Returns aDefault if not set.
    static TUint ReadVolumeLimitFromStore(Configuration::IStoreReadWrite& aStore, TUint aDefault);

    // Reads LinnAmp.Bass / LinnAmp.Treble from the store. Returns aDefault if not set.
    static TInt ReadToneFromStore(Configuration::IStoreReadWrite& aStore, const Brx& aKey, TInt aDefault);

private:
    std::unique_ptr<Configuration::ConfigChoice> iConfigVolumeControl;
    std::unique_ptr<Configuration::ConfigText> iConfigSerialPort;
    std::unique_ptr<Configuration::ConfigNum> iConfigBass;
    std::unique_ptr<Configuration::ConfigNum> iConfigTreble;
};

} // namespace Av
} // namespace OpenHome
