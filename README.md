# Description

Forked from the main branch to include support for the Audiophonics Raspdac Mini interface.

A couple of additional scripts are needed. Message me if interested.

# History

* **2026-09-04** — Synced back with AndreHro updates. Currently only used on `aarch64`, but it can easily be adapted to `armhf` if needed. Please raise an issue if support is required.

* **2025-04-19** — As `ohPlayer` is no longer maintained or aligned with the rest of the stack, the project was redesigned based on `ohPipeline/TestMediaPlayer`.

  Some build details can be found here:
  [ohContainerBuilder](https://github.com/jchassin/ohContainerBuilder)

  It can be built in an Ubuntu container or directly on a Raspberry Pi. Open a ticket if support is needed.

* **2021-10-16** — Added support for the Audiophonics Raspdac Mini LCD:
  [Audiophonics Raspdac Mini LCD Kit](https://www.audiophonics.fr/fr/lecteurs-reseau-open-source/audiophonics-raspdac-mini-lcd-kit-diy-lecteur-reseau-pour-raspberry-pi-4-dac-es9038q2m-p-15148.html)

  Initial fork added support for the Audiophonics Raspdac Mini OLED screen.

Based on the [OpenHome Sample Players](https://github.com/openhome/ohPlayer) for macOS, Windows, Linux and Raspberry Pi.

---

# Hardware Setups

Two hardware configurations are supported, selected through the web configuration UI (presentation URL of the renderer, `Linn Amp` tab):

## 1. Raspdac Mini / OLED with fixed-gain amplifier

The simplest setup: the Raspdac Mini (LCD or OLED version) is connected to a power amplifier with fixed gain, and volume is controlled by the ALSA driver of the DAC.

* Volume control: **`ALSA driver volume`**
* The Raspdac rotary encoder adjusts the ALSA volume of the DAC
* No serial connection required

```
┌───────────────┐ I2S ┌─────────────────────────┐ RCA ┌────────────────────┐ ┌──────────┐
│ Raspberry Pi 4 │────▶│ Raspdac DAC (ES9038Q2M)│────▶│ Power amp (fixed    │─▶│ Speakers │
│               │     │                         │     │ gain)               │ └──────────┘
│  ohPlayer     │     │ Raspdac Mini / OLED     │     └────────────────────┘
└───────┬───────┘     └─────────────────────────┘
        │ ▲
        │ └──────── rotary encoder / OLED (volume + display)
        └── ALSA mixer = volume control
```

Web UI settings (`Linn Amp` tab):

| Setting | Value |
|---|---|
| Volume Control | `ALSA driver volume` |
| Serial Port | *(empty)* |

## 2. Raspdac Mini / OLED with Linn preamplifier (RA2) over RS232

The Raspdac is connected to a Linn preamplifier (e.g. Akurate Kontrol / RA2) through a USB-to-RS232 adapter. The preamp handles volume, balance, bass and treble; the DAC outputs at full scale.

* Volume control: **`Linn amplifier`**
* Serial port: e.g. **`/dev/ttyUSB0`** (USB-RS232 adapter, 9600 bauds, 7E1)
* Bass / Treble: -7 to +7, applied to the preamp at startup
* Standby: the on/off button of the control point app drives the preamp standby (`$STANDBY ON/OFF$`)
* The ALSA output is set to full scale; attenuation is delegated to the preamp
* Volume is limited by the `Volume.Limit` setting (as on a Linn DSM)

```
┌───────────────┐ I2S ┌─────────────────────────┐ RCA ┌────────────────────┐ ┌──────────┐
│ Raspberry Pi 4 │────▶│ Raspdac DAC (ES9038Q2M)│────▶│ Linn preamp (RA2)  │─▶│ Speakers │
│               │     │                         │     │ power amp input    │ └──────────┘
│  ohPlayer     │     │ Raspdac Mini / OLED     │     └─────────▲──────────┘
└──────┬────────┘     └─────────────────────────┘               │
       │ USB                                                    │ zone output
       ▼                                                        │
┌──────────────────┐  RS232 (9600 bauds, 7E1)  ┌──────────────────┴─┐
│ USB-RS232 adapter│─────────────────────────▶│ RA2 control port   │
│ /dev/ttyUSB0     │   $VOL = n$, $BAL = n$,   │                    │
└──────────────────┘   $BAS/TRB$, $STANDBY$   └────────────────────┘
```

Web UI settings (`Linn Amp` tab):

| Setting | Value |
|---|---|
| Volume Control | `Linn amplifier` |
| Serial Port | `/dev/ttyUSB0` |
| Bass / Treble | -7 .. +7 |
| Detected Amplifier | `LR2` + serial (read-only, from `$ID ?$`) |

Behaviour:
* The DAC output is set to full scale; attenuation is handled by the preamp
* Volume is limited by the `Volume.Limit` setting (as on a Linn DSM)
* The standby button of the control point app drives the preamp standby

Settings are persistent and applied at the next start of the service (a reboot of the renderer, not of the Raspberry Pi, is enough: use the `Reboot` button of the web UI).

---

# Installation

## Prerequisites

Applicable to:

* `aarch64` target running Raspberry Pi OS (tested on Trixie)
* Required packages:

  * `build-essential`
  * `clang`
  * `libglib2.0-dev`
  * `libssl-dev`
  * `libasound2-dev`

Install the required packages with:

```bash
sudo apt update
sudo apt install build-essential clang libglib2.0-dev libssl-dev libasound2-dev
```

## Build

```bash
git clone https://github.com/jchassin/ohPlayer
cd ohPlayer
./fetch_deps.sh
cd linux
make
```

## Rebuild with the full OpenHome stack

For rebuilding with the full OpenHome stack (`ohNet`, etc.), see:

[ohContainerBuilder](https://github.com/jchassin/ohContainerBuilder)

## Deprecated

An Ansible playbook was created for non-Linn users (users without an account in the Linn domain):

[ohPlayer on Ansible Galaxy](https://galaxy.ansible.com/jchassin/ohplayer)
