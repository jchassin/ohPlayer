#!/bin/bash
PLATFORM="aarch64-scarthgap-linux"
DEBUG_MODE="Release"
DEPS_DIR="./dependencies/${PLATFORM}"

HOST="https://linn-artifacts-public.s3-eu-west-1.amazonaws.com"
DROP="artifacts"

mkdir -p "$DEPS_DIR"

declare -A DEPS
DEPS[ohNet]="1.42.6276"
DEPS[ohWafHelpers]="0.2.176"
DEPS[ohMediaPlayer]="1.172.1253"
DEPS[ohNetGenerated]="1.1.260"

declare -A PLATFORM_MAP
PLATFORM_MAP[ohNet]="aarch64-scarthgap-linux-${DEBUG_MODE}"
PLATFORM_MAP[ohWafHelpers]=""
PLATFORM_MAP[ohMediaPlayer]="aarch64-scarthgap-linux-${DEBUG_MODE}"
PLATFORM_MAP[ohNetGenerated]="AnyPlatform-${DEBUG_MODE}"  # headers-only package (Generated/Cp* and Dv*); the aarch64 lib comes from the ohContainerBuilder-built package

for NAME in "${!DEPS[@]}"; do
    VERSION=${DEPS[$NAME]}
    echo "Downloading ${NAME} v${VERSION}..."
    MAP_VAL=${PLATFORM_MAP[$NAME]}
    
    if [ -z "$MAP_VAL" ]; then
        # For completely clean, platform-independent packages like ohWafHelpers
        URL="${HOST}/${DROP}/${NAME}/${NAME}-${VERSION}.tar.gz"
    else
        # For standard platform-specific packages and the AnyPlatform tracker tag
        URL="${HOST}/${DROP}/${NAME}/${NAME}-${VERSION}-${MAP_VAL}.tar.gz"
    fi
    
    echo "Target URL: $URL"
    
    # Create the target directory path safely
    TARGET_PATH="${DEPS_DIR}/${NAME}"
    mkdir -p "$TARGET_PATH"
    
    curl -sSL "$URL" | tar -xz -C "$TARGET_PATH" --strip-components=1 2>/dev/null
    
    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        echo "ERROR: Failed to download or parse ${NAME} from server repository node!"
    else
        echo "Successfully downloaded and unpacked ${NAME} into position."
    fi
    echo "--------------------------------------------------------"
done

# ----------------------------------------------------------------------------
# Generated ohNet headers (Cp* proxies and Dv* providers) are not shipped in
# the S3 packages for this platform. Fetch them from the openhome/ohNetGenerated
# GitHub repository at the tag matching the version above and install them
# flat into ${DEPS_DIR}/Generated/, which is on the compiler include path
# (-I$(DEPS_DIR) resolves #include <Generated/...>).
# Only headers are needed: the corresponding .cpp files are already compiled
# into the ohNetGenerated library.
OHNG_GITHUB_TAG="ohNetGenerated_${DEPS[ohNetGenerated]}"
OHNG_TARBALL="https://github.com/openhome/ohNetGenerated/archive/refs/tags/${OHNG_GITHUB_TAG}.tar.gz"
OHNG_TMP="$(mktemp -d)"

echo "Downloading ohNetGenerated sources (${OHNG_GITHUB_TAG}) for Generated headers..."
echo "Target URL: $OHNG_TARBALL"

curl -sSL "$OHNG_TARBALL" | tar -xz -C "$OHNG_TMP" 2>/dev/null
if [ ${PIPESTATUS[0]} -ne 0 ]; then
    echo "ERROR: Failed to download ohNetGenerated sources from GitHub!"
else
    OHNG_SRC=$(find "$OHNG_TMP" -maxdepth 1 -mindepth 1 -type d | head -1)
    mkdir -p "${DEPS_DIR}/Generated"
    cp "${OHNG_SRC}"/OpenHome/Net/ControlPoint/Proxies/*.h "${DEPS_DIR}/Generated/" 2>/dev/null
    cp "${OHNG_SRC}"/OpenHome/Net/Device/Providers/*.h "${DEPS_DIR}/Generated/" 2>/dev/null
    HEADER_COUNT=$(ls "${DEPS_DIR}/Generated"/*.h 2>/dev/null | wc -l)
    echo "Installed ${HEADER_COUNT} Generated headers into ${DEPS_DIR}/Generated/"
fi
rm -rf "$OHNG_TMP"
