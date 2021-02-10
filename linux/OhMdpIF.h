
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
#include <OpenHome/OsWrapper.h>

#include <vector>
#include <map>

//namespace OpenHome {

class OhMdpIF //singleton
{
  public:
     static OhMdpIF * getInstance();

     void setTrack(OpenHome::Brn aTitle, OpenHome::Brn anArtist, OpenHome::Brn anAlbum);
     void setMode(OpenHome::Brn aMode);
     void setTime(OpenHome::TUint anElapsed, OpenHome::TUint aDuration);
     void setStreamInfo(OpenHome::TUint aBitRate, OpenHome::TUint aChannelNum, OpenHome::TUint aBitDepth, OpenHome::TUint aSampleFreq);
     void setVolume(OpenHome::TUint aVolume);

     OpenHome::Brn iTitle;
     OpenHome::Brn iArtist;
     OpenHome::Brn iAlbum;
     OpenHome::Brn iMode;
     OpenHome::TUint iElapsed;
     OpenHome::TUint iDuration;
     OpenHome::TUint iBitRate;
     OpenHome::TUint iChannelNum;
     OpenHome::TUint iBitDepth;
     OpenHome::TUint iSampleFreq;
     OpenHome::TUint iVolume;

 private : 
     OhMdpIF();
     ~OhMdpIF();
    static OhMdpIF * instance;
};
//}
#endif //HEADER_OHMDP
