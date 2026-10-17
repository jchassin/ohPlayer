#pragma once

#include <OpenHome/Av/MediaPlayer.h>
#include <OpenHome/Web/ConfigUi/ConfigUiMediaPlayer.h>

namespace OpenHome {
namespace Av {

// Extends the standard ohPipeline config web app with Linn Amp settings
// and read-only info fields (detected amp model and serial).
class ConfigAppPlayerLinn : public Web::ConfigAppMediaPlayer
{
public:
    ConfigAppPlayerLinn(IInfoAggregator& aInfoAggregator,
                        Environment& aEnv,
                        Product& aProduct,
                        Configuration::IConfigManager& aConfigManager,
                        Web::IConfigAppResourceHandlerFactory& aResourceFactory,
                        const std::vector<const Brx*>& aSources,
                        const Brx& aResourcePrefix, const Brx& aResourceDir,
                        TUint aResourceHandlerCount, TUint aMaxTabs, TUint aSendQueueSize,
                        TUint aMsgBufCount, TUint aMsgBufBytes,
                        IRebootHandler& aRebootHandler,
                        const Brx& aLinnAmpModel, const Brx& aLinnAmpSerial);
};

} // namespace Av
} // namespace OpenHome
