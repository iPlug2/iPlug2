#!/usr/bin/env python3
"""
Parse /ci commands from PR description for iPlug2 CI builds.

This script parses special CI commands from a GitHub PR description and outputs
Azure Pipeline variable commands to control what gets built.

Usage:
    python parse_ci_commands.py <pr_body_text>
    python parse_ci_commands.py --file <path_to_pr_body_file>

Command Syntax (in PR description):
    /ci projects=Project1,Project2   # Build specific projects
    /ci platforms=mac,win,ios,web    # Build specific platforms
    /ci formats=vst3,app,clap,auv2   # Build specific formats
    /ci graphics=nanovg,skia         # Graphics backends (or 'both')
    /ci test                         # Enable plugin testing
    /ci skip                         # Skip CI entirely

    # Shortcuts
    /ci full                         # All projects, all enabled platforms
    /ci quick                        # IPlugEffect only (default)
    /ci web                          # Web platform only
    /ci mac                          # macOS only
    /ci win                          # Windows only

    # Combined syntax
    /ci projects=IPlugControls platforms=mac formats=vst3 test
"""

import sys
import re
import argparse
import json

# Available projects (must match projects.yml)
ALL_PROJECTS = [
    'IPlugEffect',
    'IPlugInstrument',
    'IPlugMidiEffect',
    'IPlugControls',
    'IPlugResponsiveUI',
    'IPlugChunks',
    'IPlugSideChain',
    'IPlugSurroundEffect',
    'IPlugDrumSynth',
    'IPlugConvoEngine',
    'IPlugOSCEditor',
    'IPlugVisualizer',
    'IPlugReaperPlugin',
    'IPlugCocoaUI',
    'IPlugSwiftUI',
    'IPlugWebUI',
    'IPlugP5js',
    'IPlugSvelteUI',
    'IGraphicsStressTest',
    'IGraphicsTest',
    'MetaParamTest',
]

ALL_PLATFORMS = ['mac', 'win', 'ios', 'web']
ALL_FORMATS = ['app', 'vst3', 'clap', 'auv2', 'aax', 'vst2']
ALL_GRAPHICS = ['nanovg', 'skia']


def parse_ci_commands(text):
    """Parse /ci commands from PR body text."""
    result = {
        'projects': [],
        'platforms': [],
        'formats': [],
        'graphics': [],
        'test': False,
        'skip': False,
        'full': False,
        'build_all_projects': False,
    }

    if not text:
        return result

    # Find all /ci command lines
    ci_pattern = re.compile(r'^/ci\s+(.*)$', re.MULTILINE | re.IGNORECASE)
    matches = ci_pattern.findall(text)

    for match in matches:
        args = match.strip()

        # Handle shortcuts first
        if args.lower() == 'skip':
            result['skip'] = True
            continue

        if args.lower() == 'full':
            result['full'] = True
            result['build_all_projects'] = True
            continue

        if args.lower() == 'quick':
            result['projects'] = ['IPlugEffect']
            continue

        # Platform shortcuts
        if args.lower() in ['web', 'mac', 'win', 'ios']:
            result['platforms'].append(args.lower())
            continue

        # Parse key=value pairs and flags
        tokens = args.split()
        for token in tokens:
            token_lower = token.lower()

            # Flag-style arguments
            if token_lower == 'test':
                result['test'] = True
                continue

            if token_lower == 'skip':
                result['skip'] = True
                continue

            if token_lower == 'full':
                result['full'] = True
                result['build_all_projects'] = True
                continue

            # Key=value style arguments
            if '=' in token:
                key, value = token.split('=', 1)
                key = key.lower()
                values = [v.strip() for v in value.split(',') if v.strip()]

                if key == 'projects' or key == 'project':
                    # Match project names case-insensitively
                    for v in values:
                        for proj in ALL_PROJECTS:
                            if proj.lower() == v.lower():
                                if proj not in result['projects']:
                                    result['projects'].append(proj)
                                break

                elif key == 'platforms' or key == 'platform':
                    for v in values:
                        if v.lower() in ALL_PLATFORMS:
                            if v.lower() not in result['platforms']:
                                result['platforms'].append(v.lower())

                elif key == 'formats' or key == 'format':
                    for v in values:
                        if v.lower() in ALL_FORMATS:
                            if v.lower() not in result['formats']:
                                result['formats'].append(v.lower())

                elif key == 'graphics' or key == 'gfx':
                    for v in values:
                        if v.lower() == 'both':
                            result['graphics'] = ['nanovg', 'skia']
                        elif v.lower() in ALL_GRAPHICS:
                            if v.lower() not in result['graphics']:
                                result['graphics'].append(v.lower())

    return result


