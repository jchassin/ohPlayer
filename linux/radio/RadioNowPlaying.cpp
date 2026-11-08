#include "RadioNowPlaying.h"
#include "../OhLog.h"
#include "../OhMdpIF.h"

#include <OpenHome/Private/Uri.h>
#include <OpenHome/Private/Timer.h>
#include <OpenHome/Private/Ascii.h>
#include <OpenHome/Private/Parser.h>
#include <OpenHome/Private/Printer.h>
#include <OpenHome/Private/Converter.h>
#include <OpenHome/Private/Stream.h>
#include <OpenHome/Media/Protocol/ProtocolHls.h> // Media::UriLoader, UriLoaderError
#include <OpenHome/Json.h>

#include <vector>
#include <chrono>

using namespace OpenHome;
using namespace OpenHome::Av;

static const Brn kRadioFranceLiveMetaBase("https://api.radiofrance.fr/livemeta/pull/");
static const Brn kBbcRmsBase("https://rms.api.bbc.co.uk/v2/services/");
// experience=international: BBC Sounds is UK-only since 2025-07-21, the domestic
// experience is geo-restricted; international serves the ww streams
static const Brn kBbcRmsSuffix("/segments/latest?experience=international&offset=0&limit=1");
// rms.api.bbc.co.uk answers 403 to an empty/non-browser User-Agent
static const Brn kBbcUserAgent("Mozilla/5.0 (X11; Linux aarch64; rv:128.0) Gecko/20100101 Firefox/128.0");
static const TUint kRetryIntervalMs = 5 * 1000; // avoid a tight retry loop on errors

// Radio France livemeta station ids (the id_station used by embed.radiofrance.fr players)
static const struct { const TChar* kMount; TUint kId; } kRadioFranceStations[] = {
    { "franceinter",   1 },
    { "franceinfo",    2 },
    { "francemusique", 4 },
    { "franceculture", 5 },
    { "fip",           7 },
};

// out-of-line definitions: kPollIntervalMs is odr-used (passed by const& to
// std::chrono::milliseconds), so it needs a definition outside the class
const TUint RadioNowPlaying::kPollIntervalMs;
const TUint RadioNowPlaying::kResponseMaxBytes;
const TUint RadioNowPlaying::kTitleMaxBytes;
const TUint RadioNowPlaying::kUrlMaxBytes;
const TUint RadioNowPlaying::kKeyMaxBytes;

static TBool ContainsNoCase(const Brx& aHay, const Brx& aNeedle)
{
    if (aNeedle.Bytes() == 0 || aHay.Bytes() < aNeedle.Bytes()) {
        return false;
    }
    for (TUint i = 0; i + aNeedle.Bytes() <= aHay.Bytes(); i++) {
        if (Ascii::CaseInsensitiveEquals(Brn(aHay.Ptr() + i, aNeedle.Bytes()), aNeedle)) {
            return true;
        }
    }
    return false;
}

RadioNowPlaying::RadioNowPlaying(Environment& aEnv, SslContext& aSsl,
                                 const Brx& aUserAgent, OhMdpIF& aOhMdpIF)
    : iOhMdpIF(aOhMdpIF)
    , iTimerFactory(new TimerFactory(aEnv))
    , iLoader(nullptr)
    , iLoaderBbc(nullptr)
    , iStop(false)
    , iProvider(kProviderNone)
    , iGeneration(0)
{
    iLoader = new Media::UriLoader(aEnv, aSsl, aUserAgent, *iTimerFactory, kRetryIntervalMs);
    iLoaderBbc = new Media::UriLoader(aEnv, aSsl, kBbcUserAgent, *iTimerFactory, kRetryIntervalMs);
    iThread = std::thread(&RadioNowPlaying::WorkerLoop, this);
}

RadioNowPlaying::~RadioNowPlaying()
{
    {
        std::lock_guard<std::mutex> lock(iMutex);
        iStop = true;
    }
    iCv.notify_all();
    if (iThread.joinable()) {
        iThread.join();
    }
    delete iLoader;
    delete iLoaderBbc;
    delete iTimerFactory;
}

void RadioNowPlaying::SetStreamUri(const Brx& aUri)
{
    TUint provider = kProviderNone;
    Bws<kKeyMaxBytes> key;
    if (aUri.Bytes() > 0) {
        DetectProvider(aUri, provider, key);
    }
    if (provider != kProviderNone) {
        OhLog::Print("RadioNowPlaying: stream '%.*s' -> provider %u, key '%.*s'\n",
                     PBUF(aUri), provider, PBUF(key));
    }
    {
        std::lock_guard<std::mutex> lock(iMutex);
        if (provider != iProvider || !key.Equals(iKey)) {
            iProvider = provider;
            iKey.Replace(key);
            iLastTitle.SetBytes(0); // force a push on the first poll of the new station
            iGeneration++;
        }
    }
    iCv.notify_all();
}

