#include "ConfigAppPlayerLinn.h"
#include "LinnAmpConfig.h"
#include <OpenHome/Web/ConfigUi/ConfigUi.h>

namespace OpenHome {
namespace Av {

static const Brn kKeyLinnAmpModel("LinnAmp.Model");
static const Brn kKeyLinnAmpSerial("LinnAmp.Serial");

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
                                         const Brx& aLinnAmpModel, const Brx& aLinnAmpSerial)
    : Web::ConfigAppMediaPlayer(aInfoAggregator, aEnv, aProduct, aConfigManager, aResourceFactory,
                                aSources, aResourcePrefix, aResourceDir,
                                aResourceHandlerCount, aMaxTabs, aSendQueueSize,
                                aMsgBufCount, aMsgBufBytes, aRebootHandler)
{
    // Read-only info fields for the detected amp
    AddValue(new Web::ConfigUiValRo(kKeyLinnAmpModel, Brn(aLinnAmpModel)));
    AddValue(new Web::ConfigUiValRo(kKeyLinnAmpSerial, Brn(aLinnAmpSerial)));
    // Editable Linn Amp settings
    AddConfigChoice(LinnAmpConfig::kKeyVolumeControl);
    AddConfigText(LinnAmpConfig::kKeySerialPort);
    AddConfigNum(LinnAmpConfig::kKeyBass);
    AddConfigNum(LinnAmpConfig::kKeyTreble);
}

} // namespace Av
} // namespace OpenHome