def output_azure_variables(config):
    """Output Azure Pipeline variable commands."""

    # If skip is set, output a skip variable and exit
    if config['skip']:
        print('##vso[task.setvariable variable=ci_skip]true')
        print('CI commands parsed: SKIP CI')
        return

    print('##vso[task.setvariable variable=ci_skip]false')

    # Projects
    if config['projects']:
        projects_str = ','.join(config['projects'])
        print(f'##vso[task.setvariable variable=ci_projects]{projects_str}')
        print(f'##vso[task.setvariable variable=ci_projects;isOutput=true]{projects_str}')

    # Build all projects flag
    if config['build_all_projects'] or config['full']:
        print('##vso[task.setvariable variable=ci_build_all_projects]true')
        print('##vso[task.setvariable variable=ci_build_all_projects;isOutput=true]true')
    else:
        print('##vso[task.setvariable variable=ci_build_all_projects]false')
        print('##vso[task.setvariable variable=ci_build_all_projects;isOutput=true]false')

    # Platforms
    if config['platforms']:
        for platform in ALL_PLATFORMS:
            enabled = 'true' if platform in config['platforms'] else 'false'
            print(f'##vso[task.setvariable variable=ci_build_{platform}]{enabled}')
            print(f'##vso[task.setvariable variable=ci_build_{platform};isOutput=true]{enabled}')

    # Formats
    if config['formats']:
        for fmt in ALL_FORMATS:
            enabled = 'true' if fmt in config['formats'] else 'false'
            print(f'##vso[task.setvariable variable=ci_build_{fmt}]{enabled}')
            print(f'##vso[task.setvariable variable=ci_build_{fmt};isOutput=true]{enabled}')

    # Graphics backends
    if config['graphics']:
        nanovg = 'true' if 'nanovg' in config['graphics'] else 'false'
        skia = 'true' if 'skia' in config['graphics'] else 'false'
        print(f'##vso[task.setvariable variable=ci_build_nanovg]{nanovg}')
        print(f'##vso[task.setvariable variable=ci_build_skia]{skia}')
        print(f'##vso[task.setvariable variable=ci_build_nanovg;isOutput=true]{nanovg}')
        print(f'##vso[task.setvariable variable=ci_build_skia;isOutput=true]{skia}')

    # Testing
    test_enabled = 'true' if config['test'] else 'false'
    print(f'##vso[task.setvariable variable=ci_test_projects]{test_enabled}')
    print(f'##vso[task.setvariable variable=ci_test_projects;isOutput=true]{test_enabled}')

    # Summary for logs
    summary = []
    if config['projects']:
        summary.append(f"Projects: {', '.join(config['projects'])}")
    elif config['build_all_projects']:
        summary.append("Projects: ALL")
    else:
        summary.append("Projects: IPlugEffect (default)")

    if config['platforms']:
        summary.append(f"Platforms: {', '.join(config['platforms'])}")
    else:
        summary.append("Platforms: all enabled")

    if config['formats']:
        summary.append(f"Formats: {', '.join(config['formats'])}")
    else:
        summary.append("Formats: all enabled")

    if config['graphics']:
        summary.append(f"Graphics: {', '.join(config['graphics'])}")
    else:
        summary.append("Graphics: nanovg (default)")

    if config['test']:
        summary.append("Testing: enabled")

    print(f"\nCI commands parsed: {'; '.join(summary)}")


def output_json(config):
    """Output configuration as JSON."""
    print(json.dumps(config, indent=2))


def main():
    parser = argparse.ArgumentParser(description='Parse /ci commands from PR description')
    parser.add_argument('text', nargs='?', help='PR body text')
    parser.add_argument('--file', '-f', help='Path to file containing PR body')
    parser.add_argument('--json', '-j', action='store_true', help='Output as JSON instead of Azure variables')

    args = parser.parse_args()

    text = ''
    if args.file:
        try:
            with open(args.file, 'r') as f:
                text = f.read()
        except FileNotFoundError:
            print(f"Error: File not found: {args.file}", file=sys.stderr)
            sys.exit(1)
    elif args.text:
        text = args.text
    else:
        # Read from stdin
        text = sys.stdin.read()

    config = parse_ci_commands(text)

    if args.json:
        output_json(config)
    else:
        output_azure_variables(config)


if __name__ == '__main__':
    main()
