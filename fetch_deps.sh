#!/bin/bash
PLATFORM="aarch64-scarthgap-linux"
DEBUG_MODE="Release"
DEPS_DIR="./dependencies/${PLATFORM}"

HOST="https://linn-artifacts-public.s3-eu-west-1.amazonaws.com"
DROP="artifacts"

mkdir -p "$DEPS_DIR"

declare -A DEPS
DEPS[ohNet]="1.42.6245"
DEPS[ohWafHelpers]="0.2.175"
DEPS[ohMediaPlayer]="1.172.1251"
DEPS[ohNetGenerated]="1.1.260"

declare -A PLATFORM_MAP
PLATFORM_MAP[ohNet]="aarch64-scarthgap-linux-${DEBUG_MODE}"
PLATFORM_MAP[ohWafHelpers]=""
PLATFORM_MAP[ohMediaPlayer]="aarch64-scarthgap-linux-${DEBUG_MODE}"
PLATFORM_MAP[ohNetGenerated]="AnyPlatform"

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
