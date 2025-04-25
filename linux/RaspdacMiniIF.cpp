#if 0
#include "RaspdacMiniIF.h"
#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
using namespace OpenHome;
//using namespace OpenHome::Av;
using namespace OpenHome::Media;

#else
#include "RaspdacMiniIF.h"

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

RaspdacObserver::~RaspdacObserver()
{
    delete(iOhMdpIF);
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

#ifndef DEBUG_IF
void RaspdacObserver::NotifyMetaText(const Brx& /* aText */)
{}
#else
void RaspdacObserver::NotifyMetaText(const Brx& aText)
{
    Log::Print("MetaText : ");
    const TByte* src = aText.Ptr();
    for (TUint i=0; i<aText.Bytes(); i++) {
       Log::Print("%02.02X ", src[i]);
    }
    Log::Print("MetaText :\n");
}
#endif    
void RaspdacObserver::NotifyTime(TUint aSeconds)
{
        iOhMdpIF->setTime(aSeconds);
	
        //Log::Print("Playing %02.02u:%02.02u/%02.02u:%02.02u\r",
	//	 aSeconds/60, aSeconds%60, aTrackDurationSeconds/60, aTrackDurationSeconds%60);
}

void RaspdacObserver::NotifyTrack(Track& aTrack, TBool aStartOfStream)
{
#ifdef DEBUG_IF
    Log::Print("Pipeline report property: TRACK {uri=%.*s; trackId=%u; startOfStream=%u}\n",
    PBUF(aTrack.Uri()), aTrack.Id(), aStartOfStream);

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
Brn aAlbumArtURI;

try {
        aAlbumArtURI = XmlParserBasic::Find("albumArtURI", xmlBuffer);
}
catch (...) {aAlbumArtURI = Brn("");}

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
        
iOhMdpIF->setTrack(aTitle, aArtistName, aAlbumName, aAlbumArtURI);
Log::Print("Artist : %.*s Album : %.*s Track : %.*s\n URI : %.*s\n", PBUF(aArtistName), PBUF(aAlbumName), PBUF(aTitle), PBUF(aAlbumArtURI));


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


// RaspdacMiniIF
#if 0
RaspdacMiniIF::RaspdacMiniIF(PipelineManager &pPipeline)
{
//   iRaspdacObserver = new RaspdacObserver();
 //  iPipelineManager = pPipeline;

  //  iPipelineManager.AddObserver(*iRaspdacObserver);
}
RaspdacMiniIF::~RaspdacMiniIF()
{
    delete (iOhMdpIF);
}
int RaspdacMiniIF::test(int a)
{
  return a;
}
#endif
