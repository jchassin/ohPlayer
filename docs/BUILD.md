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

`fetch_deps.sh` downloads the prebuilt dependency packages (ohNet, ohMediaPlayer, ohNetGenerated) and additionally fetches the `Generated/` ohNet headers and device-provider sources from the [ohNetGenerated](https://github.com/openhome/ohNetGenerated) GitHub tag matching the configured version (the prebuilt library does not ship the UPnP MediaServer providers; the Radio Browser needs them).

## Rebuild with the full OpenHome stack

For rebuilding with the full OpenHome stack (`ohNet`, etc.), see:

[ohContainerBuilder](https://github.com/jchassin/ohContainerBuilder)

## Deprecated

An Ansible playbook was created for non-Linn users (users without an account in the Linn domain):

[ohPlayer on Ansible Galaxy](https://galaxy.ansible.com/jchassin/ohplayer)
