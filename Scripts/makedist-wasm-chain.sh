#!/bin/bash

# makedist-wasm-chain.sh - Build multiple iPlug2 plugins for a device chain
#
# This script builds multiple plugins and assembles them into a single
# web application with the multi-device chain template.
#
# Usage:
#   makedist-wasm-chain.sh <output_dir> <plugin1> <plugin2> [plugin3] ...
#
# Example:
#   makedist-wasm-chain.sh ~/chain-demo IPlugInstrument IPlugEffect
#
# Arguments:
#   output_dir   : Output directory for the chain web app
#   plugin1..N   : Plugin project names (must exist in Examples/ or Tests/)

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
IPLUG2_ROOT="$SCRIPT_DIR/.."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo_step() {
  echo -e "${BLUE}==>${NC} $1"
}

echo_success() {
  echo -e "${GREEN}[OK]${NC} $1"
}

echo_error() {
  echo -e "${RED}[ERROR]${NC} $1"
}

echo_warn() {
  echo -e "${YELLOW}[WARN]${NC} $1"
}

# Check arguments
if [ "$#" -lt 2 ]; then
  echo "Usage: makedist-wasm-chain.sh <output_dir> <plugin1> <plugin2> [plugin3] ..."
  echo ""
  echo "Example:"
  echo "  makedist-wasm-chain.sh ~/chain-demo IPlugInstrument IPlugEffect"
  echo ""
  echo "Available plugins:"
  for dir in "$IPLUG2_ROOT/Examples"/*/ "$IPLUG2_ROOT/Tests"/*/; do
    name=$(basename "$dir")
    if [ -f "$dir/config.h" ] && [ -f "$dir/scripts/makedist-wasm.sh" ]; then
      echo "  - $name"
    fi
  done
  exit 1
fi

OUTPUT_DIR="$1"
shift
PLUGINS=("$@")

echo ""
echo "============================================================"
echo "iPlug2 Multi-Device Chain Builder"
echo "============================================================"
echo ""
echo "Output: $OUTPUT_DIR"
echo "Plugins: ${PLUGINS[*]}"
echo ""

# Create output directory structure
echo_step "Creating output directory structure..."
mkdir -p "$OUTPUT_DIR"
mkdir -p "$OUTPUT_DIR/scripts"
mkdir -p "$OUTPUT_DIR/styles"

# Copy chain manager and template files
echo_step "Copying chain template files..."
cp "$IPLUG2_ROOT/IPlug/WEB/TemplateWasm/scripts/PluginChainManager.js" "$OUTPUT_DIR/scripts/"
cp "$IPLUG2_ROOT/IPlug/WEB/TemplateWasm/multi-index.html" "$OUTPUT_DIR/index.html"
cp "$IPLUG2_ROOT/IPlug/WEB/TemplateWasm/styles/multi-chain.css" "$OUTPUT_DIR/styles/"
echo_success "Template files copied"

# Track available plugins for the preset buttons
AVAILABLE_PLUGINS_HTML=""

# Build each plugin
for PLUGIN_NAME in "${PLUGINS[@]}"; do
  echo ""
  echo "============================================================"
  echo "Building: $PLUGIN_NAME"
  echo "============================================================"

  # Find the plugin directory
  PLUGIN_DIR=""
  if [ -d "$IPLUG2_ROOT/Examples/$PLUGIN_NAME" ]; then
    PLUGIN_DIR="$IPLUG2_ROOT/Examples/$PLUGIN_NAME"
  elif [ -d "$IPLUG2_ROOT/Tests/$PLUGIN_NAME" ]; then
    PLUGIN_DIR="$IPLUG2_ROOT/Tests/$PLUGIN_NAME"
  else
    echo_error "Plugin not found: $PLUGIN_NAME"
    echo "Looked in:"
    echo "  - $IPLUG2_ROOT/Examples/$PLUGIN_NAME"
    echo "  - $IPLUG2_ROOT/Tests/$PLUGIN_NAME"
    continue
  fi

  # Check if plugin has a WASM build script
  if [ ! -f "$PLUGIN_DIR/scripts/makedist-wasm.sh" ]; then
    echo_warn "Plugin $PLUGIN_NAME has no makedist-wasm.sh script, skipping..."
    continue
  fi

  # Build the plugin (using off to not launch browser)
  echo_step "Running makedist-wasm.sh for $PLUGIN_NAME..."
  cd "$PLUGIN_DIR/scripts"
  bash makedist-wasm.sh off 2>&1 | sed 's/^/  /'

  if [ ! -d "$PLUGIN_DIR/build-web-wasm" ]; then
    echo_error "Build failed for $PLUGIN_NAME - no build-web-wasm directory"
    continue
  fi

  # Copy plugin build output to chain output directory
  echo_step "Copying $PLUGIN_NAME build output..."
  PLUGIN_OUTPUT_DIR="$OUTPUT_DIR/$PLUGIN_NAME"
  mkdir -p "$PLUGIN_OUTPUT_DIR"
  mkdir -p "$PLUGIN_OUTPUT_DIR/scripts"

  # Copy WASM modules and scripts
  cp "$PLUGIN_DIR/build-web-wasm/scripts/"*.js "$PLUGIN_OUTPUT_DIR/scripts/" 2>/dev/null || true

  # Copy resource data files (fonts, images, etc.)
  cp "$PLUGIN_DIR/build-web-wasm/"*.data "$PLUGIN_OUTPUT_DIR/" 2>/dev/null || true

  # Extract plugin info for the preset button
  PLUGIN_NAME_LC=$(echo "$PLUGIN_NAME" | tr '[:upper:]' '[:lower:]')
  MAXNINPUTS=$(python3 "$IPLUG2_ROOT/Scripts/parse_iostr.py" "$PLUGIN_DIR" inputs 2>/dev/null || echo "2")
  IS_INSTRUMENT="false"
  if [ "$MAXNINPUTS" = "0" ]; then
    IS_INSTRUMENT="true"
  fi

  # Create bundle info file for the chain manager
  cat > "$PLUGIN_OUTPUT_DIR/scripts/${PLUGIN_NAME}-info.js" << EOF
