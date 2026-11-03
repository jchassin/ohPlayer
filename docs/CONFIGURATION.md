# Configuration

Settings live in the web configuration ui, available on the renderer's presentation url (the `<device ip>` presentation link advertised via UPnP, port 8080 by default). Changes to pipeline buffers are applied at the next start.

## Pipeline buffers

The buffers and how they interact with Songcast multiroom are drawn in the [audio chain diagram](audio-chain.svg) — see it first for the full picture: what the sender announces, what the pipeline compensates, and what is actually heard as the inter-room delay.

### Starvation Ramper Min Size (`Pipeline.StarvationRamperMinSize`, in jiffies — 56448 per ms)

Minimum amount of buffered decoded audio in the downstream reservoir; the last reserve before a dropout when the network cuts. It is part of the downstream latency together with the Driver ALSA Buffer.

**Multiroom rule:** the Songcast sender announces a media latency target in every packet — 150 ms from a Linn DS, 300 ms from an ohPlayer sender. The pipeline can compensate the downstream buffers (ramper + ALSA) up to that target but cannot recover an overshoot: above it, the receiver lags the sender permanently. Keep the sum **below the sender's target**. A runtime warning is logged when the configured sum exceeds the target observed in the live stream.

* Range: 20 ms to 280 ms (280 stays below the 300 ms ohPlayer sender target; with a Linn DS sender, keep the sum below 150 ms)
* Default: 100 ms
* Measured inter-room offsets: ramper 200 ms → 62 ms lag; ramper 85 ms + ALSA 12 ms → ~5 ms (below audibility)

### Driver ALSA Buffer (`Pipeline.AlsaBufferUs`, in microseconds)

ALSA hardware buffer of the audio driver, included in `snd_pcm_delay` (I2S fifo included). Part of the downstream latency — see the rule above. A smaller buffer reduces the multiroom offset but increases the risk of xruns (audio glitches) under CPU load.

* Range: 5000 to 50000 µs
* Default: 12052 µs (12 ms) — validated by metronome measurements

### Encoded Reservoir Size (`Pipeline.EncodedReservoirSize`, in bytes)

Buffer between the network and the decoder. Note: it only fills for sources that deliver faster than real time (http radio bursts, local files); a Songcast stream is paced at real time and keeps it near empty, so increasing it does not protect against Songcast dropouts — the Starvation Ramper and the decoded reservoir absorb network cuts there.

* Range: 64 KB to 8 MB
* Default: 1572864 bytes (1536 KB — the ohPipeline default)

## Songcast

The receiver source plays streams sent by a Songcast sender (a Linn DS or another ohPlayer instance). The sender announces its latency target (see above); the `SongcastLatencyLogger` traces it at each change in the logs:

```
Songcast: sender media latency = 1843200 (songcast time) = 8467200 jiffies (~150ms) @ 48000Hz
```

The same trace carries the overshoot warning when the configured downstream sum exceeds the live target.
