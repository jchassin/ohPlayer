#include "RadioPresetsStatic.h"
#include <OpenHome/Private/Printer.h>

using namespace OpenHome;

namespace OpenHome {
namespace Av {

const Brn RadioPresetsStatic::kDisplayName("Stations");

namespace {

// Static station list exposed as radio presets, in preset order.
struct Station {
    const char* iName;
    const char* iUri;
};

static const Station kStations[] = {
    { "France Inter",     "https://icecast.radiofrance.fr/franceinter-hifi.aac" },
    { "FIP",              "https://icecast.radiofrance.fr/fip-hifi.aac" },
    { "France Musique",   "https://icecast.radiofrance.fr/francemusique-hifi.aac" },
    { "France Info",      "https://icecast.radiofrance.fr/franceinfo-hifi.aac" },
    { "France Culture",   "https://icecast.radiofrance.fr/franceculture-hifi.aac" },
    { "Mouv'",            "https://icecast.radiofrance.fr/mouv-hifi.aac" },
};

} // namespace

RadioPresetsStatic::RadioPresetsStatic()
    : iPresetWriter(nullptr)
{
}

RadioPresetsStatic::~RadioPresetsStatic()
{
}

const Brx& RadioPresetsStatic::DisplayName() const
{
    return kDisplayName;
}

void RadioPresetsStatic::Activate(IRadioPresetWriter& aWriter)
{
    iPresetWriter = &aWriter;
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
    SetAllPresets();
}

void RadioPresetsStatic::SetAllPresets()
{
    // Preset indices are 0-based within the writer.
    TUint index = 0;
    for (const Station& station : kStations) {
        iPresetWriter->SetPreset(index, Brn(station.iUri), Brn(station.iName), Brx::Empty());
        index++;
    }
}

} // namespace Av
} // namespace OpenHome
