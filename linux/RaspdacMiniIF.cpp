#include "OhLog.h"
#include "RaspdacMiniIF.h"
#include "RadioFranceNowPlaying.h"

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
RaspdacObserver::RaspdacObserver(Environment& aEnv, SslContext& aSsl, const Brx& aUserAgent)
{
	iOhMdpIF = OhMdpIF::getInstance();
	iNowPlaying = new Av::RadioFranceNowPlaying(aEnv, aSsl, aUserAgent, *iOhMdpIF);
}

RaspdacObserver::~RaspdacObserver()
{
    delete iNowPlaying;
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
        iNowPlaying->SetStreamUri(Brx::Empty());
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
    // ICY StreamTitle updates arrive here as DIDL-Lite (<dc:title>...).
    // Log the raw payload and the extracted title so we can see whether the
    // radio stream actually sends show metadata and what it contains.
    OhLog::Print("Raspdac: METATEXT (%u bytes): %.*s\n", aText.Bytes(), PBUF(aText));
    Brn title;
    if (XmlParserBasic::TryFind(Brn("title"), aText, title)) {
        OhLog::Print("Raspdac: METATEXT title: %.*s\n", PBUF(title));
        if (title.Bytes() > 0) {
            // ICY StreamTitle (show/song) -> OLED title. Keep artist/album/art
            // and duration untouched: only setTrack() resets them.
            iOhMdpIF->setTitle(title);
        }
    }
    else {
        OhLog::Print("Raspdac: METATEXT without <title> element\n");
    }
}
void RaspdacObserver::NotifyTime(TUint aSeconds)
{
        iOhMdpIF->setTime(aSeconds);
	
        //OhLog::Print("Playing %02.02u:%02.02u/%02.02u:%02.02u\r",
	//	 aSeconds/60, aSeconds%60, aTrackDurationSeconds/60, aTrackDurationSeconds%60);
}

void RaspdacObserver::NotifyTrack(Track& aTrack, TBool aStartOfStream)
{
Brn xmlBuffer(aTrack.MetaData()) ;
OhLog::PrintDebug("Raspdac: TRACK startOfStream=%u uri=%.*s metadata(%u bytes): %.*s\n",
                  (TUint)aStartOfStream, PBUF(aTrack.Uri()), xmlBuffer.Bytes(), PBUF(xmlBuffer));
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
        iNowPlaying->SetStreamUri(aTrack.Uri());
OhLog::Print("Raspdac: Artist: %.*s  Album: %.*s  Track: %.*s\n URI: %.*s\n", PBUF(aArtistName), PBUF(aAlbumName), PBUF(aTitle), PBUF(aAlbumArtURI));


}
void RaspdacObserver::NotifyStreamInfo(const DecodedStreamInfo& aStreamInfo)
{
    iOhMdpIF->setDuration((TUint)(aStreamInfo.TrackLength() / Jiffies::kPerSecond));
    OhLog::Print("Raspdac: FORMAT {bitRate=%u; bitDepth=%u, sampleRate=%u, numChannels=%u, codec=%.*s; trackLength=%llx, lossless=%u, live=%u, seekable=%u, channelConfig=%s}\n",
               aStreamInfo.BitRate(), aStreamInfo.BitDepth(), aStreamInfo.SampleRate(), aStreamInfo.NumChannels(),
               PBUF(aStreamInfo.CodecName()), aStreamInfo.TrackLength(), aStreamInfo.Lossless(),
               (TUint)aStreamInfo.Live(), (TUint)aStreamInfo.Seekable(), aStreamInfo.Profile().ToString());
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


