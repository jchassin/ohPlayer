#include "OhLog.h"
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
 
OhMdpIFSession::OhMdpIFSession(OhMdpIF* OhMdpIF)
    {
        iParentOhMdpIF = OhMdpIF;
        iStream = new Srs<200>(*this);
        iReaderUntil = new ReaderUntilS<200>(*iStream);
        this->iWriteBuffer = new Sws<kWriteBufferBytes>(*this);
    }

OhMdpIFSession::~OhMdpIFSession()
    {
        delete iReaderUntil;
        delete iStream;
        delete iWriteBuffer;
    }
void  OhMdpIFSession::Run()
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

        

        // This session object is reused by the server accept loop for every
        // connection: recreate the streams per connection, otherwise bytes
        // left over from a previous client corrupt ReadUntil('\n') and
        // commands are no longer recognised.
        delete iReaderUntil;
        delete iStream;
        delete iWriteBuffer;
        iStream = new Srs<200>(*this);
        iReaderUntil = new ReaderUntilS<200>(*iStream);
        iWriteBuffer = new Sws<kWriteBufferBytes>(*this);

        TBool done = false;
        try {
            iWriteBuffer->Write(Brn("OK"));
            iWriteBuffer->Write(EndOfLine);
            iWriteBuffer->WriteFlush();
        }
        catch (WriterError&) {
            return; // client already gone
        }

        while (!done) {
            try {
                Brn buf = iReaderUntil->ReadUntil('\n');
		if (buf.Equals(statusCommand)||buf.Equals(statusCommandDos))
                {
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
                break; // client closed the connection
            }
            catch (WriterError&) {
                break; // client disconnected mid-response: writes to the socket throw
            }
        }
    }

void OhMdpIF::setTime(TUint anElapsed)
{
  iElapsed = anElapsed;
}

void OhMdpIF::setDuration(TUint aDuration)
{
  iDuration = aDuration;
}

// Owning copy of a Brx into a Bwh member: Bwh(const Brx&) allocates the exact
// size, Swap exchanges buffers (O(1), never overflows - unlike Replace() on a
// default Bwh whose MaxBytes is 0, which would throw BufferOverflow/ASSERTS).
static void CopyTo(Bwh& aDest, const Brx& aSrc)
{
    Bwh tmp(aSrc);
    aDest.Swap(tmp);
}

void OhMdpIF::setMode(Brn aMode)
{
  CopyTo(iMode, aMode);
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
   CopyTo(iTitle, aTitle);
   CopyTo(iArtist, anArtist);
   CopyTo(iAlbum, anAlbum);
   CopyTo(iAlbumArtURI, anAlbumArtURI);
   iDuration = 0;
   OhLog::Print("OhMdpIF: setTrack title='%.*s' artist='%.*s' album='%.*s' art='%.*s'\n",
               PBUF(aTitle), PBUF(anArtist), PBUF(anAlbum), PBUF(anAlbumArtURI));
}

void OhMdpIF::setTitle(Brn aTitle)
{
   CopyTo(iTitle, aTitle);
   OhLog::Print("OhMdpIF: setTitle '%.*s'\n", PBUF(aTitle));
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
OhMdpIF::~OhMdpIF()
{
}

OhMdpIF::OhMdpIF()
{
    //Net::UpnpLibrary::Initialise(aInitParams);
    //Debug::SetLevel(Debug::kNetwork);
    std::vector<NetworkAdapter*>* ifs = Os::NetworkListAdapters(*gEnv, Environment::ELoopbackUse, false, "OhMdp");


    ASSERT(ifs->size() > 0);

    // The OLED TCP server is loopback-only by design (the display talks to the
    // device locally): pick the loopback adapter explicitly instead of a
    // hardcoded index, so the bind address does not depend on the adapter
    // enumeration order.
    TIpAddress addr = (*ifs)[0]->Address();
    for (TUint i = 0; i < ifs->size(); i++) {
        const TIpAddress& candidate = (*ifs)[i]->Address();
        if (TIpAddressUtils::IsLoopback(candidate)) {
            addr = candidate;
            break;
        }
    }
    Endpoint endpt(0, addr);
    Endpoint::AddressBuf buf;
    endpt.AppendAddress(buf);

    Endpoint::AddressBuf myBuf;
    TIpAddressUtils::ToString(addr, myBuf);
    OhLog::Print("Using network interface %.*s\n\n", PBUF(myBuf));

#if 1
    for (TUint i=0; i<ifs->size(); i++) {
        (*ifs)[i]->RemoveRef("OhMdp");
    }
#endif
    //delete ifs;

    iDuration   = 0;
    iElapsed    = 0;
    iBitRate    = 0;
    iChannelNum = 0;
    iBitDepth   = 0;
    iSampleFreq = 0;
    iVolume     = 0;
    // Bwh members default to empty (0 bytes) - safe to read before the first
    // setter. NOTE: do NOT use Replace() on a default Bwh: MaxBytes is 0, so any
    // non-empty value would throw BufferOverflow (ASSERTS in debug builds).
    CopyTo(iMode, Brn("stop"));
    Semaphore sem("", 0);

    iServer = new SocketTcpServer(*gEnv, "OHMDP", 6600, addr);
    iSession = new OhMdpIFSession(this);
    iServer->Add("OHMDP", iSession);

    //sem.Wait();
    //delete ifs;

 //   Net::UpnpLibrary::Close();
}


