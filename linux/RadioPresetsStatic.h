#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Configuration/ConfigManager.h>
#include <OpenHome/Av/Radio/Presets.h>
#include <vector>
#include <memory>

namespace OpenHome {
namespace Av {

// Radio preset provider filling the Radio source presets from web-configured
// stations. Each of the kNumPresets presets has two config values exposed in
// the web config ui (Radio tab):
//   Radio.Preset<N>.Name  - display title (empty = preset unset)
//   Radio.Preset<N>.Uri   - stream url (empty = preset unset)
// Defaults are the Radio France hifi AAC streams. Subscribing to each ConfigVal
// keeps an in-memory copy of the current value; any change schedules a preset
// refresh so edits in the web ui are applied without waiting for the periodic
// refresh timer.
class RadioPresetsStatic : public IRadioPresetProvider
{
public:
    static const TUint kNumPresets = 6;
    static const TUint kMaxNameBytes = 64;
    static const TUint kMaxUriBytes = 256;
    static const TUint kKeyBytes = 32;
    static void GetPresetKey(TUint aIndex, const Brx& aSuffix, Bwx& aKey);
    RadioPresetsStatic(Configuration::IConfigInitialiser& aConfigInit);
    ~RadioPresetsStatic();
public: // from IRadioPresetProvider
    const Brx& DisplayName() const override;
    void Activate(IRadioPresetWriter& aWriter) override;
    void Deactivate() override;
    void RefreshPresets() override;
private:
    class PresetValue
    {
    public:
        PresetValue(Configuration::ConfigText& aConfig, RadioPresetsStatic& aOwner);
        ~PresetValue();
        const Brx& Value() const;
        void ValueChanged(Configuration::KeyValuePair<const Brx&>& aKvp);
    private:
        Configuration::ConfigText& iConfig;
        RadioPresetsStatic& iOwner;
        TUint iSubscriptionId;
        Bwh iValue;
    };
private:
    static const Brn kDisplayName;
    std::vector<std::unique_ptr<Configuration::ConfigText>> iNames;
    std::vector<std::unique_ptr<Configuration::ConfigText>> iUris;
    std::vector<std::unique_ptr<PresetValue>> iNameValues;
    std::vector<std::unique_ptr<PresetValue>> iUriValues;
    IRadioPresetWriter* iPresetWriter;
};

} // namespace Av
} // namespace OpenHome
