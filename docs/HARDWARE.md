# Hardware Setups

Two hardware configurations are supported, selected through the web configuration UI (presentation URL of the renderer, `Linn Amp` tab):

Hardware references:

* [Audiophonics Raspdac Mini LCD Kit](https://www.audiophonics.fr/fr/lecteurs-reseau-open-source/audiophonics-raspdac-mini-lcd-kit-diy-lecteur-reseau-pour-raspberry-pi-4-dac-es9038q2m-p-15148.html)
* [Audiophonics DAC I-Sabre ES9038Q2M](https://www.audiophonics.fr/fr/modules-dac-et-interfaces/audiophonics-dac-i-sabre-es9038q2m-raspberry-pi-i2s-spdif-pcm-dsd-alimentation-usb-c-p-12795.html) — tested with the ES9028 version, which has since been replaced by this one.
* [Linn RS232 specification (LR2)](https://docs.linn.co.uk/wiki/images/2/2d/LR2_RS232_Specification.pdf) — the serial protocol spoken to the Linn preamplifier (volume, bass/treble, standby, `$ID ?$`)
* [Linn RS232 cables](https://docs.linn.co.uk/wiki/images/1/13/RS232_Cables_v102.pdf) — cabling reference for the USB-to-RS232 adapter

## 1. Raspdac Mini / OLED with fixed-gain amplifier

The simplest setup: the Raspdac Mini (LCD or OLED version) is connected to a power amplifier with fixed gain, and volume is controlled by the ALSA driver of the DAC.

* Volume control: **internal (ALSA driver)** — `Internal Volume Control = Yes`
* The Raspdac rotary encoder adjusts the ALSA volume of the DAC
* No serial connection required

![Setup 1 — fixed-gain amplifier](setup1-fixed-gain.svg)

Web UI settings (`Volume` and `Linn Amp` tabs):

| Setting | Value |
|---|---|
| Internal Volume Control | `Yes` |
| Serial Port | *(empty)* |

## 2. Raspdac Mini / OLED with Linn preamplifier (RA2) over RS232

The Raspdac is connected to a Linn preamplifier (e.g. Akurate Kontrol / RA2) through a USB-to-RS232 adapter. The preamp handles volume, balance, bass and treble; the DAC outputs at full scale.

* Volume control: **external Linn amplifier** — `Internal Volume Control = No` with a serial port configured
* Serial port: e.g. **`/dev/ttyUSB0`** (USB-RS232 adapter, 9600 bauds, 7E1)
* Bass / Treble: -7 to +7, applied to the preamp at startup
* Standby: the on/off button of the control point app drives the preamp standby (`$STANDBY ON/OFF$`)
* Volume is limited by the `Volume.Limit` setting (as on a Linn DSM)

![Setup 2 — Linn preamp over RS232](setup2-linn-preamp.svg)

Web UI settings (`Volume` and `Linn Amp` tabs):

| Setting | Value |
|---|---|
| Internal Volume Control | `No` (read-only while the amplifier is active) |
| Serial Port | `/dev/ttyUSB0` |
| Bass / Treble | -7 .. +7 |
| ALSA Volume | 0 .. 100 (output level of the DAC; attenuation handled by the preamp) |
| Detected Amplifier | `LR2` + serial (read-only, from `$ID ?$`) |

Behaviour:
* The DAC output is set to full scale; attenuation is handled by the preamp
* Volume is limited by the `Volume.Limit` setting (as on a Linn DSM)
* The standby button of the control point app drives the preamp standby

Settings are persistent and applied at the next start of the service (a reboot of the renderer, not of the Raspberry Pi, is enough: use the `Reboot` button of the web UI).
