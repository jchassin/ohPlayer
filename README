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

# Installation

## Prerequisites

Applicable to:

* `aarch64` target running Raspberry Pi OS (tested on Trixie)
* Required packages:

  * `build-essential`
  * `clang`
  * `libglib2.0-dev`
  * `libssl-dev`

Install the required packages with:

```bash
sudo apt update
sudo apt install build-essential clang libglib2.0-dev libssl-dev
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
