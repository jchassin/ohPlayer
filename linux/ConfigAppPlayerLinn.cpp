#include "ConfigAppPlayerLinn.h"
#include "LinnAmpConfig.h"
#include "PipelineConfig.h"
#include "VersionInfo.h"
#include <OpenHome/Web/ConfigUi/ConfigUi.h>

namespace OpenHome {
namespace Av {

static const Brn kKeyLinnAmpModel("LinnAmp.Model");
static const Brn kKeyLinnAmpSerial("LinnAmp.Serial");
static const Brn kKeySwVersionOhNet("About.OhNetVersion");
static const Brn kKeySwVersionOhPipeline("About.OhPipelineVersion");
static const Brn kKeySwVersionOhPlayer("About.OhPlayerCommit");

ConfigAppPlayerLinn::ConfigAppPlayerLinn(IInfoAggregator& aInfoAggregator,
                                         Environment& aEnv,
                                         Product& aProduct,
                                         Configuration::IConfigManager& aConfigManager,
                                         Web::IConfigAppResourceHandlerFactory& aResourceFactory,
                                         const std::vector<const Brx*>& aSources,
                                         const Brx& aResourcePrefix, const Brx& aResourceDir,
                                         TUint aResourceHandlerCount, TUint aMaxTabs, TUint aSendQueueSize,
                                         TUint aMsgBufCount, TUint aMsgBufBytes,
                                         IRebootHandler& aRebootHandler,
                                         const Brx& aLinnAmpModel, const Brx& aLinnAmpSerial,
                                         TBool aLinnAmpActive)
    : Web::ConfigAppMediaPlayer(aInfoAggregator, aEnv, aProduct, aConfigManager, aResourceFactory,
                                aSources, aResourcePrefix, aResourceDir,
                                aResourceHandlerCount, aMaxTabs, aSendQueueSize,
                                aMsgBufCount, aMsgBufBytes, aRebootHandler)
{
    // Read-only info fields for the detected amp
    AddValue(new Web::ConfigUiValRo(kKeyLinnAmpModel, Brn(aLinnAmpModel)));
    AddValue(new Web::ConfigUiValRo(kKeyLinnAmpSerial, Brn(aLinnAmpSerial)));
    // Read-only software version fields (generated at compile time by the Makefile)
    AddValue(new Web::ConfigUiValRo(kKeySwVersionOhNet, Brn(VersionInfo::kOhNet)));
    AddValue(new Web::ConfigUiValRo(kKeySwVersionOhPipeline, Brn(VersionInfo::kOhPipeline)));
    AddValue(new Web::ConfigUiValRo(kKeySwVersionOhPlayer, Brn(VersionInfo::kOhPlayerCommit)));
    // Editable Linn Amp settings
    AddConfigText(LinnAmpConfig::kKeySerialPort);
    AddConfigNum(LinnAmpConfig::kKeyBass);
    AddConfigNum(LinnAmpConfig::kKeyTreble);
    AddConfigNum(LinnAmpConfig::kKeyAlsaVolume);
    // Editable pipeline buffer settings (Songcast tab)
    AddConfigNum(PipelineConfig::kKeyStarvationRamperMinSize);
    AddConfigNum(PipelineConfig::kKeyEncodedReservoirSize);
    // Expose ohPipeline's internal volume control toggle (hidden by default).
    // Allows disabling the volume control point-side on setups where the
    // sound card has no mixer and no external amplifier controls volume.
    // When an external Linn amplifier handles volume, the internal volume is
    // inactive by definition, so the field is read-only and reports No
    // (the MediaPlayer's ConfigVal holds the runtime-enabled value used to
    // create the Volume provider; the user's setting in the store is No).
    if (aLinnAmpActive) {
        AddValue(new Web::ConfigUiValRo(Brn("Volume.Enabled"), Brn("No")));
    }
    else {
        AddConfigChoiceConditional(Brn("Volume.Enabled"));
    }
}

} // namespace Av
} // namespace OpenHome
