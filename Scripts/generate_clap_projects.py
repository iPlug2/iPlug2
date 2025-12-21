#!/usr/bin/env python3
"""
Generate CLAP MSVC projects from VST3 templates for iPlug2 examples/tests.
"""

import os
import re
import uuid
import xml.etree.ElementTree as ET

# Register namespace to avoid ns0: prefixes
ET.register_namespace('', 'http://schemas.microsoft.com/developer/msbuild/2003')

PROJECTS_TO_ADD_CLAP = [
    # Examples
    ('Examples/IPlugChunks', 'IPlugChunks'),
    ('Examples/IPlugControls', 'IPlugControls'),
    ('Examples/IPlugMidiEffect', 'IPlugMidiEffect'),
    ('Examples/IPlugOSCEditor', 'IPlugOSCEditor'),
    ('Examples/IPlugConvoEngine', 'IPlugConvoEngine'),
    ('Examples/IPlugDrumSynth', 'IPlugDrumSynth'),
    ('Examples/IPlugP5js', 'IPlugP5js'),
    ('Examples/IPlugResponsiveUI', 'IPlugResponsiveUI'),
    ('Examples/IPlugSurroundEffect', 'IPlugSurroundEffect'),
    ('Examples/IPlugSideChain', 'IPlugSideChain'),
    ('Examples/IPlugSvelteUI', 'IPlugSvelteUI'),
    ('Examples/IPlugWebUI', 'IPlugWebUI'),
    # Tests
    ('Tests/IGraphicsStressTest', 'IGraphicsStressTest'),
    ('Tests/IGraphicsTest', 'IGraphicsTest'),
    ('Tests/MetaParamTest', 'MetaParamTest'),
]

# VST3 SDK paths that should be removed
VST3_SDK_PATTERNS = [
    r'\.\.\\\.\.\\\.\.\\Dependencies\\IPlug\\VST3_SDK\\',
    r'\.\.\\\.\.\\VST3_SDK\\',
]

def generate_guid():
    """Generate a new GUID in the format used by MSVC."""
    return '{' + str(uuid.uuid4()).upper() + '}'

def convert_vst3_to_clap(vst3_content, project_name):
    """Convert VST3 project content to CLAP project content."""
    content = vst3_content

    # Generate new GUID
    new_guid = generate_guid()
    content = re.sub(r'<ProjectGuid>\{[A-F0-9-]+\}</ProjectGuid>',
                     f'<ProjectGuid>{new_guid}</ProjectGuid>', content)

    # Change project name
    content = re.sub(rf'<ProjectName>{project_name}-vst3</ProjectName>',
                     f'<ProjectName>{project_name}-clap</ProjectName>', content)

    # Change output directories from vst3 to clap
    content = re.sub(r'build-win\\vst3\\', r'build-win\\clap\\', content)

    # Change target extension from .vst3 to .clap
    content = re.sub(r'<TargetExt>\.vst3</TargetExt>', '<TargetExt>.clap</TargetExt>', content)

    # Add TargetName if not present (needs to be added after TargetExt for consistency)
    # This is a bit complex - we need to add TargetName=$(BINARY_NAME) in PropertyGroups that have IntDir
    # For simplicity, do this replacement properly

    # Replace VST3_DEFS with CLAP_DEFS
    content = re.sub(r'\$\(VST3_DEFS\)', '$(CLAP_DEFS)', content)

    # Replace VST3_INC_PATHS with CLAP_INC_PATHS
    content = re.sub(r'\$\(VST3_INC_PATHS\)', '$(CLAP_INC_PATHS)', content)

    # Remove VST3-specific AdditionalLibraryDirectories entries
    content = re.sub(r'<AdditionalLibraryDirectories>[^<]*VST3_SDK[^<]*</AdditionalLibraryDirectories>\s*', '', content)

    # Remove AdditionalDependencies that may reference VST3
    content = re.sub(r'\s*<AdditionalDependencies>%\(AdditionalDependencies\)</AdditionalDependencies>', '', content)

    return content, new_guid

