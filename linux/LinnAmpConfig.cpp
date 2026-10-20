#include "LinnAmpConfig.h"
#include <OpenHome/Private/Converter.h>
#include <OpenHome/Private/Printer.h>

namespace OpenHome {
namespace Av {

const Brn LinnAmpConfig::kKeyVolumeControl("LinnAmp.VolumeControl");
const Brn LinnAmpConfig::kKeySerialPort("LinnAmp.SerialPort");
const Brn LinnAmpConfig::kKeyBass("LinnAmp.Bass");
const Brn LinnAmpConfig::kKeyTreble("LinnAmp.Treble");
const Brn LinnAmpConfig::kKeyAlsaVolume("LinnAmp.AlsaVolume");
const TInt LinnAmpConfig::kToneMin;
const TInt LinnAmpConfig::kToneMax;
const TUint LinnAmpConfig::kVolumeControlAlsa = 0;
const TUint LinnAmpConfig::kVolumeControlLinnAmp = 1;
const TUint LinnAmpConfig::kMaxSerialPortBytes;

// Maps choice values to human-readable labels for the web config UI
class VolumeControlMapper : public Configuration::IConfigChoiceMapper
{
public:
    void Write(IWriter& aWriter, Configuration::IConfigChoiceMappingWriter& aMappingWriter) override
    {
        aMappingWriter.Write(aWriter, LinnAmpConfig::kVolumeControlAlsa, Brn("ALSA driver volume"));
        aMappingWriter.Write(aWriter, LinnAmpConfig::kVolumeControlLinnAmp, Brn("Linn amplifier"));
        aMappingWriter.WriteComplete(aWriter);
    }
};

static VolumeControlMapper gVolumeControlMapper;

LinnAmpConfig::LinnAmpConfig(Configuration::IConfigInitialiser& aConfigInit)
{
    // Registering a ConfigVal with the ConfigInitialiser makes it appear in the web config app
    std::vector<TUint> choices = { kVolumeControlAlsa, kVolumeControlLinnAmp };
    iConfigVolumeControl = std::make_unique<Configuration::ConfigChoice>(
        aConfigInit, kKeyVolumeControl, choices, kVolumeControlAlsa, gVolumeControlMapper,
        true /* aRebootRequired: applied at next start */);
    iConfigSerialPort = std::make_unique<Configuration::ConfigText>(
        aConfigInit, kKeySerialPort, 0 /* aMinLength */, kMaxSerialPortBytes, Brx::Empty(),
        true /* aRebootRequired: applied at next start */);
    iConfigBass = std::make_unique<Configuration::ConfigNum>(
        aConfigInit, kKeyBass, kToneMin, kToneMax, 0,
        true /* aRebootRequired: applied at next start */);
    iConfigTreble = std::make_unique<Configuration::ConfigNum>(
        aConfigInit, kKeyTreble, kToneMin, kToneMax, 0,
        true /* aRebootRequired: applied at next start */);
    iConfigAlsaVolume = std::make_unique<Configuration::ConfigNum>(
        aConfigInit, kKeyAlsaVolume, 0, 100, 100,
        true /* aRebootRequired: applied at next start */);
}

LinnAmpConfig::~LinnAmpConfig()
{
}

TBool LinnAmpConfig::ReadFromStore(Configuration::IStoreReadWrite& aStore, Bwx& aSerialPort,
                                   TBool& aUseLinnAmp)
{
    // Choice values are persisted as big-endian uint32
    Bws<sizeof(TUint)> choiceBuf;
    try {
        aStore.Read(kKeyVolumeControl, choiceBuf);
        aUseLinnAmp = (Converter::BeUint32At(choiceBuf, 0) == kVolumeControlLinnAmp);
    }
    catch (StoreKeyNotFound&) {
        aUseLinnAmp = false;
    }

    try {
        aStore.Read(kKeySerialPort, aSerialPort);
    }
    catch (StoreKeyNotFound&) {
        aSerialPort.Replace(Brx::Empty());
    }

    return aUseLinnAmp;
}

void LinnAmpConfig::EnsureVolumeControlEnabled(Configuration::IStoreReadWrite& aStore)
{
    // ohPipeline reads Volume.Enabled as a big-endian uint32 choice
    // (eStringIdYes == 1). Only write when it is currently disabled.
    static const Brn kKeyVolumeEnabled("Volume.Enabled");
    Bws<sizeof(TUint)> buf;
    try {
        aStore.Read(kKeyVolumeEnabled, buf);
        if (Converter::BeUint32At(buf, 0) == 1) {
            return;
        }
    }
    catch (StoreKeyNotFound&) {
    }

    const TUint32 val = 1; // eStringIdYes
    const TByte* ptr = reinterpret_cast<const TByte*>(&val);
    TByte be[sizeof(TUint)];
    be[0] = ptr[3];
    be[1] = ptr[2];
    be[2] = ptr[1];
    be[3] = ptr[0];
    aStore.Write(kKeyVolumeEnabled, Brn(be, sizeof(be)));
    Log::Print("LinnAmpConfig: volume control delegated to the Linn amplifier - forcing Volume.Enabled to Yes\n");
}

TInt LinnAmpConfig::ReadToneFromStore(Configuration::IStoreReadWrite& aStore, const Brx& aKey, TInt aDefault)
{
    // ConfigNum values are persisted as big-endian int32
    Bws<sizeof(TInt)> buf;
    try {
        aStore.Read(aKey, buf);
        TInt val = static_cast<TInt>(Converter::BeUint32At(buf, 0));
        if (val >= kToneMin && val <= kToneMax) {
            return val;
        }
    }
    catch (StoreKeyNotFound&) {
    }
    return aDefault;
}

TUint LinnAmpConfig::ReadAlsaVolumeFromStore(Configuration::IStoreReadWrite& aStore, TUint aDefault)
{
    // ConfigNum values are persisted as big-endian int32
    Bws<sizeof(TInt)> buf;
    try {
        aStore.Read(kKeyAlsaVolume, buf);
        TInt val = static_cast<TInt>(Converter::BeUint32At(buf, 0));
        if (val >= 0 && val <= 100) {
            return static_cast<TUint>(val);
        }
    }
    catch (StoreKeyNotFound&) {
    }
    return aDefault;
}

TUint LinnAmpConfig::ReadVolumeLimitFromStore(Configuration::IStoreReadWrite& aStore, TUint aDefault)
{
    // Key defined by ohPipeline (VolumeConfig::kKeyLimit), persisted as big-endian int32
    static const Brn kKeyVolumeLimit("Volume.Limit");
    Bws<sizeof(TInt)> buf;
    try {
        aStore.Read(kKeyVolumeLimit, buf);
        TInt limit = static_cast<TInt>(Converter::BeUint32At(buf, 0));
        if (limit >= 0 && limit <= 100) {
            return static_cast<TUint>(limit);
        }
    }
    catch (StoreKeyNotFound&) {
    }
    return aDefault;
}

} // namespace Av
} // namespace OpenHome