TBool RadioNowPlaying::DetectProvider(const Brx& aUri, TUint& aProviderOut, Bws<kKeyMaxBytes>& aKeyOut) const
{
    try {
        Uri uri(aUri);
        const Brx& host = uri.Host();
        const Brx& path = uri.PathAndQuery();

        // Radio France: mount name (first path segment, cut at '-'/'.'/'?') -> livemeta id
        if (ContainsNoCase(host, Brn("radiofrance.fr")) || ContainsNoCase(host, Brn("fipradio.fr"))) {
            Parser p(path);
            Brn seg = p.Next('/');
            if (seg.Bytes() == 0) {
                seg = p.Next('/'); // leading slash
            }
            TUint len = 0;
            while (len < seg.Bytes() && seg[len] != '-' && seg[len] != '.' && seg[len] != '?' && seg[len] != '/') {
                len++;
            }
            seg.Set(seg.Ptr(), len);
            for (TUint i = 0; i < sizeof(kRadioFranceStations) / sizeof(kRadioFranceStations[0]); i++) {
                if (Ascii::CaseInsensitiveEquals(seg, Brn(kRadioFranceStations[i].kMount))) {
                    Bws<16> idBuf;
                    WriterBuffer idWriter(idBuf);
                    Ascii::StreamWriteUint(idWriter, kRadioFranceStations[i].kId);
                    aKeyOut.Replace(idBuf);
                    aProviderOut = kProviderRadioFrance;
                    return true;
                }
            }
            OhLog::Print("RadioNowPlaying: radiofrance stream but unknown mount '%.*s'\n", PBUF(seg));
            return false;
        }

        // BBC: service id = first "bbc_<service>" token anywhere in the uri
        // (path segment in akamaized/bbci.co.uk HLS urls, or ?station=bbc_... query
        // in relays like lstn.lv/bbcradio.m3u8)
        static const Brn kBbcPrefix("bbc_");
        for (TUint i = 0; i + kBbcPrefix.Bytes() <= aUri.Bytes(); i++) {
            if (Ascii::CaseInsensitiveEquals(Brn(aUri.Ptr() + i, kBbcPrefix.Bytes()), kBbcPrefix)) {
                TUint len = kBbcPrefix.Bytes();
                while (i + len < aUri.Bytes() &&
                       (Ascii::IsAlphabetic(aUri[i + len]) || Ascii::IsDigit(aUri[i + len]) || aUri[i + len] == '_')) {
                    len++;
                }
                if (len > kBbcPrefix.Bytes()) {
                    aKeyOut.Replace(Brn(aUri.Ptr() + i, len));
                    aProviderOut = kProviderBbc;
                    return true;
                }
            }
        }
        return false;
    }
    catch (...) {
        return false;
    }
}

void RadioNowPlaying::WorkerLoop()
{
    TUint lastGen = 0;
    for (;;) {
        TUint gen;
        TUint provider;
        Bws<kKeyMaxBytes> key;
        {
            std::unique_lock<std::mutex> lock(iMutex);
            iCv.wait(lock, [this, &gen, lastGen] {
                gen = iGeneration;
                return iStop.load() || gen != lastGen;
            });
            if (iStop.load()) {
                return;
            }
            provider = iProvider;
            key.Replace(iKey);
        }
        if (provider != kProviderNone) {
            PollOnce(provider, key);
        }
        lastGen = gen;
        std::unique_lock<std::mutex> lock(iMutex);
        iCv.wait_for(lock, std::chrono::milliseconds(kPollIntervalMs),
                     [this, gen] { return iStop.load() || iGeneration != gen; });
    }
}

