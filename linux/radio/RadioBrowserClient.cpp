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
static const Brn kSchemeHttps("https://");
static const TUint kReadBufBytes = 64 * 1024; // station lists can be large

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
    : iEnv(aEnv)
    , iSsl(aSsl)
    , iUserAgent(aUserAgent)
    , iLoader(new Media::UriLoader(aEnv, aSsl, aUserAgent, aTimerFactory, 0 /* retry immediately */))
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
    iLoader->Reset();
    ParseStations(reader, aResults, aMaxResults);
}

void RadioBrowserClient::ParseStations(IReader& aReader, std::vector<Station>& aResults,
                                       TUint aMaxResults)
{
    // Read the whole response into memory; station lists are up to a few
    // hundred KB for 100 entries.
    std::vector<TByte> data;
    for (;;) {
        const TUint chunk = 2048;
        Brn read = aReader.Read(chunk);
        if (read.Bytes() == 0) {
            break;
        }
        data.insert(data.end(), read.Ptr(), read.Ptr() + read.Bytes());
        if (data.size() > 4 * 1024 * 1024) {
            break; // safety cap
        }
        if (read.Bytes() < chunk) {
            // short read: server closed the connection
            break;
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
                Brn url(jsonParser.String("url_resolved"));
                if (url.Bytes() > 0 && url.Bytes() <= s.iUri.MaxBytes()) {
                    s.iUri.Replace(url);
                    haveUri = true;
                }
            }
            if (!haveUri && jsonParser.HasKey("url")) {
                Brn url(jsonParser.String("url"));
                if (url.Bytes() > 0 && url.Bytes() <= s.iUri.MaxBytes()) {
                    s.iUri.Replace(url);
                    haveUri = true;
                }
            }
            if (!haveUri) {
                continue; // unusable entry
            }
            if (jsonParser.HasKey("name")) {
                Brn name(jsonParser.String("name"));
                const TUint max = std::min<TUint>(name.Bytes(), s.iName.MaxBytes());
                s.iName.Replace(Brn(name.Ptr(), max));
            }
            if (jsonParser.HasKey("favicon")) {
                Brn image(jsonParser.String("favicon"));
                const TUint max = std::min<TUint>(image.Bytes(), s.iImage.MaxBytes());
                s.iImage.Replace(Brn(image.Ptr(), max));
            }
            if (jsonParser.HasKey("tags")) {
                Brn tags(jsonParser.String("tags"));
                const TUint max = std::min<TUint>(tags.Bytes(), s.iTags.MaxBytes());
                s.iTags.Replace(Brn(tags.Ptr(), max));
            }
            if (jsonParser.HasKey("countrycode")) {
                Brn country(jsonParser.String("countrycode"));
                const TUint max = std::min<TUint>(country.Bytes(), s.iCountry.MaxBytes());
                s.iCountry.Replace(Brn(country.Ptr(), max));
            }
            if (jsonParser.HasKey("codec")) {
                Brn codec(jsonParser.String("codec"));
                const TUint max = std::min<TUint>(codec.Bytes(), s.iCodec.MaxBytes());
                s.iCodec.Replace(Brn(codec.Ptr(), max));
            }
            if (jsonParser.HasKey("bitrate")) {
                try {
                    s.iBitrate = Ascii::Uint(jsonParser.String("bitrate"));
                }
                catch (AsciiError&) {}
            }
            if (jsonParser.HasKey("hls")) {
                // numeric 0/1 or bool string
                try {
                    s.iHls = (Ascii::Uint(jsonParser.String("hls")) != 0);
                }
                catch (AsciiError&) {}
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
