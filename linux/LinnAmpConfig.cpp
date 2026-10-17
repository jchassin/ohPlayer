#include "LinnAmpConfig.h"
#include <OpenHome/Private/Converter.h>

namespace OpenHome {
namespace Av {

const Brn LinnAmpConfig::kKeyVolumeControl("LinnAmp.VolumeControl");
const Brn LinnAmpConfig::kKeySerialPort("LinnAmp.SerialPort");
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

} // namespace Av
} // namespace OpenHome
