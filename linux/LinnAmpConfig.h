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
    static const Brn kKeyAlsaVolume;

    // ALSA output level (OpenHome volume steps, 0-100) used when volume is
    // controlled by an external Linn amplifier.

    // Tone control range (LR2 BAS/TRB commands)
    static const TInt kToneMin = -7;
    static const TInt kToneMax = 7;
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

    // Ensures Volume.Enabled is Yes in the store. Used when an external Linn
    // amplifier handles volume: ohPipeline only creates the Volume provider
    // (and announces the Volume attribute to control points) when internal
    // volume control is enabled, so it must stay enabled with the Linn amp
    // as the volume sink.
    static void EnsureVolumeControlEnabled(Configuration::IStoreReadWrite& aStore);

    // Reads Volume.Limit (max volume in OpenHome steps) from the store.
    // Returns aDefault if not set.
    static TUint ReadVolumeLimitFromStore(Configuration::IStoreReadWrite& aStore, TUint aDefault);

    // Reads LinnAmp.Bass / LinnAmp.Treble from the store. Returns aDefault if not set.
    static TInt ReadToneFromStore(Configuration::IStoreReadWrite& aStore, const Brx& aKey, TInt aDefault);

    // Reads LinnAmp.AlsaVolume (ALSA output level, 0-100) from the store.
    // Returns aDefault if not set.
    static TUint ReadAlsaVolumeFromStore(Configuration::IStoreReadWrite& aStore, TUint aDefault);

private:
    std::unique_ptr<Configuration::ConfigChoice> iConfigVolumeControl;
    std::unique_ptr<Configuration::ConfigText> iConfigSerialPort;
    std::unique_ptr<Configuration::ConfigNum> iConfigBass;
    std::unique_ptr<Configuration::ConfigNum> iConfigTreble;
    std::unique_ptr<Configuration::ConfigNum> iConfigAlsaVolume;
};

} // namespace Av
} // namespace OpenHome