def remove_vst3_sdk_files(tree):
    """Remove VST3 SDK ClInclude and ClCompile entries from the project."""
    root = tree.getroot()
    ns = {'': 'http://schemas.microsoft.com/developer/msbuild/2003'}

    # Define patterns for VST3 files to remove
    vst3_patterns = [
        'VST3_SDK',
        'IPlugVST3',
    ]

    for item_group in root.findall('.//{http://schemas.microsoft.com/developer/msbuild/2003}ItemGroup'):
        items_to_remove = []
        for child in item_group:
            include_attr = child.get('Include', '')
            for pattern in vst3_patterns:
                if pattern in include_attr:
                    items_to_remove.append(child)
                    break
        for item in items_to_remove:
            item_group.remove(item)

def add_clap_files(tree, project_name):
    """Add CLAP-specific ClInclude and ClCompile entries."""
    root = tree.getroot()
    ns = 'http://schemas.microsoft.com/developer/msbuild/2003'

    # Find the ClInclude ItemGroup
    for item_group in root.findall(f'.//{{{ns}}}ItemGroup'):
        # Check if this is the ClInclude group
        includes = item_group.findall(f'{{{ns}}}ClInclude')
        if includes:
            # Check if CLAP header is already present
            has_clap = any('IPlugCLAP.h' in inc.get('Include', '') for inc in includes)
            if not has_clap:
                clap_include = ET.SubElement(item_group, f'{{{ns}}}ClInclude')
                clap_include.set('Include', r'..\..\..\IPlug\CLAP\IPlugCLAP.h')
            break

    # Find the ClCompile ItemGroup
    for item_group in root.findall(f'.//{{{ns}}}ItemGroup'):
        compiles = item_group.findall(f'{{{ns}}}ClCompile')
        if compiles:
            # Check if CLAP source is already present
            has_clap = any('IPlugCLAP.cpp' in comp.get('Include', '') for comp in compiles)
            if not has_clap:
                clap_compile = ET.SubElement(item_group, f'{{{ns}}}ClCompile')
                clap_compile.set('Include', r'..\..\..\IPlug\CLAP\IPlugCLAP.cpp')
            break

def add_target_name_to_property_groups(content):
    """Add TargetName=$(BINARY_NAME) to PropertyGroups that have IntDir."""
    # Pattern to find PropertyGroups with IntDir but without TargetName
    # We need to add <TargetName>$(BINARY_NAME)</TargetName> after IntDir entries

    lines = content.split('\n')
    result = []
    i = 0
    while i < len(lines):
        line = lines[i]
        result.append(line)

        # Check if this is an IntDir line in a PropertyGroup
        if '<IntDir>' in line and 'clap' in line:
            # Check if the next few lines already have TargetName
            has_target_name = False
            has_target_ext = False
            for j in range(i+1, min(i+5, len(lines))):
                if '<TargetName>' in lines[j]:
                    has_target_name = True
                if '<TargetExt>' in lines[j]:
                    has_target_ext = True
                if '</PropertyGroup>' in lines[j]:
                    break

            # If no TargetName but has TargetExt, we should add TargetName
            if not has_target_name and has_target_ext:
                # Get indentation from current line
                indent = len(line) - len(line.lstrip())
                indent_str = ' ' * indent
                result.append(f'{indent_str}<TargetName>$(BINARY_NAME)</TargetName>')

        i += 1

    return '\n'.join(result)

