#!/bin/bash
# Script to generate Xcode projects using XcodeGen
# Usage: ./generate_project.sh <example_name> [--ios]
#
# Prerequisites:
#   brew install xcodegen
#
# Examples:
#   ./generate_project.sh IPlugEffect           # Generate macOS project
#   ./generate_project.sh IPlugEffect --ios     # Generate iOS project
#   ./generate_project.sh --all                 # Generate all example projects

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IPLUG2_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
EXAMPLES_DIR="$IPLUG2_ROOT/Examples"

# Check if xcodegen is installed
if ! command -v xcodegen &> /dev/null; then
    echo "Error: xcodegen is not installed."
    echo "Install with: brew install xcodegen"
    exit 1
fi

# Print usage
usage() {
    echo "Usage: $0 <example_name> [--ios]"
    echo "       $0 --all"
    echo ""
    echo "Options:"
    echo "  <example_name>  Name of the example project (e.g., IPlugEffect)"
    echo "  --ios           Generate iOS project instead of macOS"
    echo "  --all           Generate all example projects"
    echo ""
    echo "Examples:"
    echo "  $0 IPlugEffect"
    echo "  $0 IPlugEffect --ios"
    echo "  $0 --all"
}

# Generate project for a single example
generate_project() {
    local example_name="$1"
    local platform="${2:-macOS}"
    local example_dir="$EXAMPLES_DIR/$example_name"

    if [ ! -d "$example_dir" ]; then
        echo "Error: Example directory not found: $example_dir"
        return 1
    fi

    local spec_file
    if [ "$platform" = "iOS" ]; then
        spec_file="$example_dir/project-ios.yml"
        if [ ! -f "$spec_file" ]; then
            spec_file="$example_dir/project.yml"
        fi
    else
        spec_file="$example_dir/project.yml"
        # Also check for simplified version
        if [ ! -f "$spec_file" ] && [ -f "$example_dir/project-simple.yml" ]; then
            spec_file="$example_dir/project-simple.yml"
        fi
    fi

    if [ ! -f "$spec_file" ]; then
        echo "Warning: No XcodeGen spec found for $example_name ($platform)"
        return 0
    fi

    echo "Generating $platform project for $example_name..."
    cd "$example_dir"

    local output_dir
    if [ "$platform" = "iOS" ]; then
        output_dir="projects"
        local project_name="${example_name}-iOS"
    else
        output_dir="projects"
        local project_name="${example_name}-macOS"
    fi

    xcodegen generate --spec "$spec_file" --project "$output_dir" --project-root .

    echo "Generated: $output_dir/$project_name.xcodeproj"
}

# Generate all example projects
generate_all() {
    echo "Generating all example projects..."

    for example_dir in "$EXAMPLES_DIR"/*; do
        if [ -d "$example_dir" ]; then
            local example_name=$(basename "$example_dir")

            # Check for project.yml or project-simple.yml
            if [ -f "$example_dir/project.yml" ] || [ -f "$example_dir/project-simple.yml" ]; then
                generate_project "$example_name" "macOS"
            fi

            if [ -f "$example_dir/project-ios.yml" ]; then
                generate_project "$example_name" "iOS"
            fi
        fi
    done

    echo "Done!"
}

# Main
if [ $# -eq 0 ]; then
    usage
    exit 1
fi

case "$1" in
    --all)
        generate_all
        ;;
    --help|-h)
        usage
        exit 0
        ;;
    *)
        example_name="$1"
        platform="macOS"

        if [ "$2" = "--ios" ]; then
            platform="iOS"
        fi

        generate_project "$example_name" "$platform"
        ;;
esac
