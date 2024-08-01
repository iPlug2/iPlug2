#!/usr/bin/env python3
"""
Configure web template files with placeholder replacement.

This script replaces placeholders in iPlug2 web template files (HTML, JS)
with project-specific values.

Placeholders:
  NAME_PLACEHOLDER - Project name
  ORIGIN_PLACEHOLDER - Site origin for script paths (default: "/")
  MAXNINPUTS_PLACEHOLDER - Maximum number of audio inputs
  MAXNOUTPUTS_PLACEHOLDER - Maximum number of audio outputs
"""

import argparse
import sys
import os

def configure_template(input_path, output_path, name, origin="/", max_inputs=0, max_outputs=2):
    """
    Read a template file and replace placeholders with actual values.

    Args:
        input_path: Path to template file
        output_path: Path for output file
        name: Project name to replace NAME_PLACEHOLDER
        origin: Site origin to replace ORIGIN_PLACEHOLDER
        max_inputs: Max audio inputs to replace MAXNINPUTS_PLACEHOLDER
        max_outputs: Max audio outputs to replace MAXNOUTPUTS_PLACEHOLDER
    """
    with open(input_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # Replace all placeholders
    content = content.replace('NAME_PLACEHOLDER', name)
    content = content.replace('ORIGIN_PLACEHOLDER', origin)

    # For max inputs, use empty string if 0 (instruments don't need input)
    max_inputs_str = str(max_inputs) if max_inputs > 0 else ''
    content = content.replace('MAXNINPUTS_PLACEHOLDER', max_inputs_str)
    content = content.replace('MAXNOUTPUTS_PLACEHOLDER', str(max_outputs))

    # Ensure output directory exists
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(content)

def main():
    parser = argparse.ArgumentParser(
        description='Configure iPlug2 web template files with project-specific values'
    )
    parser.add_argument('--input', '-i', required=True,
                        help='Input template file path')
    parser.add_argument('--output', '-o', required=True,
                        help='Output file path')
    parser.add_argument('--name', '-n', required=True,
                        help='Project name (replaces NAME_PLACEHOLDER)')
    parser.add_argument('--origin', default='/',
                        help='Site origin for script paths (default: /)')
    parser.add_argument('--max-inputs', type=int, default=0,
                        help='Maximum number of audio inputs (default: 0)')
    parser.add_argument('--max-outputs', type=int, default=2,
                        help='Maximum number of audio outputs (default: 2)')

    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Error: Input file not found: {args.input}", file=sys.stderr)
        return 1

    try:
        configure_template(
            args.input,
            args.output,
            args.name,
            args.origin,
            args.max_inputs,
            args.max_outputs
        )
        print(f"Configured: {args.output}")
        return 0
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())
