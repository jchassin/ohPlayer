#include "RadioBrowserClient.h"
#include <OpenHome/Media/Protocol/ProtocolHls.h>
#include <OpenHome/Json.h>
#include <OpenHome/Private/Ascii.h>
#include <OpenHome/Private/Converter.h>
#include <OpenHome/Private/Printer.h>
#include <OpenHome/Private/Stream.h>

using namespace OpenHome;

namespace OpenHome {
namespace Av {

static const Brn kDefaultServer("de1.api.radio-browser.info");
static const TUint kRetryIntervalMs = 5 * 1000; // avoid a tight retry loop in synchronous handlers
// radio-browser.info requires clients to identify themselves; requests
// without a User-Agent are rejected.
static const Brn kUserAgent("ohPlayer/1.0 (github.com/jchassin/ohPlayer)");

// Normalised comparison key for stream urls: scheme, host case and any
// query string/fragment are ignored, so the same stream submitted as
// e.g. http://Host/stream.aac, https://host/stream.aac and
// https://host/stream.aac?id=x deduplicates to one entry.
static void AppendUriKey(Bwx& aKey, const Brx& aUri)
{
    // strip scheme
    TUint start = 0;
    if (aUri.Bytes() > 7 && Brn(aUri.Ptr(), 7) == Brn("http://")) {
        start = 7;
    }
    else if (aUri.Bytes() > 8 && Brn(aUri.Ptr(), 8) == Brn("https://")) {
        start = 8;
    }
    TUint end = aUri.Bytes();
    for (TUint i = start; i < aUri.Bytes(); i++) {
        const TByte ch = aUri[i];
        if (ch == '?' || ch == '#') {
            end = i;
            break;
        }
    }
    for (TUint i = start; i < end; i++) {
        TByte ch = aUri[i];
        if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<TByte>(ch - 'A' + 'a'); // lowercase host (and rest; harmless)
        }
        aKey.Append(ch);
    }
}
static const Brn kSchemeHttps("https://");

// RadioBrowserClient::Station

RadioBrowserClient::Station::Station()
    : iName(kNameMaxBytes)
    , iUri(kUriMaxBytes)
    , iImage(kImageMaxBytes)
    , iTags(kTagsMaxBytes)
    , iCountry(8)
    , iCodec(16)
    , iBitrate(0)
    , iHls(false)
{
}

// RadioBrowserClient

RadioBrowserClient::RadioBrowserClient(Environment& aEnv, SslContext& aSsl, const Brx& aUserAgent,
                                       ITimerFactory& aTimerFactory)
    : iLoader(new Media::UriLoader(aEnv, aSsl, kUserAgent, aTimerFactory, kRetryIntervalMs))
    , iServer(kUriMaxBytes)
    , iRequestUri(kUriMaxBytes)
{
    SetServer(kDefaultServer);
}

RadioBrowserClient::~RadioBrowserClient()
{
    delete iLoader;
}

void RadioBrowserClient::SetServer(const Brx& aServer)
{
    if (aServer.Bytes() > 0) {
        iServer.Replace(aServer);
    }
}

void RadioBrowserClient::SearchByName(const Brx& aName, std::vector<Station>& aResults,
                                      TUint aMaxResults)
{
    // Name may contain spaces; the api expects them percent-encoded or '+'
    Bwh query(kUriMaxBytes);
    query.Append(Brn("/json/stations/search?name="));
    for (TUint i = 0; i < aName.Bytes(); i++) {
        const TByte ch = aName[i];
        if (ch == ' ') {
            query.Append('+');
        }
        else if (Ascii::IsDigit(ch) || Ascii::IsAlphabetic(ch) || ch == '-' || ch == '_') {
            query.Append(ch);
        }
        // other characters are dropped to keep the request valid
    }
    query.Append(Brn("&order=clickcount&reverse=true&hidebroken=true"));
    Execute(query, aResults, aMaxResults);
}

void RadioBrowserClient::TopStations(std::vector<Station>& aResults, TUint aMaxResults)
{
    Execute(Brn("/json/stations/topclick?hidebroken=true"), aResults, aMaxResults);
}

void RadioBrowserClient::ByCountryCode(const Brx& aCountryCode, std::vector<Station>& aResults,
                                       TUint aMaxResults)
{
    Bwh query(128);
    query.Append(Brn("/json/stations/bycountrycodeexact/"));
    query.Append(aCountryCode);
    query.Append(Brn("?order=clickcount&reverse=true&hidebroken=true"));
    Execute(query, aResults, aMaxResults);
}

void RadioBrowserClient::ByTag(const Brx& aTag, std::vector<Station>& aResults,
                               TUint aMaxResults)
{
    Bwh query(256);
    query.Append(Brn("/json/stations/bytagexact/"));
    // tags in urls: spaces -> %20
    for (TUint i = 0; i < aTag.Bytes(); i++) {
        const TByte ch = aTag[i];
        if (ch == ' ') {
            query.Append(Brn("%20"));
        }
        else {
            query.Append(ch);
        }
    }
    query.Append(Brn("?order=clickcount&reverse=true&hidebroken=true"));
    Execute(query, aResults, aMaxResults);
}

void RadioBrowserClient::Execute(const Brx& aPathAndQuery, std::vector<Station>& aResults,
                                 TUint aMaxResults)
{
    aResults.clear();
    iRequestUri.SetBytes(0);
    iRequestUri.Append(kSchemeHttps);
    iRequestUri.Append(iServer);
    iRequestUri.Append(aPathAndQuery);
    // Limit the number of entries returned by the server.
    iRequestUri.Append(Brn("&limit="));
    Bws<16> limitBuf;
    WriterBuffer limitWriter(limitBuf);
    Ascii::StreamWriteUint(limitWriter, aMaxResults);
    iRequestUri.Append(limitBuf);

    Uri uri(iRequestUri);
    IReader& reader = iLoader->Load(uri); // THROWS UriLoaderError
    // NOTE: do NOT call iLoader->Reset() here - it disconnects the socket
    // (UriLoader::Reset is only safe between Load calls, not while reading).
    ParseStations(reader, aResults, aMaxResults);
    // Reading is complete; the socket is disconnected on the next Load or by
    // the UriLoader destructor.
    iLoader->Reset();
}

void RadioBrowserClient::ParseStations(IReader& aReader, std::vector<Station>& aResults,
                                       TUint aMaxResults)
{
    // Read the whole response into memory; station lists are up to a few
    // hundred KB for 100 entries.
    // NOTE: partial reads are normal on a TCP stream; only a zero-length read
    // signals end-of-stream (per the IReader interface). Do NOT break on
    // short reads.
    std::vector<TByte> data;
    for (;;) {
        const TUint chunk = 2048;
        Brn read = aReader.Read(chunk);
        if (read.Bytes() == 0) {
            break; // end of stream
        }
        data.insert(data.end(), read.Ptr(), read.Ptr() + read.Bytes());
        if (data.size() > 4 * 1024 * 1024) {
            Log::Print("RadioBrowserClient: response exceeds 4MB cap\n");
            break; // safety cap
        }
    }

    Brn input(reinterpret_cast<const TByte*>(data.data()), data.size());
    if (input.Bytes() == 0) {
        return;
    }

    try {
        auto jsonParserArray = JsonParserArray::Create(input);
        for (;;) {
            Brn obj;
            try {
                obj = jsonParserArray.NextObject();
            }
            catch (JsonArrayEnumerationComplete&) {
                break;
            }
            auto jsonParser = JsonParser();
            jsonParser.Parse(obj);
            Station s;
            TBool haveUri = false;
            if (jsonParser.HasKey("url_resolved")) {
                Brn url(jsonParser.StringOptional("url_resolved"));
                if (url.Bytes() > 0 && url.Bytes() <= s.iUri.MaxBytes()) {
                    s.iUri.Replace(url);
                    haveUri = true;
                }
            }
            if (!haveUri && jsonParser.HasKey("url")) {
                Brn url(jsonParser.StringOptional("url"));
                if (url.Bytes() > 0 && url.Bytes() <= s.iUri.MaxBytes()) {
                    s.iUri.Replace(url);
                    haveUri = true;
                }
            }
            if (!haveUri) {
                continue; // unusable entry
            }
            if (jsonParser.HasKey("name")) {
                Brn name(jsonParser.StringOptional("name"));
                const TUint max = std::min<TUint>(name.Bytes(), s.iName.MaxBytes());
                s.iName.Replace(Brn(name.Ptr(), max));
            }
            if (jsonParser.HasKey("favicon")) {
                Brn image(jsonParser.StringOptional("favicon"));
                const TUint max = std::min<TUint>(image.Bytes(), s.iImage.MaxBytes());
                s.iImage.Replace(Brn(image.Ptr(), max));
            }
            if (jsonParser.HasKey("tags")) {
                Brn tags(jsonParser.StringOptional("tags"));
                const TUint max = std::min<TUint>(tags.Bytes(), s.iTags.MaxBytes());
                s.iTags.Replace(Brn(tags.Ptr(), max));
            }
            if (jsonParser.HasKey("countrycode")) {
                Brn country(jsonParser.StringOptional("countrycode"));
                const TUint max = std::min<TUint>(country.Bytes(), s.iCountry.MaxBytes());
                s.iCountry.Replace(Brn(country.Ptr(), max));
            }
            if (jsonParser.HasKey("codec")) {
                Brn codec(jsonParser.StringOptional("codec"));
                const TUint max = std::min<TUint>(codec.Bytes(), s.iCodec.MaxBytes());
                s.iCodec.Replace(Brn(codec.Ptr(), max));
            }
            if (jsonParser.HasKey("bitrate")) {
                try {
                    s.iBitrate = Ascii::Uint(jsonParser.StringOptional("bitrate"));
                }
                catch (AsciiError&) {}
            }
            if (jsonParser.HasKey("hls")) {
                // numeric 0/1 or bool string
                try {
                    s.iHls = (Ascii::Uint(jsonParser.StringOptional("hls")) != 0);
                }
                catch (AsciiError&) {}
            }
            if (jsonParser.HasKey("lastcheckok")) {
                try {
                    s.iLastCheckOk = (Ascii::Uint(jsonParser.StringOptional("lastcheckok")) != 0);
                }
                catch (AsciiError&) {}
            }
            if (!s.iLastCheckOk) {
                continue; // station known to be down at last check
            }
            // The radio-browser database commonly contains multiple entries
            // for the same station (the same stream submitted several times,
            // often with slightly different urls or artworks). Results are
            // popularity-ordered, so the first occurrence is the most
            // popular; skip subsequent entries resolving to the same stream.
            Bws<RadioBrowserClient::kUriMaxBytes * 2> newKey;
            AppendUriKey(newKey, s.Uri());
            TBool duplicate = false;
            for (TUint k = 0; k < aResults.size(); k++) {
                Bws<RadioBrowserClient::kUriMaxBytes * 2> existingKey;
                AppendUriKey(existingKey, aResults[k].Uri());
                if (existingKey == newKey) {
                    duplicate = true;
                    break;
                }
            }
            if (duplicate) {
                continue;
            }
            aResults.push_back(s);
            if (aResults.size() >= aMaxResults) {
                break;
            }
        }
    }
    catch (JsonInvalid&) {
        Log::Print("RadioBrowserClient: invalid JSON response\n");
        aResults.clear();
    }
    catch (JsonUnsupported&) {
        Log::Print("RadioBrowserClient: unsupported JSON response\n");
        aResults.clear();
    }
}

} // namespace Av
} // namespace OpenHome
