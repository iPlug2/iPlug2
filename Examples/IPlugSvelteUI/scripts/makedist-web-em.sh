#!/bin/bash

# IPlugSvelteUI - Build script for Emscripten AudioWorklet with Svelte UI
# This builds a headless WASM module (DSP only) + Svelte UI

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_NAME="IPlugSvelteUI"
BUILD_DIR="$PROJECT_DIR/build-web-em"
WEB_UI_DIR="$PROJECT_DIR/web-ui"
IPLUG2_ROOT="$PROJECT_DIR/../.."

# Parse arguments
DEBUG_BUILD=0
for arg in "$@"; do
  case $arg in
    --debug)
      DEBUG_BUILD=1
      shift
      ;;
  esac
done

if [ "$DEBUG_BUILD" == "1" ]; then
  echo "=== Building $PROJECT_NAME for Web (DEBUG with DWARF) ==="
else
  echo "=== Building $PROJECT_NAME for Web (Emscripten AudioWorklet) ==="
fi
echo "Project directory: $PROJECT_DIR"
echo "Build output: $BUILD_DIR"
echo ""

# Create build directories
mkdir -p "$BUILD_DIR/scripts"
mkdir -p "$BUILD_DIR/styles"
mkdir -p "$BUILD_DIR/assets"

# Step 1: Build WASM module
echo "=== Step 1: Building WASM module ==="
cd "$PROJECT_DIR/projects"
emmake make -f "$PROJECT_NAME-em.mk" clean
if [ "$DEBUG_BUILD" == "1" ]; then
  emmake make -f "$PROJECT_NAME-em.mk" DEBUG=1
else
  emmake make -f "$PROJECT_NAME-em.mk"
fi
echo "WASM build complete!"
echo ""

# Step 2: Copy template files
echo "=== Step 2: Copying template files ==="
TEMPLATE_DIR="$IPLUG2_ROOT/IPlug/WEB/TemplateEm"
cp "$TEMPLATE_DIR/scripts/IPlugController.js" "$BUILD_DIR/scripts/"
cp "$TEMPLATE_DIR/scripts/serve.js" "$BUILD_DIR/"
echo "Template files copied!"
echo ""

# Step 3: Build Svelte UI for WASM target
echo "=== Step 3: Building Svelte UI ==="
cd "$WEB_UI_DIR"

# Check if node_modules exists
if [ ! -d "node_modules" ]; then
  echo "Installing npm dependencies..."
  npm install
fi

# Build for WASM mode (uses index-wasm.html template)
npm run build -- --mode wasm
echo "Svelte build complete!"

# Rename index-wasm files to index
if [ -f "$BUILD_DIR/index-wasm.html" ]; then
  mv "$BUILD_DIR/index-wasm.html" "$BUILD_DIR/index.html"
  echo "Renamed index-wasm.html to index.html"
fi
if [ -f "$BUILD_DIR/assets/index-wasm.css" ]; then
  mv "$BUILD_DIR/assets/index-wasm.css" "$BUILD_DIR/assets/index.css"
  # Update CSS reference in index.html
  sed -i '' 's/index-wasm\.css/index.css/g' "$BUILD_DIR/index.html"
  echo "Renamed index-wasm.css to index.css"
fi
echo ""

# Step 4: Launch browser (optional)
if [ "$1" == "--launch" ]; then
  echo "=== Launching browser ==="
  echo "Note: Server must send COOP/COEP headers for SharedArrayBuffer"
  echo "Use: npx serve -s $BUILD_DIR --cors"
  # open "$BUILD_DIR/index.html"
fi

echo "=== Build complete! ==="
echo "Output: $BUILD_DIR"
echo ""
echo "To test locally:"
echo "  cd $BUILD_DIR"
echo "  node serve.js"
echo ""
echo "Or use Vite dev server for hot-reload:"
echo "  cd $WEB_UI_DIR"
echo "  npm run dev"

if [ "$DEBUG_BUILD" == "1" ]; then
  echo ""
  echo "=== DEBUG BUILD ==="
  echo "DWARF debugging enabled. To debug in Chrome:"
  echo "  1. Install 'C/C++ DevTools Support (DWARF)' extension"
  echo "  2. Open DevTools > Sources > find .cpp files in file tree"
  echo "  3. Set breakpoints in C++ source"
  echo ""
  echo "Debug flags: -g3 -O1 -s ASSERTIONS=2 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2"
fi
