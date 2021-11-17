// TCP Server text interface for OLED screen
#include <OpenHome/Net/Private/Globals.h>
#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include "OhMdpIF.h"
#include <OpenHome/Private/Network.h>
#include <OpenHome/Net/Private/DviService.h>
#include <OpenHome/Private/Stream.h>
#include <OpenHome/Private/Converter.h>
#include <OpenHome/Private/Parser.h>
#include <OpenHome/Private/Ascii.h>
#include <OpenHome/Private/Printer.h>
#include <OpenHome/Private/Standard.h>
#include <OpenHome/Private/Thread.h>
#include <OpenHome/Private/Stream.h>

#include <limits.h>
using namespace OpenHome;

//const Brn OhMdpIF::kMsgTerminator("\r\n");
//const Brn OhMdpIF::kArgumentDelimiter(" ");
class OhMdpIFSession : public SocketTcpSession
{
public:
    OhMdpIF * iParentOhMdpIF;
 
    OhMdpIFSession(OhMdpIF* OhMdpIF)
    {
        iParentOhMdpIF = OhMdpIF;
        iStream = new Srs<200>(*this);
        iReaderUntil = new ReaderUntilS<200>(*iStream);
        this->iWriteBuffer = new Sws<kWriteBufferBytes>(*this);
    }

    ~OhMdpIFSession()
    {
        delete iReaderUntil;
        delete iStream;
    }
    void Run()
    {
        const Brn exitUnix("exit");
        const Brn exitDos("exit\r"); // not that smart but for debug purpose
        const Brn statusCommand("status");
        const Brn statusCommandDos("status\r");

        const Brn currentSongCommand("currentsong");
        const Brn currentSongCommandDos("currentsong\r");

	const Brn titleHdr("Title: ");
        const Brn artistHdr("Artist: ");
        const Brn albumHdr("Album: ");
        const Brn albumArtURI("AlbumArtURI: ");


        const Brn timeHdr("time: ");
        const Brn elapsedHdr("elapsed: ");
        const Brn durationHdr("duration: ");


        const Brn volumeHdr("volume: ");
        const Brn audioHdr("audio: ");
        const Brn bitRateHdr("bitrate: ");

	const Brn EndOfLine("\n");
        const TUint kBuffSize(200);
        char aBuff[kBuffSize];

        

        TBool done = false;
        iWriteBuffer->Write(Brn("OK"));
        iWriteBuffer->Write(EndOfLine);
        iWriteBuffer->WriteFlush();

        while (!done) {
            try {
                Brn buf = iReaderUntil->ReadUntil('\n');
                //Log::Print("buf received : "); Log::Print(buf);
		if (buf.Equals(statusCommand)||buf.Equals(statusCommandDos))
                {
                  //Log::Print("status received");
                  sprintf(aBuff, "%u", iParentOhMdpIF->iVolume);
                  iWriteBuffer->Write(volumeHdr);iWriteBuffer->Write(Brn(aBuff));iWriteBuffer->Write(EndOfLine);
                   
                  iWriteBuffer->Write(Brn("state: "));iWriteBuffer->Write(iParentOhMdpIF->iMode);iWriteBuffer->Write(EndOfLine);
                  sprintf(aBuff, "%u:%u:%u", iParentOhMdpIF->iSampleFreq , iParentOhMdpIF->iBitDepth , iParentOhMdpIF->iChannelNum);
                  iWriteBuffer->Write(audioHdr);iWriteBuffer->Write(Brn(aBuff));iWriteBuffer->Write(EndOfLine);
                 
                  sprintf(aBuff, "%u", iParentOhMdpIF->iBitRate/1000);
                  iWriteBuffer->Write(bitRateHdr);iWriteBuffer->Write(Brn(aBuff));iWriteBuffer->Write(EndOfLine);

                  sprintf(aBuff, "%.04f", double(iParentOhMdpIF->iElapsed));
                  iWriteBuffer->Write(elapsedHdr);iWriteBuffer->Write(Brn(aBuff));iWriteBuffer->Write(EndOfLine);
                  sprintf(aBuff, "%.04f", double(iParentOhMdpIF->iDuration));
                  iWriteBuffer->Write(durationHdr);iWriteBuffer->Write(Brn(aBuff));iWriteBuffer->Write(EndOfLine);

                }
		else if (buf.Equals(currentSongCommand)||buf.Equals(currentSongCommandDos))
                {
                  //Log::Print("current received");

                  iWriteBuffer->Write(titleHdr);
                  iWriteBuffer->Write(iParentOhMdpIF->iTitle);
                  iWriteBuffer->Write(EndOfLine);
                  iWriteBuffer->Write(artistHdr);iWriteBuffer->Write(iParentOhMdpIF->iArtist);iWriteBuffer->Write(EndOfLine);
                  iWriteBuffer->Write(albumHdr);iWriteBuffer->Write(iParentOhMdpIF->iAlbum);iWriteBuffer->Write(EndOfLine);
                  iWriteBuffer->Write(albumArtURI);iWriteBuffer->Write(iParentOhMdpIF->iAlbumArtURI);iWriteBuffer->Write(EndOfLine);

                  sprintf(aBuff, "%u", iParentOhMdpIF->iElapsed);

                  iWriteBuffer->Write(timeHdr);iWriteBuffer->Write (Brn(aBuff)); iWriteBuffer->Write(Brn(":")); 

                  sprintf(aBuff, "%u", iParentOhMdpIF->iDuration);
                  iWriteBuffer->Write (Brn(aBuff)); iWriteBuffer->Write(EndOfLine); 
                }

                iWriteBuffer->Write(Brn("OK")); iWriteBuffer->Write(EndOfLine);
                iWriteBuffer->WriteFlush();
                if (buf.Equals(exitUnix) || buf.Equals(exitDos))
                    done = true;
            }
            catch (ReaderError&) {
                break;
            }
        }
    }
private:
    static const TUint kWriteBufferBytes = 4000;
    Srx* iStream;
    ReaderUntil* iReaderUntil;
    Sws<kWriteBufferBytes>* iWriteBuffer;

};
void OhMdpIF::setTime(TUint anElapsed)
{
  iElapsed = anElapsed;
  iDuration = 0;
}

