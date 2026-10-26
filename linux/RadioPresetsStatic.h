#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Av/Radio/Presets.h>

namespace OpenHome {
namespace Av {

// Static radio preset provider: fills the Radio presets with a fixed list of
// stations (no network access required to enumerate them). Selection between
// this provider and any other (e.g. TuneIn) is exposed in the web config ui
// as Radio.PresetProvider by ohPipeline's RadioPresets once a second provider
// is registered.
class RadioPresetsStatic : public IRadioPresetProvider
{
public:
    static const Brn kDisplayName;
public:
    RadioPresetsStatic();
    ~RadioPresetsStatic();
public: // from IRadioPresetProvider
    const Brx& DisplayName() const override;
    void Activate(IRadioPresetWriter& aWriter) override;
    void Deactivate() override;
    void RefreshPresets() override;
private:
    void SetAllPresets();
private:
    IRadioPresetWriter* iPresetWriter;
};

} // namespace Av
} // namespace OpenHome
