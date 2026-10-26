#include "RadioPresetsStatic.h"
#include <OpenHome/Ascii.h>
#include <OpenHome/Private/Printer.h>

using namespace OpenHome;

namespace OpenHome {
namespace Av {

const Brn RadioPresetsStatic::kDisplayName("Stations");

namespace {

// Default station list (Radio France hifi AAC streams), in preset order.
struct Station {
    const char* iName;
    const char* iUri;
};

static const Station kDefaultStations[RadioPresetsStatic::kNumPresets] = {
    { "France Inter",     "https://icecast.radiofrance.fr/franceinter-hifi.aac" },
    { "FIP",              "https://icecast.radiofrance.fr/fip-hifi.aac" },
    { "France Musique",   "https://icecast.radiofrance.fr/francemusique-hifi.aac" },
    { "France Info",      "https://icecast.radiofrance.fr/franceinfo-hifi.aac" },
    { "France Culture",   "https://icecast.radiofrance.fr/franceculture-hifi.aac" },
    { "Mouv'",            "https://icecast.radiofrance.fr/mouv-hifi.aac" },
};

} // namespace

// PresetValue

RadioPresetsStatic::PresetValue::PresetValue(Configuration::ConfigText& aConfig, RadioPresetsStatic& aOwner)
    : iConfig(aConfig)
    , iOwner(aOwner)
    , iValue(aConfig.MaxLength())
{
    // Subscribing immediately reports the current value, so iValue is filled
    // here. On subsequent changes the preset refresh is scheduled.
    iSubscriptionId = iConfig.Subscribe(MakeFunctorConfigText(*this, &PresetValue::ValueChanged));
}

RadioPresetsStatic::PresetValue::~PresetValue()
{
    iConfig.Unsubscribe(iSubscriptionId);
}

const Brx& RadioPresetsStatic::PresetValue::Value() const
{
    return iValue;
}

void RadioPresetsStatic::PresetValue::ValueChanged(Configuration::KeyValuePair<const Brx&>& aKvp)
{
    iValue.Replace(aKvp.Value());
    if (iOwner.iPresetWriter != nullptr) {
        iOwner.iPresetWriter->ScheduleRefresh();
    }
}

// RadioPresetsStatic

RadioPresetsStatic::RadioPresetsStatic(Configuration::IConfigInitialiser& aConfigInit)
    : iPresetWriter(nullptr)
{
    static const Brn kSuffixName(".Name");
    static const Brn kSuffixUri(".Uri");
    Bws<kKeyBytes> key;
    for (TUint i = 0; i < kNumPresets; i++) {
        const Station& station = kDefaultStations[i];

        GetPresetKey(i, kSuffixName, key);
        iNames.push_back(std::make_unique<Configuration::ConfigText>(
            aConfigInit, key, 0 /*aMinLength*/, kMaxNameBytes,
            Brn(station.iName), false /*aRebootRequired*/, Configuration::ConfigValAccess::Public));

        GetPresetKey(i, kSuffixUri, key);
        iUris.push_back(std::make_unique<Configuration::ConfigText>(
            aConfigInit, key, 0 /*aMinLength*/, kMaxUriBytes,
            Brn(station.iUri), false /*aRebootRequired*/, Configuration::ConfigValAccess::Public));
    }
}

RadioPresetsStatic::~RadioPresetsStatic()
{
    // PresetValues (and their subscriptions) must be destroyed before the
    // ConfigTexts they subscribe to.
    iNameValues.clear();
    iUriValues.clear();
}

void RadioPresetsStatic::GetPresetKey(TUint aIndex, const Brx& aSuffix, Bwx& aKey)
{
    static const Brn kPrefix("Radio.Preset");
    aKey.Replace(kPrefix);
    WriterBuffer writer(aKey);
    Ascii::StreamWriteUint(writer, aIndex + 1);
    writer.Write(aSuffix);
}

const Brx& RadioPresetsStatic::DisplayName() const
{
    return kDisplayName;
}

void RadioPresetsStatic::Activate(IRadioPresetWriter& aWriter)
{
    iPresetWriter = &aWriter;
    // Subscribe now: subscribing reports the current value immediately and
    // schedules a refresh whenever a preset is edited in the web config ui.
    // (Subscribing earlier is not possible: the writer is handed over on
    // Activate.)
    if (iNameValues.empty()) {
        for (TUint i = 0; i < kNumPresets; i++) {
            iNameValues.push_back(std::make_unique<PresetValue>(*iNames[i], *this));
            iUriValues.push_back(std::make_unique<PresetValue>(*iUris[i], *this));
        }
    }
}

void RadioPresetsStatic::Deactivate()
{
    iPresetWriter = nullptr;
}

void RadioPresetsStatic::RefreshPresets()
{
    if (iPresetWriter == nullptr) {
        return;
    }
    for (TUint i = 0; i < kNumPresets; i++) {
        const Brx& name = iNameValues[i]->Value();
        const Brx& uri = iUriValues[i]->Value();
        if (name.Bytes() == 0 || uri.Bytes() == 0) {
            continue; // empty name or uri leaves the preset unset
        }
        iPresetWriter->SetPreset(i, uri, name, Brx::Empty());
    }
}

} // namespace Av
} // namespace OpenHome
