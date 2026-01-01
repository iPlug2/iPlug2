#!/bin/bash
#
# Setup script for iPlug2 git hooks
# Run this script to enable local git hooks for commit validation
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
HOOKS_DIR="$REPO_ROOT/.githooks"

echo "iPlug2 Git Hooks Setup"
echo "======================"
echo ""

# Check if we're in a git repository
if ! git -C "$REPO_ROOT" rev-parse --git-dir > /dev/null 2>&1; then
    echo "ERROR: Not a git repository"
    exit 1
fi

# Check if hooks directory exists
if [[ ! -d "$HOOKS_DIR" ]]; then
    echo "ERROR: Hooks directory not found at $HOOKS_DIR"
    exit 1
fi

# Make hooks executable
echo "Making hooks executable..."
chmod +x "$HOOKS_DIR"/*

# Configure git to use our hooks directory
echo "Configuring git hooks path..."
git -C "$REPO_ROOT" config core.hooksPath .githooks

echo ""
echo "Git hooks configured successfully!"
echo ""
echo "Active hooks:"
for hook in "$HOOKS_DIR"/*; do
    if [[ -f "$hook" && -x "$hook" ]]; then
        echo "  - $(basename "$hook")"
    fi
done
echo ""
echo "To disable hooks, run:"
echo "  git config --unset core.hooksPath"
echo ""
