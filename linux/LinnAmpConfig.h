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

    // Reads Volume.Enabled from the store (ohPipeline choice, big-endian
    // uint32: 1 == Yes, 0 == No). Returns aDefault if not set.
    static TBool ReadVolumeControlEnabledFromStore(Configuration::IStoreReadWrite& aStore, TBool aDefault);

    // Writes Volume.Enabled to the store. ohPipeline only creates the Volume
    // provider (and announces the Volume attribute to control points) when
    // internal volume control is enabled, so writing No greys out the volume
    // in control point apps.
    static void WriteVolumeControlEnabled(Configuration::IStoreReadWrite& aStore, TBool aEnabled);

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
