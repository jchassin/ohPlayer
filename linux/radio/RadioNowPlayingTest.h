#pragma once

#include <OpenHome/Types.h>
#include <OpenHome/Buffer.h>

namespace OpenHome {
    class Environment;
    class SslContext;
namespace Av {

// Scans a list of test radio streams, probes each one's now-playing endpoint
// (RadioNowPlaying::Probe: provider detection + one poll + parse) and prints
// the result per station. Run with --test-radio-meta.
void TestRadioNowPlaying(Environment& aEnv, SslContext& aSsl, const Brx& aUserAgent);

} // namespace Av
} // namespace OpenHome