def process_project(repo_root, rel_path, project_name):
    """Process a single project to create CLAP version."""
    projects_dir = os.path.join(repo_root, rel_path, 'projects')
    vst3_path = os.path.join(projects_dir, f'{project_name}-vst3.vcxproj')
    clap_path = os.path.join(projects_dir, f'{project_name}-clap.vcxproj')

    if not os.path.exists(vst3_path):
        print(f"  VST3 project not found: {vst3_path}")
        return None

    if os.path.exists(clap_path):
        print(f"  CLAP project already exists: {clap_path}")
        return None

    # Read VST3 project
    with open(vst3_path, 'r', encoding='utf-8-sig') as f:
        vst3_content = f.read()

    # Convert content
    clap_content, new_guid = convert_vst3_to_clap(vst3_content, project_name)

    # Parse as XML to modify ItemGroups
    tree = ET.ElementTree(ET.fromstring(clap_content))
    remove_vst3_sdk_files(tree)
    add_clap_files(tree, project_name)

    # Write back to string
    import io
    output = io.BytesIO()
    tree.write(output, encoding='utf-8', xml_declaration=True)
    clap_content = output.getvalue().decode('utf-8')

    # Fix the XML declaration and add BOM
    clap_content = clap_content.replace("<?xml version='1.0' encoding='utf-8'?>",
                                        '<?xml version="1.0" encoding="utf-8"?>')

    # Add TargetName where needed
    clap_content = add_target_name_to_property_groups(clap_content)

    # Write CLAP project with UTF-8 BOM
    with open(clap_path, 'w', encoding='utf-8-sig') as f:
        f.write(clap_content)

    print(f"  Created: {clap_path}")
    return new_guid

def update_solution_file(repo_root, rel_path, project_name, guid):
    """Add CLAP project to the solution file."""
    sln_path = os.path.join(repo_root, rel_path, f'{project_name}.sln')

    if not os.path.exists(sln_path):
        print(f"  Solution file not found: {sln_path}")
        return

    with open(sln_path, 'r', encoding='utf-8-sig') as f:
        sln_content = f.read()

    # Check if CLAP project is already in solution
    if f'{project_name}-clap' in sln_content:
        print(f"  CLAP project already in solution: {sln_path}")
        return

    # Find the last EndProject before Global section
    global_pos = sln_content.find('Global')
    if global_pos == -1:
        print(f"  Could not find Global section in: {sln_path}")
        return

    # Insert the CLAP project entry
    project_entry = f'Project("{{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}}") = "{project_name}-clap", "projects\\{project_name}-clap.vcxproj", "{guid}"\nEndProject\n'

    sln_content = sln_content[:global_pos] + project_entry + sln_content[global_pos:]

    # Find the ProjectConfigurationPlatforms section and add configurations
    config_section_start = sln_content.find('GlobalSection(ProjectConfigurationPlatforms)')
    if config_section_start == -1:
        print(f"  Could not find ProjectConfigurationPlatforms in: {sln_path}")
        return

    # Find the end of this section
    config_section_end = sln_content.find('EndGlobalSection', config_section_start)

    # Build configuration entries
    configs = ['Debug|ARM64EC', 'Debug|x64', 'Release|ARM64EC', 'Release|x64', 'Tracer|ARM64EC', 'Tracer|x64']
    config_entries = ''
    for config in configs:
        config_entries += f'\t\t{guid}.{config}.ActiveCfg = {config}\n'
        config_entries += f'\t\t{guid}.{config}.Build.0 = {config}\n'

    # Insert before EndGlobalSection
    sln_content = sln_content[:config_section_end] + config_entries + sln_content[config_section_end:]

    # Write back with UTF-8 BOM
    with open(sln_path, 'w', encoding='utf-8-sig') as f:
        f.write(sln_content)

    print(f"  Updated solution: {sln_path}")

def main():
    # Get repository root (script is in Scripts/)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.dirname(script_dir)

    print("Generating CLAP MSVC projects for iPlug2 Examples/Tests\n")

    for rel_path, project_name in PROJECTS_TO_ADD_CLAP:
        print(f"Processing {project_name}...")
        guid = process_project(repo_root, rel_path, project_name)
        if guid:
            update_solution_file(repo_root, rel_path, project_name, guid)
        print()

    print("Done!")

if __name__ == '__main__':
    main()
