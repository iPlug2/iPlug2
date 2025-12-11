#!/usr/bin/env bash
set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
IPLUG2_DIR="$(dirname "$(dirname "$PROJECT_DIR")")"

# Source and destination directories
FAUST_LIBS_SRC="$IPLUG2_DIR/Dependencies/Build/mac/share/faust"
FAUST_LIBS_DST="$PROJECT_DIR/resources/faust-libs"

# Check source exists
if [ ! -d "$FAUST_LIBS_SRC" ]; then
  echo "Error: FAUST libraries not found at $FAUST_LIBS_SRC"
  echo "Run Dependencies/Extras/faust/build-faust-mac.sh first"
  exit 1
fi

# Create destination if needed
mkdir -p "$FAUST_LIBS_DST"

# Copy all .lib files
echo "Copying FAUST libraries..."
cp "$FAUST_LIBS_SRC"/*.lib "$FAUST_LIBS_DST/"

# Count copied files
COUNT=$(ls -1 "$FAUST_LIBS_DST"/*.lib 2>/dev/null | wc -l | tr -d ' ')
echo "Copied $COUNT FAUST library files to $FAUST_LIBS_DST"
