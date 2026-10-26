#include "PipelineConfig.h"
#include <OpenHome/Private/Converter.h>
#include <OpenHome/Private/Printer.h>

namespace OpenHome {
namespace Av {

const Brn PipelineConfig::kKeyStarvationRamperMinSize("Pipeline.StarvationRamperMinSize");
const Brn PipelineConfig::kKeyEncodedReservoirSize("Pipeline.EncodedReservoirSize");

// Out-of-class definitions: the constants are odr-used (passed by const
// reference to the Configuration::ConfigNum constructor), which requires
// a definition in addition to the in-class initialisation in the header.
const TUint PipelineConfig::kMinMs;
const TUint PipelineConfig::kMaxMs;
const TUint PipelineConfig::kDefaultStarvationRamperMinSizeMs;
const TUint PipelineConfig::kDefaultEncodedReservoirSizeMs;
const TUint PipelineConfig::kEncodedReservoirBytesPerMs;

PipelineConfig::PipelineConfig(Configuration::IConfigInitialiser& aConfigInit)
{
    // Registering a ConfigVal with the ConfigInitialiser makes it appear in the web config app
    iConfigStarvationRamperMinSize = std::make_unique<Configuration::ConfigNum>(
        aConfigInit, kKeyStarvationRamperMinSize, kMinMs, kMaxMs, kDefaultStarvationRamperMinSizeMs,
        true /* aRebootRequired: applied at next start */);
    iConfigEncodedReservoirSize = std::make_unique<Configuration::ConfigNum>(
        aConfigInit, kKeyEncodedReservoirSize, kMinMs, kMaxMs, kDefaultEncodedReservoirSizeMs,
        true /* aRebootRequired: applied at next start */);
}

PipelineConfig::~PipelineConfig()
{
}

TUint PipelineConfig::ReadStarvationRamperMinSizeMsFromStore(Configuration::IStoreReadWrite& aStore)
{
    return ReadNumFromStore(aStore, kKeyStarvationRamperMinSize, kDefaultStarvationRamperMinSizeMs);
}

TUint PipelineConfig::ReadEncodedReservoirSizeMsFromStore(Configuration::IStoreReadWrite& aStore)
{
    return ReadNumFromStore(aStore, kKeyEncodedReservoirSize, kDefaultEncodedReservoirSizeMs);
}

TUint PipelineConfig::ReadNumFromStore(Configuration::IStoreReadWrite& aStore, const Brx& aKey, TUint aDefault)
{
    // ConfigNum values are persisted as big-endian int32
    Bws<sizeof(TInt)> buf;
    try {
        aStore.Read(aKey, buf);
        TInt val = static_cast<TInt>(Converter::BeUint32At(buf, 0));
        if (val >= static_cast<TInt>(kMinMs) && val <= static_cast<TInt>(kMaxMs)) {
            return static_cast<TUint>(val);
        }
    }
    catch (StoreKeyNotFound&) {
    }
    return aDefault;
}

} // namespace Av
} // namespace OpenHome
