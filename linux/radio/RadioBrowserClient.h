#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
// UriLoader (from ProtocolHls.h) is held by pointer to avoid leaking that
// header (and its exceptions) into every including translation unit.
#include <OpenHome/Private/Uri.h>
#include <vector>

namespace OpenHome {
    class Environment;
    class SslContext;
    class ITimerFactory;
    class IReader;
namespace Media {
class UriLoader;
}
namespace Av {

// Client for the community radio directory at radio-browser.info
// (https://api.radio-browser.info). Uses the public JSON API: no key, no auth.
// The server should be one of the load-balanced mirrors, e.g.
// de1.api.radio-browser.info, or the all-in-one round-robin name
// all.api.radio-browser.info.
//
// All calls are synchronous (blocking); callers are expected to run them on a
// worker thread (e.g. the web ui / UPnP invoker threads), not on the pipeline.
class RadioBrowserClient
{
public:
    static const TUint kDefaultMaxResults = 100;
    static const TUint kUriMaxBytes = 512;
    static const TUint kNameMaxBytes = 256;
    static const TUint kImageMaxBytes = 512;
    static const TUint kTagsMaxBytes = 256;
public:
    class Station
    {
    public:
        Station();
        const Brx& Name() const { return iName; }
        const Brx& Uri() const { return iUri; }         // resolved stream url
        const Brx& ImageUri() const { return iImage; }   // favicon (may be empty)
        const Brx& Tags() const { return iTags; }
        const Brx& CountryCode() const { return iCountry; }
        const Brx& Codec() const { return iCodec; }
        TUint Bitrate() const { return iBitrate; }
        TBool Hls() const { return iHls; }
    private:
        friend class RadioBrowserClient;
        Bwh iName;
        Bwh iUri;
        Bwh iImage;
        Bwh iTags;
        Bwh iCountry;
        Bwh iCodec;
        TUint iBitrate;
        TBool iHls;
    };

public:
    RadioBrowserClient(Environment& aEnv, SslContext& aSsl, const Brx& aUserAgent,
                       ITimerFactory& aTimerFactory);
    ~RadioBrowserClient();

    // Set the API server (host name only, e.g. "de1.api.radio-browser.info").
    void SetServer(const Brx& aServer);

    // Search stations by (partial) name.
    void SearchByName(const Brx& aName, std::vector<Station>& aResults,
                      TUint aMaxResults = kDefaultMaxResults);

    // Top stations globally (by click count).
    void TopStations(std::vector<Station>& aResults,
                     TUint aMaxResults = kDefaultMaxResults);

    // Stations of a country (ISO 3166-1 alpha-2 code, e.g. "FR", "ES").
    void ByCountryCode(const Brx& aCountryCode, std::vector<Station>& aResults,
                       TUint aMaxResults = kDefaultMaxResults);

    // Stations carrying a tag (e.g. "jazz", "rock").
    void ByTag(const Brx& aTag, std::vector<Station>& aResults,
               TUint aMaxResults = kDefaultMaxResults);

private:
    void Execute(const Brx& aPathAndQuery, std::vector<Station>& aResults,
                 TUint aMaxResults);
    void ParseStations(IReader& aReader, std::vector<Station>& aResults,
                       TUint aMaxResults);

private:
    Media::UriLoader* iLoader;
    Bwh iServer;      // host name of the API server
    Bwh iRequestUri;  // scratch for building request uris
};

} // namespace Av
} // namespace OpenHome
