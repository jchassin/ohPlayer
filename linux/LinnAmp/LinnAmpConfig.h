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
    static const Brn kKeySerialPort;
    static const Brn kKeyBass;
    static const Brn kKeyTreble;
    static const Brn kKeyAlsaVolume;
    static const Brn kKeySource;

    // Tone control range (LR2 BAS/TRB commands)
    static const TInt kToneMin = -7;
    static const TInt kToneMax = 7;
    // Amplifier input range (Majik INPUT[1..6], LR2 LISTEN 1..6)
    static const TInt kSourceMin = 1;
    static const TInt kSourceMax = 6;
    static const TUint kMaxSerialPortBytes = 64;

    // Registers the web config parameters with the MediaPlayer's ConfigManager.
    // Must be called after MediaPlayer creation, before Start().
    LinnAmpConfig(Configuration::IConfigInitialiser& aConfigInit);
    ~LinnAmpConfig();

    // Reads the serial port from the store (before MediaPlayer creation).
    // Returns an empty buffer if not set.
    static void ReadSerialPortFromStore(Configuration::IStoreReadWrite& aStore, Bwx& aSerialPort);

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

    // Reads LinnAmp.Source (amplifier input the renderer is connected to,
    // 1-6) from the store. Returns aDefault if not set.
    static TInt ReadSourceFromStore(Configuration::IStoreReadWrite& aStore, TInt aDefault);

private:
    std::unique_ptr<Configuration::ConfigText> iConfigSerialPort;
    std::unique_ptr<Configuration::ConfigNum> iConfigBass;
    std::unique_ptr<Configuration::ConfigNum> iConfigTreble;
    std::unique_ptr<Configuration::ConfigNum> iConfigAlsaVolume;
    std::unique_ptr<Configuration::ConfigNum> iConfigSource;
};

} // namespace Av
} // namespace OpenHome
