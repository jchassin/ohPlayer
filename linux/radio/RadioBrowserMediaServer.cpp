#include "RadioBrowserMediaServer.h"
#include <OpenHome/Media/Protocol/ProtocolHls.h>  // UriLoaderError
#include <OpenHome/Net/Private/DviStack.h>
#include <OpenHome/Net/Core/DvDevice.h>
#include <OpenHome/DidlLite.h>
#include <OpenHome/Private/Ascii.h>
#include <OpenHome/Private/Printer.h>
#include <OpenHome/Private/Converter.h>
#include <chrono>

using namespace OpenHome;

namespace OpenHome {
namespace Av {

static const Brn kProtocolInfoSink("*:*:*:*"); // accept anything; playback is done by the renderer's own protocols

// RadioBrowserMediaServer::ContentDirectory

RadioBrowserMediaServer::ContentDirectory::ContentDirectory(Net::DvDeviceStandard& aDevice, RadioBrowserMediaServer& aServer)
    : DvProviderUpnpOrgContentDirectory1(aDevice)
    , iServer(aServer)
{
    // The generated action GetSystemUpdateID binds its output parameter to the
    // SystemUpdateID property; the property must exist (and be initialised)
    // before any SCPD request or event subscription dereferences it.
    EnablePropertySystemUpdateID();
    SetPropertySystemUpdateID(1);
    EnableActionGetSearchCapabilities();
    EnableActionGetSortCapabilities();
    EnableActionGetSystemUpdateID();
    EnableActionBrowse();
    EnableActionSearch();
}

void RadioBrowserMediaServer::ContentDirectory::GetSearchCapabilities(Net::IDvInvocation& aInvocation, Net::IDvInvocationResponseString& aSearchCaps)
{
    aInvocation.StartResponse();
    aSearchCaps.Write(Brn("dc:title"));
    aSearchCaps.WriteFlush();
    aInvocation.EndResponse();
}

void RadioBrowserMediaServer::ContentDirectory::GetSortCapabilities(Net::IDvInvocation& aInvocation, Net::IDvInvocationResponseString& aSortCaps)
{
    aInvocation.StartResponse();
    aSortCaps.Write(Brx::Empty());
    aSortCaps.WriteFlush();
    aInvocation.EndResponse();
}

void RadioBrowserMediaServer::ContentDirectory::GetSystemUpdateID(Net::IDvInvocation& aInvocation, Net::IDvInvocationResponseUint& aId)
{
    aInvocation.StartResponse();
    aId.Write(iServer.iSystemUpdateId);
    aInvocation.EndResponse();
}

void RadioBrowserMediaServer::ContentDirectory::Browse(Net::IDvInvocation& aInvocation, const Brx& aObjectID, const Brx& aBrowseFlag, const Brx& /*aFilter*/,
                                                       TUint aStartingIndex, TUint aRequestedCount, const Brx& /*aSortCriteria*/,
                                                       Net::IDvInvocationResponseString& aResult, Net::IDvInvocationResponseUint& aNumberReturned,
                                                       Net::IDvInvocationResponseUint& aTotalMatches, Net::IDvInvocationResponseUint& aUpdateID)
{
    Bws<16384> didl;
    TUint numberReturned = 0;
    TUint totalMatches = 0;
    iServer.UpdateId();
    try {
        // BrowseMetadata: return the object itself (as a container).
        static const Brn kBrowseMetadata("BrowseMetadata");
        if (aBrowseFlag == kBrowseMetadata) {
            std::lock_guard<std::mutex> lock(iServer.iLock);
            iServer.WriteContainerDidl(didl, aObjectID, aObjectID, 0);
            numberReturned = 1;
            totalMatches = 1;
        }
        else {
        std::lock_guard<std::mutex> lock(iServer.iLock);
        static const Brn kRoot("0");
        static const Brn kTop("top");
        static const Brn kCountry("country");
        static const Brn kGenre("genre");
        static const Brn kSearch("search");
        static const Brn kCountryPrefix("country/");
        static const Brn kGenrePrefix("genre/");

        if (aObjectID == kRoot) {
            iServer.BrowseRoot(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
        }
        else if (aObjectID == kTop) {
            iServer.BrowseTop(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
        }
        else if (aObjectID == kCountry) {
            iServer.BrowseCountry(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
        }
        else if (aObjectID == kGenre) {
            iServer.BrowseGenre(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
        }
        else if (aObjectID == kSearch) {
            iServer.BrowseSearch(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
        }
        else if (aObjectID.Bytes() > kCountryPrefix.Bytes() &&
                 Brn(aObjectID.Ptr(), kCountryPrefix.Bytes()) == kCountryPrefix) {
            Brn country(aObjectID.Ptr() + kCountryPrefix.Bytes(), aObjectID.Bytes() - kCountryPrefix.Bytes());
            iServer.BrowseCountryStations(country, didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
        }
        else if (aObjectID.Bytes() > kGenrePrefix.Bytes() &&
                 Brn(aObjectID.Ptr(), kGenrePrefix.Bytes()) == kGenrePrefix) {
            Brn genre(aObjectID.Ptr() + kGenrePrefix.Bytes(), aObjectID.Bytes() - kGenrePrefix.Bytes());
            iServer.BrowseGenreStations(genre, didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
        }
        else {
            // unknown object: empty result
        }
        }
    }
    catch (UriLoaderError&) {
        Log::Print("RadioBrowserMediaServer: API request failed\n");
        didl.SetBytes(0);
    }

    aInvocation.StartResponse();
    aResult.Write(didl);
    aResult.WriteFlush();
    aNumberReturned.Write(numberReturned);
    aTotalMatches.Write(totalMatches);
    aUpdateID.Write(iServer.iSystemUpdateId);
    aInvocation.EndResponse();
}

void RadioBrowserMediaServer::ContentDirectory::Search(Net::IDvInvocation& aInvocation, const Brx& /*aContainerID*/, const Brx& aSearchCriteria,
                                                       const Brx& /*aFilter*/, TUint aStartingIndex, TUint aRequestedCount, const Brx& /*aSortCriteria*/,
                                                       Net::IDvInvocationResponseString& aResult, Net::IDvInvocationResponseUint& aNumberReturned,
                                                       Net::IDvInvocationResponseUint& aTotalMatches, Net::IDvInvocationResponseUint& aUpdateID)
{
    // The control point search box sends criteria like: dc:title contains "jazz"
    Bws<128> term;
    Log::Print("RadioBrowserMediaServer: Search criteria received: %.*s\n", PBUF(aSearchCriteria));
    {
        static const Brn kContains("contains \"");
        const TByte* ptr = aSearchCriteria.Ptr();
        const TUint bytes = aSearchCriteria.Bytes();
        TUint i = 0;
        // find the opening quote of contains "..."
        for (; i + kContains.Bytes() <= bytes; i++) {
            if (Brn(ptr + i, kContains.Bytes()) == kContains) {
                i += kContains.Bytes();
                break;
            }
        }
        if (i < bytes) {
            for (; i < bytes; i++) {
                const TByte ch = ptr[i];
                if (ch == '"') {
                    break;
                }
                if (term.Bytes() < term.MaxBytes()) {
                    term.Append(ch);
                }
            }
        }
    }

    Log::Print("RadioBrowserMediaServer: extracted search term: %.*s\n", PBUF(term));
    Bws<16384> didl;
    TUint numberReturned = 0;
    TUint totalMatches = 0;
    iServer.UpdateId();
    try {
        std::lock_guard<std::mutex> lock(iServer.iLock);
        iServer.RunSearch(term);
        iServer.BrowseSearch(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
    }
    catch (UriLoaderError&) {
        Log::Print("RadioBrowserMediaServer: search request failed\n");
    }

    aInvocation.StartResponse();
    aResult.Write(didl);
    aResult.WriteFlush();
    aNumberReturned.Write(numberReturned);
    aTotalMatches.Write(totalMatches);
    aUpdateID.Write(iServer.iSystemUpdateId);
    aInvocation.EndResponse();
}

// RadioBrowserMediaServer::ConnectionManager

RadioBrowserMediaServer::ConnectionManager::ConnectionManager(Net::DvDeviceStandard& aDevice)
    : DvProviderUpnpOrgConnectionManager1(aDevice)
{
    // Generated actions bind output parameters to these properties; they must
    // exist and be initialised.
    static const Brn kProtocolInfo("*:*:*:*");
    EnablePropertySourceProtocolInfo();
    SetPropertySourceProtocolInfo(Brx::Empty());
    EnablePropertySinkProtocolInfo();
    SetPropertySinkProtocolInfo(kProtocolInfo);
    EnablePropertyCurrentConnectionIDs();
    SetPropertyCurrentConnectionIDs(Brx::Empty());
    EnableActionGetProtocolInfo();
    EnableActionGetCurrentConnectionIDs();
    EnableActionGetCurrentConnectionInfo();
}

void RadioBrowserMediaServer::ConnectionManager::GetProtocolInfo(Net::IDvInvocation& aInvocation, Net::IDvInvocationResponseString& aSource, Net::IDvInvocationResponseString& aSink)
{
    aInvocation.StartResponse();
    aSource.Write(Brx::Empty());
    aSource.WriteFlush();
    aSink.Write(kProtocolInfoSink);
    aSink.WriteFlush();
    aInvocation.EndResponse();
}

void RadioBrowserMediaServer::ConnectionManager::GetCurrentConnectionIDs(Net::IDvInvocation& aInvocation, Net::IDvInvocationResponseString& aConnectionIDs)
{
    aInvocation.StartResponse();
    aConnectionIDs.Write(Brx::Empty());
    aConnectionIDs.WriteFlush();
    aInvocation.EndResponse();
}

void RadioBrowserMediaServer::ConnectionManager::GetCurrentConnectionInfo(Net::IDvInvocation& aInvocation, TInt /*aConnectionID*/,
                                                                          Net::IDvInvocationResponseInt& aRcsID, Net::IDvInvocationResponseInt& aAVTransportID,
                                                                          Net::IDvInvocationResponseString& aProtocolInfo, Net::IDvInvocationResponseString& aPeerConnectionManager,
                                                                          Net::IDvInvocationResponseInt& aPeerConnectionID, Net::IDvInvocationResponseString& aDirection,
                                                                          Net::IDvInvocationResponseString& aStatus)
{
    aInvocation.StartResponse();
    aRcsID.Write(0);
    aAVTransportID.Write(0);
    aProtocolInfo.Write(Brx::Empty());
    aProtocolInfo.WriteFlush();
    aPeerConnectionManager.Write(Brx::Empty());
    aPeerConnectionManager.WriteFlush();
    aPeerConnectionID.Write(0);
    aDirection.Write(Brn("Output"));
    aDirection.WriteFlush();
    aStatus.Write(Brn("OK"));
    aStatus.WriteFlush();
    aInvocation.EndResponse();
}

// RadioBrowserMediaServer

RadioBrowserMediaServer::RadioBrowserMediaServer(Net::DvStack& aDvStack,
                                                 Environment& aEnv,
                                                 SslContext& aSsl,
                                                 const Brx& aUserAgent,
                                                 const Brx& aUdn,
                                                 const Brx& aFriendlyName)
    : iDevice(nullptr)
    , iTimerFactory(aEnv)
    , iClient(aEnv, aSsl, aUserAgent, iTimerFactory)
    , iSystemUpdateId(1)
    , iTopCacheTimeMs(0)
    , iSearchCacheTimeMs(0)
{
    iDevice = new Net::DvDeviceStandard(aDvStack, aUdn);
    iDevice->SetAttribute("Upnp.Domain", "upnp.org");
    iDevice->SetAttribute("Upnp.Type", "MediaServer");
    iDevice->SetAttribute("Upnp.Version", "1");
    iDevice->SetAttribute("Upnp.Manufacturer", "OpenHome");
    iDevice->SetAttribute("Upnp.ModelName", "Radio Browser");
    iDevice->SetAttribute("Upnp.ModelDescription", "Community radio directory (radio-browser.info)");
    Bws<128> friendlyName("Radio Browser ");
    friendlyName.Append(aFriendlyName);
    friendlyName.PtrZ(); // ensure null termination for SetAttribute
    iDevice->SetAttribute("Upnp.FriendlyName", reinterpret_cast<const TChar*>(friendlyName.Ptr()));

    iContentDirectory = std::make_unique<ContentDirectory>(*iDevice, *this);
    iConnectionManager = std::make_unique<ConnectionManager>(*iDevice);

    // Short country / genre lists (defaults; configurable later via web ui).
    iCountries = { Brn("FR"), Brn("ES"), Brn("GB"), Brn("DE"), Brn("IT"), Brn("US") };
    iGenres = { Brn("jazz"), Brn("rock"), Brn("classical"), Brn("news"), Brn("pop"), Brn("electronic") };
}

RadioBrowserMediaServer::~RadioBrowserMediaServer()
{
    delete iDevice;
}

void RadioBrowserMediaServer::Start()
{
    iDevice->SetEnabled();
}

void RadioBrowserMediaServer::SetCountryList(const std::vector<Brn>& aCountries)
{
    std::lock_guard<std::mutex> lock(iLock);
    iCountries = aCountries;
    UpdateId();
}

void RadioBrowserMediaServer::SetGenreList(const std::vector<Brn>& aGenres)
{
    std::lock_guard<std::mutex> lock(iLock);
    iGenres = aGenres;
    UpdateId();
}

void RadioBrowserMediaServer::UpdateId()
{
    iSystemUpdateId++;
}

void RadioBrowserMediaServer::BrowseRoot(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches)
{
    static const Brn kTopId("top");
    static const Brn kCountryId("country");
    static const Brn kGenreId("genre");
    static const Brn kSearchId("search");
    static const Brn kTopTitle("Most popular");
    static const Brn kCountryTitle("By country");
    static const Brn kGenreTitle("By genre");
    static const Brn kSearchTitle("Search results");
    struct Entry { const Brx* id; const Brx* title; };
    const Entry entries[] = {
        { &kTopId, &kTopTitle },
        { &kCountryId, &kCountryTitle },
        { &kGenreId, &kGenreTitle },
        { &kSearchId, &kSearchTitle },
    };
    const TUint total = sizeof(entries) / sizeof(entries[0]);
    aTotalMatches = total;
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < total && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        WriteContainerDidl(aDidl, *entries[i].id, *entries[i].title, 0);
        returned++;
    }
    aNumberReturned = returned;
}

void RadioBrowserMediaServer::BrowseTop(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches)
{
    // Note: didl buffer for stations is bigger; reuse via a local
    const std::vector<RadioBrowserClient::Station>& stations = TopStations();
    aTotalMatches = stations.size();
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < stations.size() && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        Bws<2048> entry;
        WriteStationDidl(entry, stations[i], i);
        if (aDidl.Bytes() + entry.Bytes() <= aDidl.MaxBytes()) {
            aDidl.Append(entry);
            returned++;
        }
        else {
            break;
        }
    }
    aNumberReturned = returned;
}

void RadioBrowserMediaServer::BrowseCountry(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches)
{
    const TUint total = iCountries.size();
    aTotalMatches = total;
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < total && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        Bws<64> id("country/");
        id.Append(iCountries[i]);
        WriteContainerDidl(aDidl, id, iCountries[i], 0);
        returned++;
    }
    aNumberReturned = returned;
}

void RadioBrowserMediaServer::BrowseCountryStations(const Brx& aCountry, Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches)
{
    const std::vector<RadioBrowserClient::Station>& stations = CountryStations(aCountry);
    aTotalMatches = stations.size();
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < stations.size() && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        Bws<2048> entry;
        WriteStationDidl(entry, stations[i], i);
        if (aDidl.Bytes() + entry.Bytes() <= aDidl.MaxBytes()) {
            aDidl.Append(entry);
            returned++;
        }
        else {
            break;
        }
    }
    aNumberReturned = returned;
}

void RadioBrowserMediaServer::BrowseGenre(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches)
{
    const TUint total = iGenres.size();
    aTotalMatches = total;
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < total && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        Bws<64> id("genre/");
        id.Append(iGenres[i]);
        WriteContainerDidl(aDidl, id, iGenres[i], 0);
        returned++;
    }
    aNumberReturned = returned;
}

void RadioBrowserMediaServer::BrowseGenreStations(const Brx& aGenre, Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches)
{
    const std::vector<RadioBrowserClient::Station>& stations = GenreStations(aGenre);
    aTotalMatches = stations.size();
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < stations.size() && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        Bws<2048> entry;
        WriteStationDidl(entry, stations[i], i);
        if (aDidl.Bytes() + entry.Bytes() <= aDidl.MaxBytes()) {
            aDidl.Append(entry);
            returned++;
        }
        else {
            break;
        }
    }
    aNumberReturned = returned;
}

void RadioBrowserMediaServer::BrowseSearch(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches)
{
    aTotalMatches = iSearchCache.size();
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < iSearchCache.size() && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        Bws<2048> entry;
        WriteStationDidl(entry, iSearchCache[i], i);
        if (aDidl.Bytes() + entry.Bytes() <= aDidl.MaxBytes()) {
            aDidl.Append(entry);
            returned++;
        }
        else {
            break;
        }
    }
    aNumberReturned = returned;
}

void RadioBrowserMediaServer::RunSearch(const Brx& aName)
{
    if (aName.Bytes() == 0) {
        return;
    }
    // refresh the search cache if the term changed or the cache is stale
    const TBool sameTerm = (iSearchTerm.Bytes() == aName.Bytes() &&
                            Brn(iSearchTerm.Ptr(), iSearchTerm.Bytes()) == Brn(aName.Ptr(), aName.Bytes()));
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    if (sameTerm && now - iSearchCacheTimeMs < kCacheLifetimeMs) {
        return;
    }
    iClient.SearchByName(aName, iSearchCache, kMaxStations);
    iSearchTerm = Brn(aName.Ptr(), aName.Bytes());
    iSearchCacheTimeMs = now;
}

const std::vector<RadioBrowserClient::Station>& RadioBrowserMediaServer::TopStations()
{
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    if (now - iTopCacheTimeMs >= kCacheLifetimeMs) {
        iClient.TopStations(iTopCache, kMaxStations);
        iTopCacheTimeMs = now;
    }
    return iTopCache;
}

const std::vector<RadioBrowserClient::Station>& RadioBrowserMediaServer::CountryStations(const Brx& aCountry)
{
    // no per-country cache: short lists, browse events are rare
    iCountryScratch.clear();
    iClient.ByCountryCode(aCountry, iCountryScratch, kMaxStations);
    return iCountryScratch;
}

const std::vector<RadioBrowserClient::Station>& RadioBrowserMediaServer::GenreStations(const Brx& aGenre)
{
    iGenreScratch.clear();
    iClient.ByTag(aGenre, iGenreScratch, kMaxStations);
    return iGenreScratch;
}

void RadioBrowserMediaServer::WriteStationDidl(Bws<2048>& aDidl, const RadioBrowserClient::Station& aStation, TUint aIndex)
{
    // object id: "station/<index within the enclosing list>" is not stable
    // across requests; use the stream uri itself as the id (unique enough for
    // browsing purposes; the CP only needs the res uri to play).
    WriterBuffer w(aDidl);
    WriterDIDLLite writer(Brx::Empty(), DIDLLite::kItemTypeAudioItem, w);
    writer.WriteTitle(aStation.Name());
    WriterDIDLLite::StreamingDetails details;
    writer.WriteStreamingDetails(kProtocolInfoSink, details, aStation.Uri());
    if (aStation.ImageUri().Bytes() > 0) {
        writer.WriteArtwork(aStation.ImageUri());
    }
    writer.WriteEnd();
}

void RadioBrowserMediaServer::WriteContainerDidl(Bws<16384>& aDidl, const Brx& aObjectId, const Brx& aTitle, TUint aChildCount)
{
    // WriterDIDLLite only covers items; containers are written directly.
    WriterBuffer w(aDidl);
    w.Write(Brn("<container id=\""));
    Converter::ToXmlEscaped(w, aObjectId);
    w.Write(Brn("\" parentID=\"0\" restricted=\"1\"><dc:title>"));
    Converter::ToXmlEscaped(w, aTitle);
    w.Write(Brn("</dc:title><upnp:class>object.container</upnp:class><upnp:childCount>"));
    Ascii::StreamWriteUint(w, aChildCount);
    w.Write(Brn("</upnp:childCount></container>"));
}

} // namespace Av
} // namespace OpenHome
