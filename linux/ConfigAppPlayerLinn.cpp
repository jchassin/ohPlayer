#include "ConfigAppPlayerLinn.h"
#include "LinnAmpConfig.h"

namespace OpenHome {
namespace Av {

ConfigAppPlayerLinn::ConfigAppPlayerLinn(IInfoAggregator& aInfoAggregator,
                                         Environment& aEnv,
                                         Product& aProduct,
                                         Configuration::IConfigManager& aConfigManager,
                                         Web::IConfigAppResourceHandlerFactory& aResourceFactory,
                                         const std::vector<const Brx*>& aSources,
                                         const Brx& aResourcePrefix, const Brx& aResourceDir,
                                         TUint aResourceHandlerCount, TUint aMaxTabs, TUint aSendQueueSize,
                                         TUint aMsgBufCount, TUint aMsgBufBytes,
                                         IRebootHandler& aRebootHandler)
    : Web::ConfigAppMediaPlayer(aInfoAggregator, aEnv, aProduct, aConfigManager, aResourceFactory,
                                aSources, aResourcePrefix, aResourceDir,
                                aResourceHandlerCount, aMaxTabs, aSendQueueSize,
                                aMsgBufCount, aMsgBufBytes, aRebootHandler)
{
    // Expose the Linn Amp settings in the web config UI
    AddConfigChoice(LinnAmpConfig::kKeyVolumeControl);
    AddConfigText(LinnAmpConfig::kKeySerialPort);
}

} // namespace Av
} // namespace OpenHome
