#if 0
#include "RaspdacMiniOledIF.h"
#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
using namespace OpenHome;
//using namespace OpenHome::Av;
using namespace OpenHome::Media;

#else
#include "RaspdacMiniOledIF.h"

#include <OpenHome/Media/PipelineObserver.h>
#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Media/Pipeline/Msg.h>
#include <OpenHome/Private/Printer.h>
#include <OpenHome/Av/OhMetadata.h>
#include <OpenHome/Net/Private/XmlParser.h>

#include <map>

using namespace OpenHome;
using namespace OpenHome::Media;
using namespace OpenHome::Av;
using namespace OpenHome::Net;


#endif

// RaspdacObserver
RaspdacObserver::RaspdacObserver()
{
	iOhMdpIF = OhMdpIF::getInstance();
}
void RaspdacObserver::NotifyPipelineState(EPipelineState aState)
{
    const TChar* state;
    switch (aState)
    {
    case EPipelinePlaying:
        state = "play";
        break;
    case EPipelinePaused:
        state = "pause";
        break;
    case EPipelineStopped:
    case EPipelineBuffering:
    case EPipelineWaiting:
        state = "stop";
        break;
    default:
        ASSERTS();
        state = "";
        break;
    }

   iOhMdpIF->setMode(Brn(state));
}

void RaspdacObserver::NotifyMode(const Brx& /*aMode*/,
                                      const ModeInfo& /*aInfo*/,
                                      const ModeTransportControls& /*aTransportControls*/)
{
}

void RaspdacObserver::NotifyMetaText(const Brx& aText)
{
    Log::Print("MetaText : ");
    const TByte* src = aText.Ptr();
    for (TUint i=0; i<aText.Bytes(); i++) {
       Log::Print("%02.02X ", src[i]);
    }
    Log::Print("MetaText :\n");
    
}
void RaspdacObserver::NotifyTime(TUint aSeconds,  TUint aTrackDurationSeconds)
{
        iOhMdpIF->setTime(aSeconds, aTrackDurationSeconds);
	
        //Log::Print("Playing %02.02u:%02.02u/%02.02u:%02.02u\r",
	//	 aSeconds/60, aSeconds%60, aTrackDurationSeconds/60, aTrackDurationSeconds%60);
}

void RaspdacObserver::NotifyTrack(Track& aTrack, const Brx& aMode, TBool aStartOfStream)
{
#if 0
        Log::Print("Track MetaData : %.*s\n", PBUF(aTrack.MetaData()));
	
	const TByte* src = aTrack.MetaData();
        for (TUint i=0; i<PBUF(aTrack.MetaData())->Bytes(); i++) {
            Log::Print("%02.02X ", src[i]);
        }
        Log::Print("MetaText :\n");
#endif

Brn xmlBuffer(aTrack.MetaData()) ;
Brn aTitle;
Brn aAlbumName;
Brn aArtistName;

try {
        aTitle = XmlParserBasic::Find("title", xmlBuffer);
}
catch (...) {aTitle = Brn("");}
try {
        aAlbumName = XmlParserBasic::Find("album", xmlBuffer);
}
catch (...) {aAlbumName = Brn("");}
try {
        aArtistName = XmlParserBasic::Find("artist",xmlBuffer );
} catch (...) {aArtistName = Brn("");}
        
iOhMdpIF->setTrack(aTitle, aArtistName, aAlbumName);
Log::Print("Artist : %.*s Album : %.*s Track : %.*s\n", PBUF(aArtistName), PBUF(aAlbumName), PBUF(aTitle));


}
void RaspdacObserver::NotifyStreamInfo(const DecodedStreamInfo& aStreamInfo)
{
    //iDurationSeconds = (TUint)(aStreamInfo.TrackLength() / Jiffies::kPerSecond);
    Log::Print("Pipeline report property: FORMAT {bitRate=%u; bitDepth=%u, sampleRate=%u, numChannels=%u, codec=%.*s; trackLength=%llx, lossless=%u, channelConfig=%s}\n",
               aStreamInfo.BitRate(), aStreamInfo.BitDepth(), aStreamInfo.SampleRate(), aStreamInfo.NumChannels(),
               PBUF(aStreamInfo.CodecName()), aStreamInfo.TrackLength(), aStreamInfo.Lossless(), aStreamInfo.Profile().ToString());
    iOhMdpIF-> setStreamInfo(aStreamInfo.BitRate(), aStreamInfo.NumChannels(), aStreamInfo.BitDepth(), aStreamInfo.SampleRate());

}

//RaspdacVolumeObserver::RaspdacVolumeObserver(IVolumeManager& aVolumeManager)
RaspdacVolumeObserver::RaspdacVolumeObserver()
{
    this->iOhMdpIF = OhMdpIF::getInstance();
}

void RaspdacVolumeObserver::VolumeChanged(const IVolumeValue& aVolume)
{
   iOhMdpIF->setVolume(aVolume.VolumeUser());
}


// RaspdacMiniOledIF
#if 0
RaspdacMiniOledIF::RaspdacMiniOledIF(PipelineManager &pPipeline)
{
//   iRaspdacObserver = new RaspdacObserver();
 //  iPipelineManager = pPipeline;

  //  iPipelineManager.AddObserver(*iRaspdacObserver);
}
RaspdacMiniOledIF::~RaspdacMiniOledIF()
{
}
int RaspdacMiniOledIF::test(int a)
{
  return a;
}
#endif
