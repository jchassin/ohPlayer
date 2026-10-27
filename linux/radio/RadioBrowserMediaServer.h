#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>
#include <OpenHome/Net/Core/DvDevice.h>
#include <OpenHome/Net/Core/DvInvocationResponse.h>
#include <Generated/DvUpnpOrgContentDirectory1.h>
#include <Generated/DvUpnpOrgConnectionManager1.h>
#include "RadioBrowserClient.h"

#include <vector>
#include <memory>
#include <mutex>

namespace OpenHome {
    class Environment;
    class SslContext;
    class ITimerFactory;
namespace Net {
    class DvStack;
}
namespace Av {

// DLNA Media Server exposing the radio-browser.info community directory.
//
// Control points (Linn app, BubbleUPnP, mconnect, ...) browse this server like
// any UPnP MediaServer: navigate the directory (Top stations, By country,
// By genre, Search) and play a station by pushing its stream url to the
// renderer via SetAVTransportURI.
//
// Container layout (object ids):
//   "0"                root
//   "top"              most popular stations
//   "country"          country list (short, configurable)
//   "country/<code>"   stations of that country
//   "genre"            genre list (short, configurable)
//   "genre/<tag>"      stations of that genre
//   "search"           last search results (via the Search action, or the
//                      web ui); browsing it re-runs the last search
//
// Results are cached briefly so that paging through a list does not hammer
// the API.
class RadioBrowserMediaServer
{
public:
    static const TUint kCacheLifetimeMs = 60 * 1000;
    static const TUint kMaxStations = 200;
public:
    RadioBrowserMediaServer(Net::DvStack& aDvStack,
                            Environment& aEnv,
                            SslContext& aSsl,
                            const Brx& aUserAgent,
                            const Brx& aUdn,          // full UPnP udn: uuid:xxxxxxxx-...
                            const Brx& aFriendlyName);
    ~RadioBrowserMediaServer();

    void Start();

    // Set the short country / genre lists (e.g. {"FR","ES"} / {"jazz","rock"}).
    void SetCountryList(const std::vector<Brn>& aCountries);
    void SetGenreList(const std::vector<Brn>& aGenres);

private:
    class ContentDirectory : public Net::DvProviderUpnpOrgContentDirectory1
    {
    public:
        ContentDirectory(Net::DvDeviceStandard& aDevice, RadioBrowserMediaServer& aServer);
    private: // DvUpnpOrgContentDirectory1
        void GetSearchCapabilities(Net::IDvInvocation& aInvocation, Net::IDvInvocationResponseString& aSearchCaps) override;
        void GetSortCapabilities(Net::IDvInvocation& aInvocation, Net::IDvInvocationResponseString& aSortCaps) override;
        void GetSystemUpdateID(Net::IDvInvocation& aInvocation, Net::IDvInvocationResponseUint& aId) override;
        void Browse(Net::IDvInvocation& aInvocation, const Brx& aObjectID, const Brx& aBrowseFlag, const Brx& aFilter, TUint aStartingIndex, TUint aRequestedCount, const Brx& aSortCriteria, Net::IDvInvocationResponseString& aResult, Net::IDvInvocationResponseUint& aNumberReturned, Net::IDvInvocationResponseUint& aTotalMatches, Net::IDvInvocationResponseUint& aUpdateID) override;
        void Search(Net::IDvInvocation& aInvocation, const Brx& aContainerID, const Brx& aSearchCriteria, const Brx& aFilter, TUint aStartingIndex, TUint aRequestedCount, const Brx& aSortCriteria, Net::IDvInvocationResponseString& aResult, Net::IDvInvocationResponseUint& aNumberReturned, Net::IDvInvocationResponseUint& aTotalMatches, Net::IDvInvocationResponseUint& aUpdateID) override;
    private:
        RadioBrowserMediaServer& iServer;
    };

    class ConnectionManager : public Net::DvProviderUpnpOrgConnectionManager1
    {
    public:
        ConnectionManager(Net::DvDeviceStandard& aDevice);
    private: // DvUpnpOrgConnectionManager1
        void GetProtocolInfo(Net::IDvInvocation& aInvocation, Net::IDvInvocationResponseString& aSource, Net::IDvInvocationResponseString& aSink) override;
        void GetCurrentConnectionIDs(Net::IDvInvocation& aInvocation, Net::IDvInvocationResponseString& aConnectionIDs) override;
        void GetCurrentConnectionInfo(Net::IDvInvocation& aInvocation, TInt aConnectionID, Net::IDvInvocationResponseInt& aRcsID, Net::IDvInvocationResponseInt& aAVTransportID, Net::IDvInvocationResponseString& aProtocolInfo, Net::IDvInvocationResponseString& aPeerConnectionManager, Net::IDvInvocationResponseInt& aPeerConnectionID, Net::IDvInvocationResponseString& aDirection, Net::IDvInvocationResponseString& aStatus) override;
    };

private:
    void BrowseRoot(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches);
    void BrowseTop(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches);
    void BrowseCountry(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches);
    void BrowseCountryStations(const Brx& aCountry, Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches);
    void BrowseGenre(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches);
    void BrowseName(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches);
    void BrowseNameStations(const Brx& aLetter, Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches);
    void BrowseGenreStations(const Brx& aGenre, Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches);
    void BrowseSearch(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches);
    void RunSearch(const Brx& aName);
    void WriteStationDidl(Bws<2048>& aDidl, const RadioBrowserClient::Station& aStation, TUint aIndex);
    void WriteContainerDidl(Bws<16384>& aDidl, const Brx& aObjectId, const Brx& aTitle, TUint aChildCount);
    const std::vector<RadioBrowserClient::Station>& TopStations();
    const std::vector<RadioBrowserClient::Station>& CountryStations(const Brx& aCountry);
    const std::vector<RadioBrowserClient::Station>& GenreStations(const Brx& aGenre);
    void UpdateId();

private:
    Net::DvDeviceStandard* iDevice;
    std::unique_ptr<ContentDirectory> iContentDirectory;
    std::unique_ptr<ConnectionManager> iConnectionManager;
    TimerFactory iTimerFactory;   // declared before iClient (init order)
    RadioBrowserClient iClient;
    TUint iSystemUpdateId;
    std::mutex iLock;
    // caches
    std::vector<RadioBrowserClient::Station> iTopCache;
    TUint64 iTopCacheTimeMs;
    std::vector<RadioBrowserClient::Station> iSearchCache;
    Brn iSearchTerm;
    TUint64 iSearchCacheTimeMs;
    std::vector<RadioBrowserClient::Station> iCountryScratch;
    std::vector<RadioBrowserClient::Station> iGenreScratch;
    std::vector<RadioBrowserClient::Station> iNameScratch;
    std::vector<Brn> iCountries;
    std::vector<Brn> iGenres;
};

} // namespace Av
} // namespace OpenHome