void RadioNowPlaying::PollOnce(TUint aProvider, const Brx& aKey)
{
    if (aProvider == kProviderNone) {
        return;
    }
    try {
        Bws<kUrlMaxBytes> url;
        if (aProvider == kProviderRadioFrance) {
            url.Append(kRadioFranceLiveMetaBase);
            url.Append(aKey);
        }
        else { // kProviderBbc
            url.Append(kBbcRmsBase);
            url.Append(aKey);
            url.Append(kBbcRmsSuffix);
        }

        Uri uri(url);
        Media::UriLoader& loader = (aProvider == kProviderBbc) ? *iLoaderBbc : *iLoader;
        IReader& reader = loader.Load(uri); // THROWS UriLoaderError
        std::vector<TByte> data;
        for (;;) {
            Brn chunk = reader.Read(4096);
            if (chunk.Bytes() == 0) {
                break; // end of stream
            }
            data.insert(data.end(), chunk.Ptr(), chunk.Ptr() + chunk.Bytes());
            if (data.size() > kResponseMaxBytes) {
                OhLog::PrintWarning("RadioNowPlaying: response exceeds %u bytes cap\n", kResponseMaxBytes);
                break;
            }
        }
        loader.Reset();

        if (data.empty()) {
            return;
        }
        Brn json(reinterpret_cast<const TByte*>(data.data()), data.size());
        OhLog::PrintDebug("RadioNowPlaying: provider %u key '%.*s': %.*s\n", aProvider, PBUF(aKey), PBUF(json));

        Bws<kTitleMaxBytes> title;
        TBool ok = (aProvider == kProviderRadioFrance) ? ParseRadioFrance(json, title)
                                                        : ParseBbc(json, title);
        if (ok && title.Bytes() > 0) {
            TBool changed;
            {
                std::lock_guard<std::mutex> lock(iMutex);
                changed = !title.Equals(iLastTitle);
                if (changed) {
                    iLastTitle.Replace(title);
                }
            }
            if (changed) {
                iOhMdpIF.setTitle(Brn(title.Ptr(), title.Bytes()));
            }
        }
    }
    catch (...) {
        // UriLoader logs the HTTP status code at kMedia debug level ("UriLoader::Load code: %d")
        OhLog::PrintWarning("RadioNowPlaying: poll failed (provider %u, key '%.*s') - enable kMedia debug to see the HTTP status\n",
                            aProvider, PBUF(aKey));
    }
}

TBool RadioNowPlaying::ParseRadioFrance(const Brx& aJson, Bwx& aTitleOut)
{
    // livemeta JSON: { "levels": [ { "items": ["uid", ...], "position": N }, ... ],
    //                  "steps": { "<uid>": { "title": ..., "authors": ..., ... } } }
    // The current item is items[position] of the deepest (last) level.
    try {
        JsonParser top;
        top.Parse(aJson);
        Brn levelsRaw = top.String("levels");
        auto levels = JsonParserArray::Create(levelsRaw);
        Brn lastLevel;
        TBool any = false;
        while (levels.TryNextObject(lastLevel)) {
            any = true;
        }
        if (!any) {
            return false;
        }
        JsonParser lvl;
        lvl.Parse(lastLevel);
        TInt position = lvl.Num("position");
        Brn itemsRaw = lvl.String("items");
        auto items = JsonParserArray::Create(itemsRaw);
        Brn uid;
        TBool found = false;
        for (TInt i = 0; ; i++) {
            Brn u;
            if (!items.TryNextString(u)) {
                break;
            }
            if (i == position) {
                uid = u;
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
        Brn stepsRaw = top.String("steps");
        JsonParser steps;
        steps.Parse(stepsRaw);
        Brn stepRaw = steps.String(uid);
        JsonParser step;
        step.Parse(stepRaw);
        Brn title = step.StringOptional("title");
        Brn authors = step.StringOptional("authors");
        aTitleOut.SetBytes(0);
        if (authors.Bytes() > 0) {
            aTitleOut.Append(authors);
            aTitleOut.Append(Brn(" - "));
        }
        if (title.Bytes() > 0) {
            aTitleOut.Append(title);
        }
        return aTitleOut.Bytes() > 0;
    }
    catch (...) {
        return false;
    }
}

TBool RadioNowPlaying::ParseBbc(const Brx& aJson, Bwx& aTitleOut)
{
    // rms.api.bbc.co.uk segments/latest JSON:
    // { "data": [ { "segment_type": "music"|"programme"|...,
    //               "titles": { "primary": <artist or show>, "secondary": <track>, ... },
    //               "offset": { "now_playing": true|false, ... } }, ... ] }
    // data[0] is the most recent segment.
    try {
        JsonParser top;
        top.Parse(aJson);
        Brn dataRaw = top.String("data");
        auto arr = JsonParserArray::Create(dataRaw);
        Brn first;
        if (!arr.TryNextObject(first)) {
            return false;
        }
        JsonParser seg;
        seg.Parse(first);
        Brn segType = seg.StringOptional("segment_type");
        Brn titlesRaw = seg.String("titles");
        JsonParser titles;
        titles.Parse(titlesRaw);
        Brn primary = titles.StringOptional("primary");
        Brn secondary = titles.StringOptional("secondary");
        aTitleOut.SetBytes(0);
        if (segType.Equals(Brn("music")) && primary.Bytes() > 0 && secondary.Bytes() > 0) {
            aTitleOut.Append(primary);
            aTitleOut.Append(Brn(" - "));
            aTitleOut.Append(secondary);
        }
        else if (primary.Bytes() > 0) {
            aTitleOut.Append(primary); // programme/show title
        }
        return aTitleOut.Bytes() > 0;
    }
    catch (...) {
        return false;
    }
}