void OhMdpIF::setMode(Brn aMode)
{
  iMode = aMode;
}

void OhMdpIF::setVolume(OpenHome::TUint aVolume)
{
   iVolume = aVolume;
}

void OhMdpIF::setStreamInfo(TUint aBitRate, TUint aChannelNum, TUint aBitDepth, TUint aSampleFreq)
{
    iBitRate    = aBitRate;
    iChannelNum = aChannelNum;
    iBitDepth   = aBitDepth;
    iSampleFreq = aSampleFreq;
}

void OhMdpIF::setTrack(Brn aTitle, Brn anArtist, Brn anAlbum, Brn anAlbumArtURI)
{
   iTitle = aTitle;
   iArtist = anArtist;
   iAlbum = anAlbum;
   iAlbumArtURI = anAlbumArtURI;
}


OhMdpIF * OhMdpIF::instance=nullptr;

OhMdpIF * OhMdpIF::getInstance()
{
  if (OhMdpIF::instance == nullptr)
  {
	OhMdpIF::instance = new OhMdpIF();
  }
  return (OhMdpIF::instance);
}

OhMdpIF::OhMdpIF()
{
//    Net::UpnpLibrary::Initialise(aInitParams);
//    Debug::SetLevel(Debug::kNetwork);
    std::vector<NetworkAdapter*>* ifs = Os::NetworkListAdapters(*gEnv, Environment::ELoopbackUse, false, "OhMdp");


    ASSERT(ifs->size() > 0);

    TIpAddress addr = (*ifs)[0]->Address();
    Endpoint endpt(0, addr);
    Endpoint::AddressBuf buf;
    endpt.AppendAddress(buf);
#if 0
    for (TUint i=0; i<ifs->size(); i++) {
        (*ifs)[i]->RemoveRef("TestEcho");
    }
#endif
    iDuration   = 0;
    iElapsed    = 0; 
    iBitRate    = 0;
    iChannelNum = 0;
    iBitDepth   = 0;
    iSampleFreq = 0;

    iMode = Brn("stop");
    Semaphore sem("", 0);
    SocketTcpServer* server = new SocketTcpServer(*gEnv, "OHMDP", 6600, addr);
    server->Add("OHMDP", new OhMdpIFSession(this));

//    sem.Wait();
    //delete ifs;

 //   Net::UpnpLibrary::Close();
}


