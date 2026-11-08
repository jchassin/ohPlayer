#include "RadioNowPlayingTest.h"
#include "RadioNowPlaying.h"
#include "../OhLog.h"

#include <OpenHome/Private/Network.h> // SslContext
#include <OpenHome/Private/Printer.h>

using namespace OpenHome;
using namespace OpenHome::Av;

// Test stations: Radio France (livemeta), BBC (rms segments/broadcasts), and one
// ICY-only station (Couleur 3) as a control - it has no API provider, its metadata
// comes from the stream itself (NotifyMetaText).
static const struct { const TChar* kName; const TChar* kUri; } kStations[] = {
    { "France Inter (icecast aac)", "https://icecast.radiofrance.fr/franceinter-hifi.aac" },
    { "FIP (icecast aac)",          "https://icecast.radiofrance.fr/fip-hifi.aac" },
    { "France Musique (hls)",       "https://stream.radiofrance.fr/francemusique/francemusique-hifi.m3u8?id=radiofrance" },
    { "FIP (hls)",                  "https://stream.radiofrance.fr/fip/fip_hifi.m3u8?id=radiofrance" },
    { "BBC Radio 1 (hls ww)",       "https://as-hls-ww-live.akamaized.net/pool_01505109/live/ww/bbc_radio_one/bbc_radio_one.isml/bbc_radio_one-audio=320000.norewind.m3u8" },
    { "BBC Radio 2 (hls ww)",       "https://as-hls-ww-live.akamaized.net/pool_74208725/live/ww/bbc_radio_two/bbc_radio_two.isml/bbc_radio_two-audio=320000.norewind.m3u8" },
    { "BBC Radio 4 FM (hls ww)",    "http://as-hls-ww-live.akamaized.net/pool_55057080/live/ww/bbc_radio_fourfm/bbc_radio_fourfm.isml/bbc_radio_fourfm-audio=96000.norewind.m3u8" },
    { "Couleur 3 (icy control)",    "http://stream.srg-ssr.ch/m/couleur3/mp3_128" },
    // Spanish national stations: commercial ones are ICY mp3/aac (no API provider
    // expected - their metadata comes from the stream via NotifyMetaText);
    // RTVE is HLS with no in-stream metadata either (verified: empty EXTINF, no ID3).
    { "Cadena SER (es icy)",  "https://playerservices.streamtheworld.com/api/livestream-redirect/CADENASER.mp3" },
    { "COPE (es icy)",        "https://flucast09-h-cloud.flumotion.com/cope/net1.mp3" },
    { "Los 40 (es icy)",      "https://playerservices.streamtheworld.com/api/livestream-redirect/LOS40.mp3" },
    { "Kiss FM (es icy)",     "https://kissfm.kissfmradio.cires21.com/kissfm.mp3" },
    { "esRadio (es icy)",     "https://libertaddigital-radio-live1.flumotion.com/libertaddigital/ld-live1-high.mp3" },
    { "Onda Cero (es icy)",   "https://radio-atres-live.ondacero.es/api/livestream-redirect/OCAAC.aac" },
    { "Radio 3 RNE (es hls)", "https://rtvelivestream.rtve.es/rtvesec/rne/rne_r3_main.m3u8" },
};

void Av::TestRadioNowPlaying(Environment& aEnv, SslContext& aSsl, const Brx& aUserAgent)
{
    // nullptr OhMdpIF: probe only, nothing is pushed to the OLED interface
    RadioNowPlaying prober(aEnv, aSsl, aUserAgent, nullptr);

    const TUint count = sizeof(kStations) / sizeof(kStations[0]);
    OhLog::Print("=== Radio now-playing endpoint probe: %u stations ===\n", count);
    for (TUint i = 0; i < count; i++) {
        const Brn uri(kStations[i].kUri);
        OhLog::Print("-- %s\n   %.*s\n", kStations[i].kName, PBUF(uri));
        TUint provider = RadioNowPlaying::kProviderNone;
        Bws<RadioNowPlaying::kKeyMaxBytes> key;
        Bws<RadioNowPlaying::kTitleMaxBytes> title;
        TBool ok = prober.Probe(uri, provider, key, title);
        if (provider == RadioNowPlaying::kProviderNone) {
            OhLog::Print("   => no API provider (ICY/in-stream metadata expected instead)\n");
        }
        else if (!ok) {
            OhLog::Print("   => provider %u key '%.*s': POLL/PARSE FAILED\n", provider, PBUF(key));
        }
        else {
            OhLog::Print("   => provider %u key '%.*s' title='%.*s'\n", provider, PBUF(key), PBUF(title));
        }
    }
    OhLog::Print("=== probe done ===\n");
}
