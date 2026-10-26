#include "RadioPresetsStatic.h"
#include <OpenHome/Private/Ascii.h>
#include <OpenHome/Private/Printer.h>

using namespace OpenHome;

namespace OpenHome {
namespace Av {

const Brn RadioPresetsStatic::kDisplayName("Stations");

// Out-of-class definitions: the constants are odr-used (passed by const
// reference to the Configuration::ConfigText constructor), which requires
// a definition in addition to the in-class initialisation in the header.
const TUint RadioPresetsStatic::kNumPresets;
const TUint RadioPresetsStatic::kMaxNameBytes;
const TUint RadioPresetsStatic::kMaxUriBytes;
const TUint RadioPresetsStatic::kMaxImageUriBytes;
const TUint RadioPresetsStatic::kKeyBytes;

namespace {

// Default station list (Radio France hifi AAC streams), in preset order.
struct Station {
    const char* iName;
    const char* iUri;
    const char* iImageUri;
};

static const Station kDefaultStations[RadioPresetsStatic::kNumPresets] = {
    { "France Inter",     "https://icecast.radiofrance.fr/franceinter-hifi.aac",   "https://commons.wikimedia.org/wiki/Special:FilePath/France_Inter_logo_2021.svg?width=250" },
    { "FIP",              "https://icecast.radiofrance.fr/fip-hifi.aac",            "https://commons.wikimedia.org/wiki/Special:FilePath/FIP_logo_2021.svg?width=250" },
    { "France Musique",   "https://icecast.radiofrance.fr/francemusique-hifi.aac",  "https://commons.wikimedia.org/wiki/Special:FilePath/France_Musique_logo_2021.svg?width=250" },
    { "France Info",      "https://icecast.radiofrance.fr/franceinfo-hifi.aac",     "https://commons.wikimedia.org/wiki/Special:FilePath/Franceinfo.svg?width=250" },
    { "France Culture",   "https://icecast.radiofrance.fr/franceculture-hifi.aac",  "https://commons.wikimedia.org/wiki/Special:FilePath/France_Culture_logo_2021.svg?width=250" },
    { "Mouv'",            "https://icecast.radiofrance.fr/mouv-hifi.aac",            "https://commons.wikimedia.org/wiki/Special:FilePath/Mouv_logo_2022.svg?width=250" },
    { "Radio Nacional",   "https://rtvelivestream.rtve.es/rtvesec/rne/rne_r1_main.m3u8", "https://graph.facebook.com/radionacionalrne/picture?width=200&height=200" },
    { "Radio Cl\u00e1sica RNE", "https://rtvelivestream.rtve.es/rtvesec/rne/rne_r2_main.m3u8", "https://graph.facebook.com/radioclasicartve/picture?width=200&height=200" },
    { "Radio 3 RNE",      "https://rtvelivestream.rtve.es/rtvesec/rne/rne_r3_main.m3u8", "https://graph.facebook.com/radio3/picture?width=200&height=200" },
    { "Radio 5 RNE",      "https://rtvelivestream.rtve.es/rtvesec/rne/rne_r5_madrid_main.m3u8", "https://graph.facebook.com/rne/picture?width=200&height=200" },
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
    static const Brn kSuffixImage(".Image");
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

        GetPresetKey(i, kSuffixImage, key);
        iImageUris.push_back(std::make_unique<Configuration::ConfigText>(
            aConfigInit, key, 0 /*aMinLength*/, kMaxImageUriBytes,
            Brn(station.iImageUri), false /*aRebootRequired*/, Configuration::ConfigValAccess::Public));
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
            iImageUriValues.push_back(std::make_unique<PresetValue>(*iImageUris[i], *this));
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
        const Brx& imageUri = iImageUriValues[i]->Value();
        iPresetWriter->SetPreset(i, uri, name, imageUri.Bytes() > 0 ? imageUri : Brx::Empty());
    }
}

} // namespace Av
} // namespace OpenHome
