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

## Package (apt / .deb)

```bash
cd linux
make deb                          # or: ./make-apt-package.sh --version 1.0.7 --output dist
sudo apt install ./dist/openhome-player_<version>_<arch>.deb
```

`make-apt-package.sh` builds the renderer, stages it with `make install DESTDIR=...`
(binary in `/usr/bin`, web UI in `/usr/share/openhome-player/res/`, systemd unit
`ohPlayer.service`), computes the runtime `Depends:` from the staged binary
(`ldd` + `dpkg -S`), writes the `DEBIAN/control` + `postinst`/`postrm` files and
packs everything with `dpkg-deb`. The version defaults to `git describe` (tag
`release-X` stripped). Use `--no-build` to package an already built binary and
`--extra-depends "pkg1, pkg2"` to add manual dependencies.

## Rebuild with the full OpenHome stack

For rebuilding with the full OpenHome stack (`ohNet`, etc.), see:

[ohContainerBuilder](https://github.com/jchassin/ohContainerBuilder)

## Deprecated

An Ansible playbook was created for non-Linn users (users without an account in the Linn domain):

[ohPlayer on Ansible Galaxy](https://galaxy.ansible.com/jchassin/ohplayer)
