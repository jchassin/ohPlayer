#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace OpenHome {
    class Environment;
    class SslContext;
    class TimerFactory;
    class OhMdpIF;
namespace Media {
    class UriLoader;
}
namespace Av {

// Polls Radio France's public "livemeta" endpoint
// (https://api.radiofrance.fr/livemeta/pull/<stationId>) for stations whose
// audio stream carries no ICY / in-band metadata: the Icecast .aac mounts of
// radiofrance.fr / fipradio.fr do not send icy-metaint, and ohPipeline does not
// surface HLS timed metadata - so NotifyMetaText never fires for them.
// The current show/song title is pushed to the OLED interface (OhMdpIF::setTitle).
//
// Usage: call SetStreamUri() on every pipeline track change (and with an empty
// uri when playback stops). A worker thread polls the endpoint periodically
// while the current stream maps to a known Radio France station.
class RadioFranceNowPlaying
{
public:
    static const TUint kPollIntervalMs = 15 * 1000;
    static const TUint kResponseMaxBytes = 256 * 1024;
    static const TUint kTitleMaxBytes = 512;
    static const TUint kUrlMaxBytes = 256;
public:
    RadioFranceNowPlaying(Environment& aEnv, SslContext& aSsl, const Brx& aUserAgent,
                         OhMdpIF& aOhMdpIF);
    ~RadioFranceNowPlaying();

    // aUri: current track uri (empty to stop polling).
    void SetStreamUri(const Brx& aUri);
private:
    void WorkerLoop();
    void PollOnce(TUint aStationId);
    TUint StationIdFromUri(const Brx& aUri) const; // 0 = not a known Radio France stream
    TBool ParseLiveMeta(const Brx& aJson, Bwx& aTitleOut);
private:
    OhMdpIF& iOhMdpIF;
    TimerFactory* iTimerFactory;
    Media::UriLoader* iLoader;
    std::thread iThread;
    std::mutex iMutex;
    std::condition_variable iCv;
    std::atomic<TBool> iStop;
    std::atomic<TUint> iStationId;
};

} // namespace Av
} // namespace OpenHome
