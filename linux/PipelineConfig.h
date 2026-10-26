#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Configuration/ConfigManager.h>
#include <OpenHome/Configuration/IStore.h>
#include <memory>

namespace OpenHome {
namespace Av {

class PipelineConfig
{
public:
    static const Brn kKeyStarvationRamperMinSize;
    static const Brn kKeyEncodedReservoirSize;

    // Allowed range for both parameters (milliseconds)
    static const TUint kMinMs = 100;
    static const TUint kMaxMs = 3000;

    // Defaults = the values previously used by the code:
    //  - StarvationRamperMinSize was explicitly set to 100 ms in TestMediaPlayer.cpp.
    //  - EncodedReservoirSize was left at the ohPipeline default
    //    (PipelineInitParams::kEncodedReservoirSizeBytes = 1536 * 1024 bytes),
    //    which converts back to exactly 1024 ms with the conversion below.
    static const TUint kDefaultStarvationRamperMinSizeMs = 100;
    static const TUint kDefaultEncodedReservoirSizeMs = 1024;

    // Conversion used to express the encoded reservoir (bytes) as a duration.
    // Worst-case PCM stream the reservoir must absorb: 192 kHz, 2 channels,
    // 4-byte subsamples => 1536 bytes per ms. 1024 ms therefore reproduces
    // exactly the ohPipeline default of 1536 * 1024 bytes.
    static const TUint kEncodedReservoirBytesPerMs = 1536;

    // Registers the web config parameters with the MediaPlayer's ConfigManager.
    // Must be called after MediaPlayer creation, before Start().
    PipelineConfig(Configuration::IConfigInitialiser& aConfigInit);
    ~PipelineConfig();

    // Reads the values from the store (before MediaPlayer creation).
    // Returns the defaults if not set or out of range.
    static TUint ReadStarvationRamperMinSizeMsFromStore(Configuration::IStoreReadWrite& aStore);
    static TUint ReadEncodedReservoirSizeMsFromStore(Configuration::IStoreReadWrite& aStore);

private:
    static TUint ReadNumFromStore(Configuration::IStoreReadWrite& aStore, const Brx& aKey, TUint aDefault);
    std::unique_ptr<Configuration::ConfigNum> iConfigStarvationRamperMinSize;
    std::unique_ptr<Configuration::ConfigNum> iConfigEncodedReservoirSize;
};

} // namespace Av
} // namespace OpenHome
