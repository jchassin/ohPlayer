#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace OpenHome {
    class Environment;
    class SslContext;
    class TimerFactory;
    class OhMdpIF;
namespace Media {
    class UriLoader;
}
namespace Av {

// Polls public, keyless "now playing" JSON endpoints for radio streams that
// carry no in-stream metadata (no ICY StreamTitle, and ohPipeline does not
// surface HLS EXTINF/ID3 timed metadata, so NotifyMetaText never fires):
//  - Radio France (hosts radiofrance.fr / fipradio.fr):
//      https://api.radiofrance.fr/livemeta/pull/<stationId>
//  - BBC (BBC HLS URLs carry the service id in the path, e.g.
//    .../live/ww/bbc_radio_one/bbc_radio_one.isml/...m3u8):
//      https://rms.api.bbc.co.uk/v2/services/<serviceId>/segments/latest?experience=domestic&offset=0&limit=1
// The current show/song title is pushed to the OLED interface (OhMdpIF::setTitle).
//
// Usage: call SetStreamUri() on every pipeline track change (and with an empty
// uri when playback stops). A worker thread polls the endpoint periodically
// while the current stream maps to a known provider.
class RadioNowPlaying
{
public:
    static const TUint kPollIntervalMs = 15 * 1000;
    static const TUint kResponseMaxBytes = 256 * 1024;
    static const TUint kTitleMaxBytes = 512;
    static const TUint kUrlMaxBytes = 320;
    static const TUint kKeyMaxBytes = 64;
    enum Provider : TUint { kProviderNone = 0, kProviderRadioFrance = 1, kProviderBbc = 2 };
public:
    RadioNowPlaying(Environment& aEnv, SslContext& aSsl, const Brx& aUserAgent,
                    OhMdpIF* aOhMdpIF); // may be nullptr (test/probe mode: no OLED push)
    ~RadioNowPlaying();

    // aUri: current track uri (empty to stop polling).
    void SetStreamUri(const Brx& aUri);

    // One-shot probe (used by the --test-radio-meta scan): detect the provider
    // for aUri, poll its endpoint once, and return the title it would display.
    // Returns false when the uri maps to no known provider or the poll fails.
    TBool Probe(const Brx& aUri, TUint& aProviderOut, Bws<kKeyMaxBytes>& aKeyOut,
                Bws<kTitleMaxBytes>& aTitleOut);
private:
    void WorkerLoop();
    void PollOnce(TUint aProvider, const Brx& aKey);
    TBool FetchTitle(TUint aProvider, const Brx& aKey, Bws<kTitleMaxBytes>& aTitleOut);
    TBool FetchJson(const Brx& aUrl, Media::UriLoader& aLoader, std::vector<TByte>& aDataOut);
    TBool DetectProvider(const Brx& aUri, TUint& aProviderOut, Bws<kKeyMaxBytes>& aKeyOut) const;
    TBool ParseRadioFrance(const Brx& aJson, Bwx& aTitleOut);
    TBool ParseBbcSegments(const Brx& aJson, Bwx& aTitleOut);
    TBool ParseBbcBroadcast(const Brx& aJson, Bwx& aTitleOut);
private:
    OhMdpIF* iOhMdpIF;
    TimerFactory* iTimerFactory;
    Media::UriLoader* iLoader;    // player user-agent (Radio France)
    Media::UriLoader* iLoaderBbc; // browser user-agent (BBC rms rejects empty UA)
    std::thread iThread;
    std::mutex iMutex;
    std::condition_variable iCv;
    std::atomic<TBool> iStop;
    TUint iProvider;            // guarded by iMutex
    TUint iGeneration;          // guarded by iMutex, bumped on every target change
    Bws<kKeyMaxBytes> iKey;     // guarded by iMutex: RF station id or BBC service id
    Bws<kTitleMaxBytes> iLastTitle; // guarded by iMutex: skip redundant setTitle
};

} // namespace Av
} // namespace OpenHome