// Plugin bundle info for chain manager
window['${PLUGIN_NAME}_BundleInfo'] = {
  name: '${PLUGIN_NAME}',
  numInputs: ${MAXNINPUTS:-2},
  numOutputs: 2,
  isInstrument: ${IS_INSTRUMENT}
};
window['${PLUGIN_NAME}_BundleLoaded'] = true;
EOF

  # Add to available plugins
  PLUGIN_TYPE_LABEL=""
  if [ "$IS_INSTRUMENT" = "true" ]; then
    PLUGIN_TYPE_LABEL=" (Synth)"
  fi
  AVAILABLE_PLUGINS_HTML+="        <button class=\"plugin-preset\" data-plugin=\"$PLUGIN_NAME\">+ $PLUGIN_NAME$PLUGIN_TYPE_LABEL</button>\n"

  echo_success "$PLUGIN_NAME built and copied"
done

# Update index.html with available plugins
echo ""
echo_step "Updating index.html with available plugins..."

# Create the plugins list for the HTML
PLUGINS_LIST=$(printf '%s\n' "${PLUGINS[@]}" | paste -sd ',' -)

# Replace the preset buttons in index.html
# First, create a backup
cp "$OUTPUT_DIR/index.html" "$OUTPUT_DIR/index.html.bak"

# Use awk to replace the availablePlugins section
awk -v plugins="$AVAILABLE_PLUGINS_HTML" '
  /<div id="availablePlugins">/ {
    print;
    print plugins;
    skip=1;
    next;
  }
  /<\/div>/ && skip {
    print;
    skip=0;
    next;
  }
  !skip {print}
' "$OUTPUT_DIR/index.html.bak" > "$OUTPUT_DIR/index.html.tmp"

# If the awk approach worked, use the output
if [ -s "$OUTPUT_DIR/index.html.tmp" ]; then
  mv "$OUTPUT_DIR/index.html.tmp" "$OUTPUT_DIR/index.html"
else
  # Fallback: simple replacement using sed
  # Remove existing preset buttons and add new ones
  BUTTONS_BLOCK=$(echo -e "$AVAILABLE_PLUGINS_HTML")
  # Keep original for now - preset buttons are already there as examples
  mv "$OUTPUT_DIR/index.html.bak" "$OUTPUT_DIR/index.html"
fi

rm -f "$OUTPUT_DIR/index.html.bak" "$OUTPUT_DIR/index.html.tmp"

echo ""
echo "============================================================"
echo "BUILD COMPLETE"
echo "============================================================"
echo ""
echo "Output directory: $OUTPUT_DIR"
echo ""
echo "Contents:"
find "$OUTPUT_DIR" -maxdepth 2 -type f -name "*.js" -o -name "*.html" -o -name "*.css" | sort | sed 's/^/  /'
echo ""
echo "To run:"
echo "  cd $OUTPUT_DIR"
echo "  emrun --no_emrun_detect index.html"
echo ""
echo "Or with npx serve:"
echo "  npx serve -p 8080 --cors"
echo ""
echo_warn "IMPORTANT: Server must send these headers for SharedArrayBuffer:"
echo "  Cross-Origin-Opener-Policy: same-origin"
echo "  Cross-Origin-Embedder-Policy: require-corp"
echo ""
