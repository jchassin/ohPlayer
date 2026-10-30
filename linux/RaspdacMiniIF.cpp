#include "RaspdacMiniIF.h"

#include <OpenHome/Media/PipelineObserver.h>
#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Media/Pipeline/Msg.h>
#include <OpenHome/Private/Printer.h>
#include <OpenHome/OhMetadata.h>
#include <OpenHome/Net/Private/XmlParser.h>

#include <map>

using namespace OpenHome;
using namespace OpenHome::Media;
using namespace OpenHome::Av;
using namespace OpenHome::Net;


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

void RaspdacObserver::NotifyMetaText(const Brx& /* aText */)
{
}
void RaspdacObserver::NotifyTime(TUint aSeconds)
{
        iOhMdpIF->setTime(aSeconds);
	
        //Log::Print("Playing %02.02u:%02.02u/%02.02u:%02.02u\r",
	//	 aSeconds/60, aSeconds%60, aTrackDurationSeconds/60, aTrackDurationSeconds%60);
}

void RaspdacObserver::NotifyTrack(Track& aTrack, TBool /*aStartOfStream*/)
{
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


