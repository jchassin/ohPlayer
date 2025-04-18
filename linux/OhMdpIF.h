
// TCP Server text interface for OLED screen


#ifndef HEADER_OHMDP
#define HEADER_OHMDP



#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Exception.h>
#include <OpenHome/Private/Network.h>
#include <OpenHome/Net/Private/DviService.h>
#include <OpenHome/Net/Private/DviServer.h>
#include <OpenHome/Private/Stream.h>
#include <OpenHome/Private/Parser.h>
#include <OpenHome/Private/Standard.h>
#include <OpenHome/Private/Network.h>
#include <OpenHome/OsWrapper.h>
#include <OpenHome/Net/Private/Globals.h>
#include <OpenHome/Private/Converter.h>
#include <OpenHome/Private/Ascii.h>
#include <OpenHome/Private/Printer.h>
#include <OpenHome/Private/Thread.h>

#include <vector>
#include <map>

namespace OpenHome {
    class OhMdpIF; //singleton;

    class OhMdpIFSession : public SocketTcpSession
    {
    public:
        OhMdpIF * iParentOhMdpIF;
     
        OhMdpIFSession(OhMdpIF* OhMdpIF);
    
        ~OhMdpIFSession();
        void Run();
    
        private:
        static const TUint kWriteBufferBytes = 4000;
        Srx* iStream;
        ReaderUntil* iReaderUntil;
        Sws<kWriteBufferBytes>* iWriteBuffer;
    };
class OhMdpIF //singleton
{
  public:
     static OhMdpIF * getInstance();
     ~OhMdpIF();

     void setTrack(OpenHome::Brn aTitle, OpenHome::Brn anArtist, OpenHome::Brn anAlbum, OpenHome::Brn anAlbumArtURI);
     void setMode(OpenHome::Brn aMode);
     void setTime(OpenHome::TUint anElapsed);
     void setStreamInfo(OpenHome::TUint aBitRate, OpenHome::TUint aChannelNum, OpenHome::TUint aBitDepth, OpenHome::TUint aSampleFreq);
     void setVolume(OpenHome::TUint aVolume);

     OpenHome::Brn iTitle;
     OpenHome::Brn iArtist;
     OpenHome::Brn iAlbum;
     OpenHome::Brn iAlbumArtURI;
     OpenHome::Brn iMode;
     OpenHome::TUint iElapsed;
     OpenHome::TUint iDuration;
     OpenHome::TUint iBitRate;
     OpenHome::TUint iChannelNum;
     OpenHome::TUint iBitDepth;
     OpenHome::TUint iSampleFreq;
     OpenHome::TUint iVolume;
     SocketTcpServer* iServer;
     OpenHome::OhMdpIFSession* iSession;
 
 private : 
     OhMdpIF();
    static OhMdpIF * instance;

};

}
#endif //HEADER_OHMDP
