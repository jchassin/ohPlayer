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


static const char* kDidlPreamble =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
    " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
    " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">";

static const char* kDidlEpilogue = "</DIDL-Lite>";

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
            didl.Append(Brn(kDidlPreamble));
            iServer.WriteContainerDidl(didl, aObjectID, aObjectID, 0);
            didl.Append(Brn(kDidlEpilogue));
            numberReturned = 1;
            totalMatches = 1;
        }
        else {
        std::lock_guard<std::mutex> lock(iServer.iLock);
        static const Brn kRoot("0");
        static const Brn kTop("top");
        static const Brn kCountry("country");
        static const Brn kGenre("genre");
        static const Brn kName("name");
        static const Brn kSearch("search");
        static const Brn kCountryPrefix("country/");
        static const Brn kGenrePrefix("genre/");
        static const Brn kNamePrefix("name/");

        if (aObjectID == kRoot) {
            didl.Append(Brn(kDidlPreamble));
            iServer.BrowseRoot(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
            didl.Append(Brn(kDidlEpilogue));
        }
        else if (aObjectID == kTop) {
            didl.Append(Brn(kDidlPreamble));
            iServer.BrowseTop(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
            didl.Append(Brn(kDidlEpilogue));
        }
        else if (aObjectID == kCountry) {
            didl.Append(Brn(kDidlPreamble));
            iServer.BrowseCountry(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
            didl.Append(Brn(kDidlEpilogue));
        }
        else if (aObjectID == kGenre) {
            didl.Append(Brn(kDidlPreamble));
            iServer.BrowseGenre(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
            didl.Append(Brn(kDidlEpilogue));
        }
        else if (aObjectID == kName) {
            didl.Append(Brn(kDidlPreamble));
            iServer.BrowseName(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
            didl.Append(Brn(kDidlEpilogue));
        }
        else if (aObjectID == kSearch) {
            didl.Append(Brn(kDidlPreamble));
            iServer.BrowseSearch(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
            didl.Append(Brn(kDidlEpilogue));
        }
        else if (aObjectID.Bytes() > kCountryPrefix.Bytes() &&
                 Brn(aObjectID.Ptr(), kCountryPrefix.Bytes()) == kCountryPrefix) {
            Brn country(aObjectID.Ptr() + kCountryPrefix.Bytes(), aObjectID.Bytes() - kCountryPrefix.Bytes());
            didl.Append(Brn(kDidlPreamble));
            iServer.BrowseCountryStations(country, didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
            didl.Append(Brn(kDidlEpilogue));
        }
        else if (aObjectID.Bytes() > kGenrePrefix.Bytes() &&
                 Brn(aObjectID.Ptr(), kGenrePrefix.Bytes()) == kGenrePrefix) {
            Brn genre(aObjectID.Ptr() + kGenrePrefix.Bytes(), aObjectID.Bytes() - kGenrePrefix.Bytes());
            didl.Append(Brn(kDidlPreamble));
            iServer.BrowseGenreStations(genre, didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
            didl.Append(Brn(kDidlEpilogue));
        }
        else if (aObjectID.Bytes() > kNamePrefix.Bytes() &&
                 Brn(aObjectID.Ptr(), kNamePrefix.Bytes()) == kNamePrefix) {
            Brn letter(aObjectID.Ptr() + kNamePrefix.Bytes(), aObjectID.Bytes() - kNamePrefix.Bytes());
            didl.Append(Brn(kDidlPreamble));
            iServer.BrowseNameStations(letter, didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
            didl.Append(Brn(kDidlEpilogue));
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

    Bws<16384> didl;
    TUint numberReturned = 0;
    TUint totalMatches = 0;
    iServer.UpdateId();
    try {
        std::lock_guard<std::mutex> lock(iServer.iLock);
        iServer.RunSearch(term);
        didl.Append(Brn(kDidlPreamble));
        iServer.BrowseSearch(didl, aStartingIndex, aRequestedCount, numberReturned, totalMatches);
        didl.Append(Brn(kDidlEpilogue));
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
    static const Brn kNameId("name");
    static const Brn kSearchId("search");
    static const Brn kTopTitle("Most popular");
    static const Brn kCountryTitle("By country");
    static const Brn kGenreTitle("By genre");
    static const Brn kNameTitle("By name");
    static const Brn kSearchTitle("Search results");
    // Control points use upnp:childCount to decide whether a container is
    // worth browsing; announcing 0 makes the Linn app show an empty root.
    // Report realistic counts (top may not be cached yet at first browse;
    // use the cache size or the max as a hint).
    const TUint topCount = (iTopCache.empty() ? kMaxStations : (TUint)iTopCache.size());
    const TUint countryCount = (TUint)iCountries.size();
    const TUint genreCount = (TUint)iGenres.size();
    const TUint nameCount = 26; // A-Z
    struct Entry { const Brx* id; const Brx* title; TUint childCount; };
    const Entry entries[] = {
        { &kTopId, &kTopTitle, topCount },
        { &kCountryId, &kCountryTitle, countryCount },
        { &kGenreId, &kGenreTitle, genreCount },
        { &kNameId, &kNameTitle, nameCount },
        { &kSearchId, &kSearchTitle, (TUint)iSearchCache.size() },
    };
    const TUint total = sizeof(entries) / sizeof(entries[0]);
    aTotalMatches = total;
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < total && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        WriteContainerDidl(aDidl, *entries[i].id, *entries[i].title, entries[i].childCount);
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
    // Do not advertise more than this page can serve: the 16KB didl buffer
    // caps the number of items per response; announcing the full list
    // invites control points to page through (each page = a fresh slow api
    // call), which times out. Clamp to what is here.
    if (returned < aRequestedCount || aRequestedCount == 0) {
        aTotalMatches = aStartingIndex + returned;
    }
}

void RadioBrowserMediaServer::BrowseCountry(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches)
{
    const TUint total = iCountries.size();
    aTotalMatches = total;
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < total && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        Bws<64> id("country/");
        id.Append(iCountries[i]);
        // hint at the number of stations; the exact count requires an api call
        WriteContainerDidl(aDidl, id, iCountries[i], kMaxStations);
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
    // Do not advertise more than this page can serve: the 16KB didl buffer
    // caps the number of items per response; announcing the full list
    // invites control points to page through (each page = a fresh slow api
    // call), which times out. Clamp to what is here.
    if (returned < aRequestedCount || aRequestedCount == 0) {
        aTotalMatches = aStartingIndex + returned;
    }
}

void RadioBrowserMediaServer::BrowseGenre(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches)
{
    const TUint total = iGenres.size();
    aTotalMatches = total;
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < total && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        Bws<64> id("genre/");
        id.Append(iGenres[i]);
        WriteContainerDidl(aDidl, id, iGenres[i], kMaxStations);
        returned++;
    }
    aNumberReturned = returned;
}

void RadioBrowserMediaServer::BrowseName(Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches)
{
    // A-Z, using the ascii codes 0x41..0x5A
    const TUint total = 26;
    aTotalMatches = total;
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < total && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        Bws<8> id("name/");
        Bws<2> letter;
        letter.Append(static_cast<TByte>('A' + i));
        id.Append(letter);
        WriteContainerDidl(aDidl, id, letter, kMaxStations);
        returned++;
    }
    aNumberReturned = returned;
}

void RadioBrowserMediaServer::BrowseNameStations(const Brx& aLetter, Bws<16384>& aDidl, TUint aStartingIndex, TUint aRequestedCount, TUint& aNumberReturned, TUint& aTotalMatches)
{
    // Search stations whose name starts with the letter (cached).
    // The api's name search is a 'contains' match; filter locally on the
    // first character (case-insensitive) to get a true 'starts with'.
    Bws<16> key("name:");
    key.Append(aLetter);
    const std::vector<RadioBrowserClient::Station>& filteredRef = CachedStations(
        key, [this, &aLetter](std::vector<RadioBrowserClient::Station>& v) {
            std::vector<RadioBrowserClient::Station> raw;
            iClient.SearchByName(aLetter, raw, kMaxStations * 4);
            const TByte target = static_cast<TByte>(Ascii::ToUpperCase(aLetter.Bytes() > 0 ? aLetter[0] : ' '));
            for (const auto& station : raw) {
                if (station.Name().Bytes() > 0) {
                    const TByte first = static_cast<TByte>(Ascii::ToUpperCase(station.Name()[0]));
                    if (first == target) {
                        v.push_back(station);
                        if (v.size() >= kMaxStations) {
                            break;
                        }
                    }
                }
            }
        });
    const std::vector<RadioBrowserClient::Station>& filtered = filteredRef;
    aTotalMatches = filtered.size();
    TUint returned = 0;
    for (TUint i = aStartingIndex; i < filtered.size() && (aRequestedCount == 0 || returned < aRequestedCount); i++) {
        Bws<2048> entry;
        WriteStationDidl(entry, filtered[i], i);
        if (aDidl.Bytes() + entry.Bytes() <= aDidl.MaxBytes()) {
            aDidl.Append(entry);
            returned++;
        }
        else {
            break;
        }
    }
    aNumberReturned = returned;
    // Do not advertise more than this page can serve: the 16KB didl buffer
    // caps the number of items per response; announcing the full list
    // invites control points to page through (each page = a fresh slow api
    // call), which times out. Clamp to what is here.
    if (returned < aRequestedCount || aRequestedCount == 0) {
        aTotalMatches = aStartingIndex + returned;
    }
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
    // Do not advertise more than this page can serve: the 16KB didl buffer
    // caps the number of items per response; announcing the full list
    // invites control points to page through (each page = a fresh slow api
    // call), which times out. Clamp to what is here.
    if (returned < aRequestedCount || aRequestedCount == 0) {
        aTotalMatches = aStartingIndex + returned;
    }
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
    // Do not advertise more than this page can serve: the 16KB didl buffer
    // caps the number of items per response; announcing the full list
    // invites control points to page through (each page = a fresh slow api
    // call), which times out. Clamp to what is here.
    if (returned < aRequestedCount || aRequestedCount == 0) {
        aTotalMatches = aStartingIndex + returned;
    }
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


const std::vector<RadioBrowserClient::Station>& RadioBrowserMediaServer::CachedStations(
    const Brx& aKey,
    std::function<void(std::vector<RadioBrowserClient::Station>&)> aFetcher)
{
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    for (auto& entry : iListCache) {
        if (entry.iKey == aKey) {
            if (now - entry.iTimeMs < kCacheLifetimeMs) {
                return entry.iStations;
            }
            entry.iStations.clear();
            aFetcher(entry.iStations);
            entry.iTimeMs = now;
            return entry.iStations;
        }
    }
    // new entry (bounded: drop the oldest when exceeding 8 cached lists)
    if (iListCache.size() >= 8) {
        iListCache.erase(iListCache.begin());
    }
    ListCacheEntry entry;
    entry.iKey.Replace(aKey);
    aFetcher(entry.iStations);
    entry.iTimeMs = now;
    iListCache.push_back(std::move(entry));
    return iListCache.back().iStations;
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
    Bws<32> key("country:");
    key.Append(aCountry);
    return CachedStations(key, [this, &aCountry](std::vector<RadioBrowserClient::Station>& v) {
        iClient.ByCountryCode(aCountry, v, kMaxStations);
    });
}

const std::vector<RadioBrowserClient::Station>& RadioBrowserMediaServer::GenreStations(const Brx& aGenre)
{
    Bws<64> key("genre:");
    key.Append(aGenre);
    return CachedStations(key, [this, &aGenre](std::vector<RadioBrowserClient::Station>& v) {
        iClient.ByTag(aGenre, v, kMaxStations);
    });
}

void RadioBrowserMediaServer::WriteStationDidl(Bws<2048>& aDidl, const RadioBrowserClient::Station& aStation, TUint aIndex)
{
    // Write a bare <item> fragment; the caller wraps the response (once) in a
    // single DIDL-Lite document. (WriterDIDLLite writes a complete xml
    // document per item, which cannot be concatenated for a multi-item
    // response.)
    WriterBuffer w(aDidl);
    w.Write(Brn("<item id=\"station-"));
    Ascii::StreamWriteUint(w, aIndex);
    w.Write(Brn("\" parentID=\"-1\" restricted=\"1\">"));
    w.Write(Brn("<dc:title>"));
    Converter::ToXmlEscaped(w, aStation.Name());
    w.Write(Brn("</dc:title>"));
    w.Write(Brn("<upnp:class>object.item.audioItem</upnp:class>"));
    if (aStation.ImageUri().Bytes() > 0) {
        w.Write(Brn("<upnp:albumArtURI>"));
        Converter::ToXmlEscaped(w, aStation.ImageUri());
        w.Write(Brn("</upnp:albumArtURI>"));
    }
    w.Write(Brn("<res protocolInfo=\""));
    w.Write(kProtocolInfoSink);
    w.Write(Brn("\">"));
    Converter::ToXmlEscaped(w, aStation.Uri());
    w.Write(Brn("</res>"));
    w.Write(Brn("</item>"));
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
