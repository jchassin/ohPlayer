#pragma once


#if 0
#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>

#include <OpenHome/Media/Pipeline/Pipeline.h>
#include <OpenHome/Media/PipelineManager.h>
#include <OpenHome/Media/Pipeline/Msg.h>
#else
#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Media/Pipeline/Msg.h>
#include <OpenHome/Media/Pipeline/Pipeline.h>
#include <OpenHome/Media/PipelineManager.h>

#include "OhMdpIF.h"

#endif


namespace OpenHome {
namespace Av {
class RaspdacVolumeObserver : public IVolumeObserver
{
   public :
    //RaspdacVolumeObserver(OpenHome::Av::IVolumeManager& VolumeManager); 
    RaspdacVolumeObserver();


   private : 
    //OpenHome::Av::IVolumeManager& VolumeManager;
    OhMdpIF * iOhMdpIF;
    void VolumeChanged(const OpenHome::Av::IVolumeValue& aVolume);

};
}
namespace Media {
class RaspdacObserver : public IPipelineObserver
{
   public:
    RaspdacObserver();
    ~RaspdacObserver();
   //void NotifyTrack(OpenHome::Media::Track& aTrack, OpenHome::TBool aStartOfStream) override ;

   private: // from IPipelineObserver
    OhMdpIF * iOhMdpIF;
    void NotifyPipelineState(EPipelineState aState) override;
    void NotifyMode(const Brx& aMode, const ModeInfo& aInfo,
                    const ModeTransportControls& aTransportControls) override;
    void NotifyTrack(Track& aTrack, TBool aStartOfStream) override;
    void NotifyMetaText(const Brx& aText) override;
    void NotifyTime(TUint aSeconds) override;

    void NotifyStreamInfo(const DecodedStreamInfo& aStreamInfo) override;

   private : // from IVolumeObserver
    void TestXml();
};





#if 1
class RaspdacMiniIF 
{
   public:
   RaspdacMiniIF(PipelineManager &pPipelineManager);
   ~RaspdacMiniIF();
   public : 
      int test(int a);

private :
    RaspdacObserver *iRaspdacObserver;
    PipelineManager& iPipelineManager;

};
#endif
 } // namespace Media
} // namespace OpenHome
