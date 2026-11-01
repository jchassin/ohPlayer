#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Media/Pipeline/Msg.h>
#include <OpenHome/Configuration/ConfigManager.h>
#include <OpenHome/Configuration/IStore.h>
#include <memory>

namespace OpenHome {
namespace Av {

// Advanced pipeline buffer tuning, exposed in the web config ui (Device tab,
// Advanced section). Values use the pipeline's native units:
//  - StarvationRamperMinSize: jiffies (Media::Jiffies::kPerMs = 56448; the
//    pipeline's PipelineInitParams::SetStarvationRamperMinSize takes jiffies
//    directly)
//  - EncodedReservoirSize: bytes (PipelineInitParams::SetEncodedReservoirSize
//    takes bytes directly)
// No unit conversion is performed anywhere: the configured value is passed
// through as-is to the pipeline.
class PipelineConfig
{
public:
    static const Brn kKeyStarvationRamperMinSize;
    static const Brn kKeyEncodedReservoirSize;
    static const Brn kKeyAlsaBufferUs;

    // Allowed ranges, in native units.
    // StarvationRamperMinSize: 20 ms (ohPipeline default) to 120 ms, in jiffies.
    // The Songcast protocol aligns a receiver on the media latency announced by
    // the sender (150 ms on Linn DS firmware): the PhaseAdjuster compensates
    // downstream buffer depth up to that target, but cannot pull a pipeline that
    // exceeds it back in sync. Keep StarvationRamperMinSize + DriverAlsaBuffer
    // below 150 ms so multiroom offset stays compensated.
    static const TUint kMinStarvationRamperJiffies = Media::Jiffies::kPerMs * 20;
    static const TUint kMaxStarvationRamperJiffies = Media::Jiffies::kPerMs * 120;
    static const TUint kMinEncodedReservoirBytes  = 64 * 1024;
    static const TUint kMaxEncodedReservoirBytes  = 8 * 1024 * 1024;

    // DriverAlsaBuffer: ALSA hardware buffer, in microseconds. Part of the
    // downstream latency (together with StarvationRamperMinSize): keep the sum
    // below the Songcast sender target (150 ms on Linn DS) - see the comment
    // above. Range 5 ms to 50 ms; default 12052 us (12 ms) - the value validated
    // by multiroom metronome measurements.
    static const TUint kMinAlsaBufferUs = 5000;
    static const TUint kMaxAlsaBufferUs = 50000;
    static const TUint kDefaultAlsaBufferUs = 12052;

    // Defaults = the values previously used by the code:
    //  - StarvationRamperMinSize was explicitly set to 100 ms in TestMediaPlayer.cpp.
    //  - EncodedReservoirSize was left at the ohPipeline default
    //    (PipelineInitParams::kEncodedReservoirSizeBytes = 1536 * 1024 bytes).
    static const TUint kDefaultStarvationRamperJiffies = Media::Jiffies::kPerMs * 100;
    static const TUint kDefaultEncodedReservoirBytes  = 1536 * 1024;

    // Registers the web config parameters with the MediaPlayer's ConfigManager.
    // Must be called after MediaPlayer creation, before Start().
    PipelineConfig(Configuration::IConfigInitialiser& aConfigInit);
    ~PipelineConfig();

    // Reads the values from the store (before MediaPlayer creation).
    // Returns the defaults if not set or out of range.
    static TUint ReadStarvationRamperMinSizeJiffiesFromStore(Configuration::IStoreReadWrite& aStore);
    static TUint ReadEncodedReservoirSizeBytesFromStore(Configuration::IStoreReadWrite& aStore);
    static TUint ReadAlsaBufferUsFromStore(Configuration::IStoreReadWrite& aStore);

private:
    static TUint ReadNumFromStore(Configuration::IStoreReadWrite& aStore, const Brx& aKey, TUint aMin, TUint aMax, TUint aDefault);
    std::unique_ptr<Configuration::ConfigNum> iConfigStarvationRamperMinSize;
    std::unique_ptr<Configuration::ConfigNum> iConfigEncodedReservoirSize;
    std::unique_ptr<Configuration::ConfigNum> iConfigAlsaBufferUs;
};

} // namespace Av
} // namespace OpenHome
