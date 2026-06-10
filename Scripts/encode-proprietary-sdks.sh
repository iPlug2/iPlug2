#!/bin/bash
# Creates base64-encoded tarballs of proprietary SDKs for GitHub Actions secrets
# Usage: ./encode-proprietary-sdks.sh
#
# Output files can be used as GitHub repository secrets:
#   VST2_SDK_B64 -> contents of vst2_sdk.b64
#   AAX_SDK_B64  -> contents of aax_sdk.b64

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SDK_DIR="$REPO_ROOT/Dependencies/IPlug"
OUTPUT_DIR="$REPO_ROOT"

# Detect base64 command options (macOS vs GNU)
if base64 --help 2>&1 | grep -q '\-w'; then
    B64_OPTS="-w 0"  # GNU base64: no line wrapping
else
    B64_OPTS=""      # macOS base64: no wrapping by default
fi

encode_sdk() {
    local sdk_name="$1"
    local output_file="$2"
    shift 2
    local excludes=("$@")
    local sdk_path="$SDK_DIR/$sdk_name"

    if [ ! -d "$sdk_path" ]; then
        echo "Warning: $sdk_path not found, skipping"
        return 1
    fi

    echo "Encoding $sdk_name..."
    local original_size=$(du -sh "$sdk_path" | cut -f1)

    # Build exclude arguments
    local tar_excludes=""
    for excl in "${excludes[@]}"; do
        tar_excludes="$tar_excludes --exclude=$excl"
    done

    tar -czf - -C "$SDK_DIR" $tar_excludes "$sdk_name" | base64 $B64_OPTS > "$OUTPUT_DIR/$output_file"

    local encoded_size=$(du -sh "$OUTPUT_DIR/$output_file" | cut -f1)
    echo "  Original: $original_size -> Encoded: $encoded_size"
    echo "  Output: $OUTPUT_DIR/$output_file"
}

echo "Creating base64-encoded SDK tarballs for GitHub Actions..."
echo ""

encode_sdk "VST2_SDK" "vst2_sdk.b64"

# AAX_SDK: create minimal tarball with only CMake-required files
# Required: CMakeLists.txt, cmake/, Interfaces/, Libs/AAXLibrary/{source,include,CMakeLists.txt}, Extensions/GUI/CMakeLists.txt
echo "Encoding AAX_SDK (minimal CMake build)..."
SDK_PATH="$SDK_DIR/AAX_SDK"
if [ ! -d "$SDK_PATH" ]; then
    echo "Warning: $SDK_PATH not found, skipping"
else
    original_size=$(du -sh "$SDK_PATH" | cut -f1)

    # Create tarball with only required files for CMake build (using xz for better compression)
    (cd "$SDK_DIR" && tar -cJf - \
        AAX_SDK/CMakeLists.txt \
        AAX_SDK/CMakePresets.json \
        AAX_SDK/cmake \
        AAX_SDK/Interfaces \
        AAX_SDK/Libs/AAXLibrary/CMakeLists.txt \
        AAX_SDK/Libs/AAXLibrary/source \
        AAX_SDK/Libs/AAXLibrary/include \
        AAX_SDK/Extensions/GUI/CMakeLists.txt \
        2>/dev/null) | base64 $B64_OPTS > "$OUTPUT_DIR/aax_sdk.b64"

    encoded_size=$(wc -c < "$OUTPUT_DIR/aax_sdk.b64")
    echo "  Original: $original_size -> Encoded: $encoded_size bytes"
    echo "  Output: $OUTPUT_DIR/aax_sdk.b64"

    # GitHub secrets are limited to 48KB, split if necessary
    CHUNK_SIZE=45000  # Leave some margin below 48KB
    if [ "$encoded_size" -gt "$CHUNK_SIZE" ]; then
        echo "  Splitting into chunks for GitHub secrets (48KB limit)..."
        split -b $CHUNK_SIZE "$OUTPUT_DIR/aax_sdk.b64" "$OUTPUT_DIR/aax_sdk_part_"

        # Rename to numbered files
        i=1
        for part in "$OUTPUT_DIR"/aax_sdk_part_*; do
            mv "$part" "$OUTPUT_DIR/aax_sdk_part_$i.b64"
            part_size=$(wc -c < "$OUTPUT_DIR/aax_sdk_part_$i.b64")
            echo "    Part $i: $part_size bytes"
            i=$((i + 1))
        done
        NUM_PARTS=$((i - 1))
        echo "  Split into $NUM_PARTS parts"
    fi
fi

echo ""
echo "Done! Add these as GitHub repository secrets:"
echo "  VST2_SDK_B64 = contents of vst2_sdk.b64"
if [ -n "$NUM_PARTS" ]; then
    echo ""
    echo "  AAX SDK split into $NUM_PARTS secrets (GitHub 48KB limit):"
    for i in $(seq 1 $NUM_PARTS); do
        echo "    AAX_SDK_B64_$i = contents of aax_sdk_part_$i.b64"
    done
fi
echo ""
echo "To verify decoding works:"
echo "  cat vst2_sdk.b64 | base64 -d | tar -tzf - | head"

# Interactive clipboard helper (macOS only)
if [ "$1" = "--copy" ] && command -v pbcopy &> /dev/null; then
    echo ""
    echo "=== Copying secrets to clipboard ==="
    echo ""

    read -p "Copy VST2_SDK_B64 to clipboard? [Y/n] " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Nn]$ ]]; then
        cat "$OUTPUT_DIR/vst2_sdk.b64" | pbcopy
        echo "Copied! Paste into GitHub as: VST2_SDK_B64"
        read -p "Press Enter when ready for next secret..."
    fi

    if [ -n "$NUM_PARTS" ]; then
        for i in $(seq 1 $NUM_PARTS); do
            read -p "Copy AAX_SDK_B64_$i to clipboard? [Y/n] " -n 1 -r
            echo
            if [[ ! $REPLY =~ ^[Nn]$ ]]; then
                cat "$OUTPUT_DIR/aax_sdk_part_$i.b64" | pbcopy
                echo "Copied! Paste into GitHub as: AAX_SDK_B64_$i"
                if [ $i -lt $NUM_PARTS ]; then
                    read -p "Press Enter when ready for next secret..."
                fi
            fi
        done
    fi
    echo ""
    echo "All done!"
elif [ "$1" != "--copy" ]; then
    echo ""
    echo "Tip: Run with --copy flag to interactively copy each secret to clipboard:"
    echo "  $0 --copy"
fi
