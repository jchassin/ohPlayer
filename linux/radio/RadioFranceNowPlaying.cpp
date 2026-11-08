#include "RadioFranceNowPlaying.h"
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

static const Brn kLiveMetaBase("https://api.radiofrance.fr/livemeta/pull/");
static const TUint kRetryIntervalMs = 5 * 1000; // avoid a tight retry loop on errors

// livemeta station ids (the id_station used by embed.radiofrance.fr players)
static const struct { const TChar* kMount; TUint kId; } kStations[] = {
    { "franceinter",   1 },
    { "franceinfo",    2 },
    { "francemusique", 4 },
    { "franceculture", 5 },
    { "fip",           7 },
};

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

RadioFranceNowPlaying::RadioFranceNowPlaying(Environment& aEnv, SslContext& aSsl,
                                             const Brx& aUserAgent, OhMdpIF& aOhMdpIF)
    : iOhMdpIF(aOhMdpIF)
    , iTimerFactory(new TimerFactory(aEnv))
    , iLoader(nullptr)
    , iStop(false)
    , iStationId(0)
{
    iLoader = new Media::UriLoader(aEnv, aSsl, aUserAgent, *iTimerFactory, kRetryIntervalMs);
    iThread = std::thread(&RadioFranceNowPlaying::WorkerLoop, this);
}

RadioFranceNowPlaying::~RadioFranceNowPlaying()
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
    delete iTimerFactory;
}

void RadioFranceNowPlaying::SetStreamUri(const Brx& aUri)
{
    TUint id = (aUri.Bytes() == 0) ? 0 : StationIdFromUri(aUri);
    if (id != 0) {
        OhLog::Print("RadioFranceNowPlaying: stream '%.*s' -> livemeta station %u\n", PBUF(aUri), id);
    }
    {
        std::lock_guard<std::mutex> lock(iMutex);
        iStationId = id;
    }
    iCv.notify_all();
}

TUint RadioFranceNowPlaying::StationIdFromUri(const Brx& aUri) const
{
    try {
        Uri uri(aUri);
        const Brx& host = uri.Host();
        if (!ContainsNoCase(host, Brn("radiofrance.fr")) && !ContainsNoCase(host, Brn("fipradio.fr"))) {
            return 0;
        }
        // first path segment, e.g. "/franceinter-hifi.aac?id=radiofrance" -> "franceinter"
        Parser p(uri.PathAndQuery());
        Brn seg = p.Next('/');
        if (seg.Bytes() == 0) {
            seg = p.Next('/'); // leading slash
        }
        TUint len = 0;
        while (len < seg.Bytes() && seg[len] != '-' && seg[len] != '.' && seg[len] != '?' && seg[len] != '/') {
            len++;
        }
        seg.Set(seg.Ptr(), len);
        for (TUint i = 0; i < sizeof(kStations) / sizeof(kStations[0]); i++) {
            if (Ascii::CaseInsensitiveEquals(seg, Brn(kStations[i].kMount))) {
                return kStations[i].kId;
            }
        }
        OhLog::Print("RadioFranceNowPlaying: radiofrance stream but unknown mount '%.*s'\n", PBUF(seg));
        return 0;
    }
    catch (...) {
        return 0;
    }
}

void RadioFranceNowPlaying::WorkerLoop()
{
    TUint lastId = 0;
    for (;;) {
        TUint id;
        {
            std::unique_lock<std::mutex> lock(iMutex);
            iCv.wait(lock, [this, &id, lastId] {
                id = iStationId.load();
                return iStop.load() || id != lastId;
            });
            if (iStop.load()) {
                return;
            }
        }
        PollOnce(id);
        lastId = id;
        std::unique_lock<std::mutex> lock(iMutex);
        iCv.wait_for(lock, std::chrono::milliseconds(kPollIntervalMs),
                     [this, id] { return iStop.load() || iStationId.load() != id; });
    }
}

void RadioFranceNowPlaying::PollOnce(TUint aStationId)
{
    try {
        Bws<kUrlMaxBytes> url;
        url.Append(kLiveMetaBase);
        Bws<16> idBuf;
        WriterBuffer idWriter(idBuf);
        Ascii::StreamWriteUint(idWriter, aStationId);
        url.Append(idBuf);

        Uri uri(url);
        IReader& reader = iLoader->Load(uri); // THROWS UriLoaderError
        std::vector<TByte> data;
        for (;;) {
            Brn chunk = reader.Read(4096);
            if (chunk.Bytes() == 0) {
                break; // end of stream
            }
            data.insert(data.end(), chunk.Ptr(), chunk.Ptr() + chunk.Bytes());
            if (data.size() > kResponseMaxBytes) {
                OhLog::PrintWarning("RadioFranceNowPlaying: response exceeds %u bytes cap\n", kResponseMaxBytes);
                break;
            }
        }
        iLoader->Reset();

        if (data.empty()) {
            return;
        }
        Brn json(reinterpret_cast<const TByte*>(data.data()), data.size());
        OhLog::PrintDebug("RadioFranceNowPlaying: livemeta station %u: %.*s\n", aStationId, PBUF(json));

        Bws<kTitleMaxBytes> title;
        if (ParseLiveMeta(json, title) && title.Bytes() > 0) {
            iOhMdpIF.setTitle(Brn(title.Ptr(), title.Bytes()));
        }
    }
    catch (...) {
        OhLog::PrintWarning("RadioFranceNowPlaying: poll failed for station %u\n", aStationId);
    }
}

TBool RadioFranceNowPlaying::ParseLiveMeta(const Brx& aJson, Bwx& aTitleOut)
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
