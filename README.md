# Description

Forked from the main branch to include support for the Audiophonics Raspdac Mini interface.

A full (software) OpenHome media player for Raspberry Pi, controllable from any OpenHome control point (Linn app, Kazoo, BubbleUPnP, ...). It plays local files, UPnP media servers (e.g. MinimServer), internet radio (presets and the integrated [Radio Browser](docs/RADIO.md)), and supports the Audiophonics Raspdac Mini front panel.

## Documentation

* [Radio](docs/RADIO.md) — internet radio: presets and the Radio Browser media server (radio-browser.info)
* [Hardware setups](docs/HARDWARE.md) — Raspdac Mini configurations, internal vs Linn amplifier volume routing
* [Installation & build](docs/BUILD.md) — prerequisites, build, dependencies
* [History](docs/HISTORY.md) — change log

## Quick facts

* Target: `aarch64` (Raspberry Pi OS, tested on Trixie)
* Web configuration ui on the renderer's presentation url
* Two hardware setups: fixed-gain amplifier (ALSA volume) or Linn preamplifier (RA2) over RS232
* Based on [ohPipeline](https://github.com/openhome/ohPipeline) / [ohNet](https://github.com/openhome/ohNet); forked from the [OpenHome sample players](https://github.com/openhome/ohPlayer)

## License

Licensed under the [MIT License](MitLicense.txt), copyright (c) 2015 OpenHome Ltd. See [License.txt](License.txt) for separate terms applying to bundled third-party files, and [linux/copyright](linux/copyright) for the Debian copyright file.
