#!/usr/bin/env bash
# ============================================================================
# make-apt-package.sh - build an apt (.deb) package for openhome-player
#
# Usage:
#   ./make-apt-package.sh [--version <ver>] [--output <dir>] [--no-build]
#                         [--extra-depends "pkg1, pkg2"]
#
# Steps:
#   1. build the renderer (make), unless --no-build
#   2. stage the files with `make install DESTDIR=<stage>` (binary, web ui
#      resources, icons, copyright), normalised under usr/
#   3. add the systemd unit and the DEBIAN control files (control/postinst/
#      postrm)
#   4. compute runtime dependencies from the staged binary (ldd + dpkg -S)
#   5. pack with dpkg-deb
#
# Example:
#   ./make-apt-package.sh --version 1.0.7 --output dist
#   sudo apt install ./dist/openhome-player_1.0.7_arm64.deb
# ============================================================================
set -euo pipefail

cd "$(dirname "$0")"   # linux/

PKG_NAME=openhome-player
VERSION=""
OUTPUT_DIR="dist"
BUILD=1
EXTRA_DEPS=""

usage() {
    sed -n '2,25p' "$0" | sed 's/^# \{0,1\}//'
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version)       VERSION="$2"; shift 2 ;;
        --output)        OUTPUT_DIR="$2"; shift 2 ;;
        --no-build)      BUILD=0; shift ;;
        --extra-depends) EXTRA_DEPS="$2"; shift 2 ;;
        -h|--help)       usage; exit 0 ;;
        *) echo "unknown option: $1" >&2; usage; exit 1 ;;
    esac
done

# Default version from git: 'release-1.0.6-9-gb702156' -> '1.0.6-9-gb702156'
# (a debian version must start with a digit).
if [[ -z "$VERSION" ]]; then
    VERSION=$(git describe --tags --always --dirty 2>/dev/null \
              | sed -e 's/^release-//' -e 's/^[^0-9]*//' || true)
    [[ -z "$VERSION" ]] && VERSION="0.0.0"
fi
ARCH=$(dpkg --print-architecture)

STAGE=$(mktemp -d /tmp/ohp-deb.XXXXXX)
trap 'rm -rf "$STAGE"' EXIT

echo "==> Package: $PKG_NAME $VERSION ($ARCH)"

if [[ $BUILD -eq 1 ]]; then
    echo "==> Building"
    make -j"$(nproc)"
fi

echo "==> Staging files into $STAGE"
make install DESTDIR="$STAGE"

# `make install` honours DESTDIR literally (BINDIR=/bin, RESDIR=/share/...):
# normalise to a FHS layout under usr/ so the deb installs /usr/bin and
# /usr/share (the runtime expects /usr/share/openhome-player/res/).
mkdir -p "$STAGE/usr"
for d in bin share lib; do
    if [[ -d "$STAGE/$d" ]]; then
        mkdir -p "$STAGE/usr/$d"
        mv "$STAGE/$d"/* "$STAGE/usr/$d/"
        rmdir "$STAGE/$d"
    fi
done

# systemd unit, with the packaged binary path and a system store location
install -m 755 -d "$STAGE/usr/lib/systemd/system"
sed -e 's|^ExecStart=.*|ExecStart=/usr/bin/openhome-player -s /var/lib/openhome-player/ohPersist --webui 8080|' \
    ../ohPlayer.service > "$STAGE/usr/lib/systemd/system/ohPlayer.service"

echo "==> Computing runtime dependencies (ldd + dpkg -S)"
BIN="$STAGE/usr/bin/$PKG_NAME"
[[ -x $BIN ]] || { echo "error: $BIN not staged (run without --no-build or build first)" >&2; exit 1; }

DEP_LIST=$(ldd "$BIN" 2>/dev/null | awk '/=> \// {print $3}' | sort -u | while read -r lib; do
    pkg=$(dpkg -S "$lib" 2>/dev/null | grep -v '^diversion' | head -1 | cut -d: -f1)
    [[ -z $pkg ]] && continue
    ver=$(dpkg-query -W -f='${Version}' "$pkg" 2>/dev/null || true)
    if [[ -n $ver ]]; then echo "$pkg (>= $ver)"; else echo "$pkg"; fi
done | sort -u | paste -sd, -)

if [[ -n $EXTRA_DEPS ]]; then
    DEP_LIST="${DEP_LIST:+$DEP_LIST, }$EXTRA_DEPS"
fi
[[ -z $DEP_LIST ]] && DEP_LIST="libc6"
echo "    Depends: $DEP_LIST"

INSTALLED_SIZE=$(du -sk "$STAGE" | cut -f1)

echo "==> Writing DEBIAN control files"
install -m 755 -d "$STAGE/DEBIAN"
cat > "$STAGE/DEBIAN/control" <<EOF
Package: $PKG_NAME
Version: $VERSION
Architecture: $ARCH
Maintainer: OpenHome <info@openhomelabs.com>
Installed-Size: $INSTALLED_SIZE
Depends: $DEP_LIST
Section: sound
Priority: optional
Homepage: https://github.com/jchassin/ohPlayer
Description: OpenHome UPnP/DLNA media renderer (ohPlayer)
 ohPlayer is a minimalist OpenHome renderer for the Raspberry Pi: UPnP/DLNA
 render point with web configuration UI, optional Linn amplifier control
 over RS232 and radio (Radio Browser) support.
EOF

cat > "$STAGE/DEBIAN/postinst" <<'EOF'
#!/bin/sh
set -e
if [ "$1" = "configure" ]; then
    mkdir -p /var/lib/openhome-player
    systemctl daemon-reload || true
    systemctl enable ohPlayer.service || true
fi
EOF

cat > "$STAGE/DEBIAN/postrm" <<'EOF'
#!/bin/sh
set -e
systemctl daemon-reload || true
if [ "$1" = "purge" ]; then
    systemctl disable ohPlayer.service || true
    rm -rf /var/lib/openhome-player
fi
EOF
chmod 755 "$STAGE/DEBIAN/postinst" "$STAGE/DEBIAN/postrm"

echo "==> Packing"
chmod 755 "$STAGE"   # mktemp creates it 700; the package root must stay 755
mkdir -p "$OUTPUT_DIR"
OUT="$OUTPUT_DIR/${PKG_NAME}_${VERSION}_${ARCH}.deb"
dpkg-deb --root-owner-group --build "$STAGE" "$OUT"

echo "==> Built $OUT"
echo "    Install with: sudo apt install $OUT"
