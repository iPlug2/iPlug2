#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import sys
from pathlib import Path

# Shared constants
BASE_DIR = Path(__file__).resolve().parent.parent / "Build"
DEPOT_TOOLS_PATH = BASE_DIR / "tmp" / "depot_tools"
DEPOT_TOOLS_URL = "https://chromium.googlesource.com/chromium/tools/depot_tools.git"
SKIA_SRC_DIR = BASE_DIR / "src" / "skia"
TMP_DIR = BASE_DIR / "tmp" / "skia"

# Platform-specific library directories
MAC_LIB_DIR = BASE_DIR / "mac" / "lib"
IOS_LIB_DIR = BASE_DIR / "ios" / "lib"
WIN_LIB_DIR = BASE_DIR / "win"

# Platform-specific constants
MAC_MIN_VERSION = "10.3"
IOS_MIN_VERSION = "13.0"
WIN_CLANG_PATH = "C:\\Program Files\\LLVM"

# Shared libraries
LIBS = {
    "mac": [
        "libskia.a", "libskottie.a", "libskshaper.a", "libsksg.a",
        "libskparagraph.a", "libskunicode_icu.a", "libskunicode_core.a", "libsvg.a"
    ],
    "ios": [
        "libskia.a", "libskottie.a", "libsksg.a", "libskshaper.a",
        "libskparagraph.a", "libskunicode_core.a", "libskunicode_icu.a", "libsvg.a"
    ],
    "win": [
        "skia.lib", "skottie.lib", "sksg.lib", "skshaper.lib",
        "skparagraph.lib", "skunicode_icu.lib", "skunicode_core.lib", "svg.lib"
    ]
}

# Directories to package
PACKAGE_DIRS = [
    "include",
    "modules/skottie",
    "modules/skparagraph",
    "modules/skshaper",
    "modules/skresources",
    "modules/skunicode",
    "modules/skcms",
    "modules/svg",
    "src/core",
    "src/base",
    "src/utils",
    "src/xml",
    "third_party/externals/icu/source/common/unicode"
]

DONT_PACKAGE = [
    "android"
]

BASIC_GN_ARGS = """
    cc = "clang"
    cxx = "clang++"
    """

# Shared GN args
RELEASE_GN_ARGS = """
skia_use_system_libjpeg_turbo = false
skia_use_system_libpng = false
skia_use_system_zlib = false
skia_use_system_expat = false
skia_use_system_icu = false
skia_use_system_harfbuzz = false
skia_use_libwebp_decode = false
skia_use_libwebp_encode = false
skia_use_xps = false
skia_use_dng_sdk = false
skia_use_expat = true
skia_use_icu = true
skia_use_gl = true
skia_enable_graphite = true
skia_enable_svg = true
skia_enable_skottie = true
skia_enable_pdf = false
skia_enable_gpu = true
skia_enable_skparagraph = true
"""

# Platform-specific GN args
PLATFORM_GN_ARGS = {
    "mac": f"""
    skia_use_metal = true
    skia_use_dawn = true
    target_os = "mac"
    # extra_cflags = ["-mmacosx-version-min={MAC_MIN_VERSION}"]
    # extra_asmflags = ["-mmacosx-version-min={MAC_MIN_VERSION}"]
    extra_cflags_c = ["-Wno-error"]
    """,
    "ios": f"""
    skia_use_metal = true
    target_os = "ios"
    skia_ios_use_signing = false
    extra_cflags = ["-miphoneos-version-min={IOS_MIN_VERSION}", "-I../../../src/skia/third_party/externals/expat/lib"]
    extra_cflags_c = ["-Wno-error"]
    """,
    "win": f"""
    skia_use_dawn = true
    skia_use_direct3d = true
    clang_win = "{WIN_CLANG_PATH}"
    """
}

class SkiaBuildScript:
    def __init__(self):
        self.platform = None
        self.config = "Release"
        self.archs = []
        self.spm = False

    def parse_arguments(self):
        parser = argparse.ArgumentParser(description="Build Skia for macOS, iOS, and Windows")
        parser.add_argument("platform", choices=["mac", "ios", "win", "spm"], help="Target platform or swift package")
        parser.add_argument("-config", choices=["Debug", "Release"], default="Release", help="Build configuration")
        parser.add_argument("-archs", help="Target architectures (comma-separated)")
        args = parser.parse_args()

        if args.platform == "spm":
            self.spm = True
            self.platform = "mac"  # We'll handle iOS separately
            self.config = "Release"
            self.archs = ["universal"]
        else:
            self.platform = args.platform
            self.config = args.config
            if args.archs:
                self.archs = args.archs.split(',')
            else:
                self.archs = self.get_default_archs()

        self.validate_archs()

    def get_default_archs(self):
        if self.platform == "mac":
            return ["universal"]
        elif self.platform == "ios":
            return ["arm64"]
        elif self.platform == "win":
            return ["x64"]

    def validate_archs(self):
        valid_archs = {
            "mac": ["x86_64", "arm64", "universal"],
            "ios": ["x86_64", "arm64"],
            "win": ["x64", "Win32"]
        }
        for arch in self.archs:
            if arch not in valid_archs[self.platform]:
                print(f"Invalid architecture for {self.platform}: {arch}")
                sys.exit(1)

    def setup_depot_tools(self):
        if not DEPOT_TOOLS_PATH.exists():
            subprocess.run(["git", "clone", DEPOT_TOOLS_URL, str(DEPOT_TOOLS_PATH)], check=True)
        os.environ["PATH"] = f"{DEPOT_TOOLS_PATH}:{os.environ['PATH']}"

    def sync_deps(self):
        os.chdir(SKIA_SRC_DIR)
        print("Syncing Deps...")
        subprocess.run(["python3", "tools/git-sync-deps"], check=True)

    def generate_build_files(self, arch: str):
        output_dir = TMP_DIR / f"{self.platform}_{self.config}_{arch}"
        gn_args = BASIC_GN_ARGS

        if self.config == 'Debug':
            gn_args += """
            is_debug = true
            """
        else:
            gn_args += PLATFORM_GN_ARGS[self.platform]
            gn_args += RELEASE_GN_ARGS
            gn_args += """
            is_debug = false
            is_official_build = true
            """

        if self.platform == "mac":
            gn_args += f"""
                target_cpu = "{arch}"
            """
        elif self.platform == "ios":
            gn_args += f"""
                target_cpu = "{'arm64' if arch == 'arm64' else 'x64'}"
            """
        elif self.platform == "win":
            gn_args += f"""
                extra_cflags = ["{'/MTd' if self.config == 'Debug' else '/MT'}"]
                target_cpu = "{'x86' if arch == 'Win32' else 'x64'}"
            """

        subprocess.run(["./bin/gn", "gen", str(output_dir), f"--args={gn_args}"], check=True)

    def build_skia(self, arch: str):
        output_dir = TMP_DIR / f"{self.platform}_{self.config}_{arch}"
        
        # Get the list of libraries for the current platform
        libs_to_build = LIBS[self.platform]
        
        # On Windows, ninja expects targets without the .lib extension
        if self.platform == "win":
            libs_to_build = [lib[:-4] if lib.endswith('.lib') else lib for lib in libs_to_build]
        
        # Construct the ninja command with all library targets
        ninja_command = ["ninja", "-C", str(output_dir)] + libs_to_build
        
        # Run the ninja command
        try:
            subprocess.run(ninja_command, check=True)
            print(f"Successfully built targets for {self.platform} {arch}")
        except subprocess.CalledProcessError as e:
            print(f"Error: Build failed for {self.platform} {arch}")
            print(f"Ninja command: {' '.join(ninja_command)}")
            print(f"Error details: {e}")
            sys.exit(1)

    def move_libs(self, arch: str):
        src_dir = TMP_DIR / f"{self.platform}_{self.config}_{arch}"
        if self.platform == "mac":
            dest_dir = MAC_LIB_DIR / self.config / (arch if arch != "universal" else "")
        elif self.platform == "ios":
            dest_dir = IOS_LIB_DIR / self.config / arch
        else:  # Windows
            dest_dir = WIN_LIB_DIR / arch / self.config

        dest_dir.mkdir(parents=True, exist_ok=True)

        for lib in LIBS[self.platform]:
            src_file = src_dir / lib
            dest_file = dest_dir / lib
            if src_file.exists():
                shutil.copy2(str(src_file), str(dest_file))
                print(f"Copied {lib} to {dest_dir}")
                # Remove the source file after copying
                src_file.unlink()
            else:
                print(f"Warning: {lib} not found in {src_dir}")
    
    def create_universal_binary(self):
        print('Creating universal files...')
        dest_dir = MAC_LIB_DIR / self.config
        dest_dir.mkdir(parents=True, exist_ok=True)

        for lib in LIBS[self.platform]:
            input_libs = [str(MAC_LIB_DIR / self.config / arch / lib) for arch in ["x86_64", "arm64"]]
            output_lib = str(dest_dir / lib)
            subprocess.run(["lipo", "-create"] + input_libs + ["-output", output_lib], check=True)
            print(f"Created universal file: {lib}")

        # Remove architecture-specific folders
        # shutil.rmtree(MAC_LIB_DIR / self.config / "x86_64", ignore_errors=True)
        # shutil.rmtree(MAC_LIB_DIR / self.config / "arm64", ignore_errors=True)

    def combine_libraries(self, platform, arch):
        print(f"Combining libraries for {platform} {arch}...")
        if platform == "mac":
            lib_dir = MAC_LIB_DIR / self.config / (arch if arch != "universal" else "")
        else:  # iOS
            lib_dir = IOS_LIB_DIR / self.config / arch

        output_lib = lib_dir / "libSkia.a"
        input_libs = [str(lib_dir / lib) for lib in LIBS[platform] if (lib_dir / lib).exists()]

        if input_libs:
            libtool_command = ["libtool", "-static", "-o", str(output_lib)] + input_libs
            subprocess.run(libtool_command, check=True)
            print(f"Created combined library: {output_lib}")
        else:
            print(f"No libraries found to combine for {platform} {arch}")

    def create_xcframework(self):
        print("Creating Skia XCFramework...")
        xcframework_dir = BASE_DIR / "xcframework"
        xcframework_dir.mkdir(parents=True, exist_ok=True)

        xcframework_path = xcframework_dir / "Skia.xcframework"

        # Remove existing XCFramework if it exists
        if xcframework_path.exists():
            shutil.rmtree(xcframework_path)

        xcframework_command = ["xcodebuild", "-create-xcframework"]

        # Add iOS libraries
        for ios_arch in ["x86_64", "arm64"]:
            ios_lib_path = IOS_LIB_DIR / "Release" / ios_arch / "libSkia.a"
            xcframework_command.extend(["-library", str(ios_lib_path)])
            # Add headers
            headers_path = BASE_DIR / "include"
            xcframework_command.extend(["-headers", str(headers_path)])

        # Add macOS universal library
        mac_lib_path = MAC_LIB_DIR / "Release" / "libSkia.a"
        xcframework_command.extend(["-library", str(mac_lib_path)])

        # Add headers
        headers_path = BASE_DIR / "include"
        xcframework_command.extend(["-headers", str(headers_path)])

        # Specify output
        xcframework_command.extend(["-output", str(xcframework_path)])

        try:
            subprocess.run(xcframework_command, check=True)
            print(f"Created Skia XCFramework at {xcframework_path}")
        except subprocess.CalledProcessError as e:
            print(f"Error creating Skia XCFramework")
            print(f"Command: {' '.join(xcframework_command)}")
            print(f"Error details: {e}")


    def package_headers(self, dest_dir):
        print(f"Packaging headers to {dest_dir}...")
        dest_dir.mkdir(parents=True, exist_ok=True)

        for dir_path in PACKAGE_DIRS:
            src_path = SKIA_SRC_DIR / dir_path
            if src_path.exists() and src_path.is_dir():
                for root, dirs, files in os.walk(src_path):
                    # Remove excluded directories
                    dirs[:] = [d for d in dirs if d not in DONT_PACKAGE]

                    for file in files:
                        if file.endswith('.h'):
                            src_file = Path(root) / file
                            rel_path = src_file.relative_to(SKIA_SRC_DIR)
                            
                            # Check if the file is in an excluded directory
                            if not any(exclude in rel_path.parts for exclude in DONT_PACKAGE):
                                dest_file = dest_dir / rel_path
                                dest_file.parent.mkdir(parents=True, exist_ok=True)
                                shutil.copy2(src_file, dest_file)
                                # print(f"Copied {rel_path} to {dest_file}")


    def create_swift_package(self):
        print("Creating Swift package...")
        package_dir = BASE_DIR / "spm" / "Skia"
        package_dir.mkdir(parents=True, exist_ok=True)

        # Create package structure
        (package_dir / "Sources" / "Skia").mkdir(parents=True, exist_ok=True)
        (package_dir / "Skia").mkdir(parents=True, exist_ok=True)

        # Move XCFramework
        xcframework_src = BASE_DIR / "xcframework" / "Skia.xcframework"
        xcframework_dest = package_dir / "Skia" / "Skia.xcframework"
        if xcframework_src.exists():
            if xcframework_dest.exists():
                shutil.rmtree(xcframework_dest)
            shutil.copytree(xcframework_src, xcframework_dest)
            print(f"Copied XCFramework to {xcframework_dest}")
        else:
            print(f"Warning: XCFramework not found at {xcframework_src}")

        # Copy headers
        headers_dest = package_dir / "Sources" / "Skia"
        self.package_headers(headers_dest)

        # Create dummy.swift
        with open(package_dir / "Sources" / "Skia" / "dummy.swift", "w") as f:
            f.write("// This file is needed to make SPM happy\n")

        # Create Package.swift
        package_swift_content = """
// swift-tools-version:5.3
import PackageDescription

let package = Package(
    name: "Skia",
    products: [
        .library(
            name: "Skia",
            targets: ["Skia", "SkiaXCFramework"])
    ],
    targets: [
        .target(
            name: "Skia",
            dependencies: ["SkiaXCFramework"],
            path: "Sources",
            publicHeadersPath: "Skia"),
        .binaryTarget(
            name: "SkiaXCFramework",
            path: "Skia/Skia.xcframework"),
    ],
    cxxLanguageStandard: .cxx14
)
        """
        with open(package_dir / "Package.swift", "w") as f:
            f.write(package_swift_content)


#         module_map_content = """
# module Skia {
#   header "Skia.hpp"
#   export *
#   requires cplusplus
# }
# """

        print(f"Swift package created at {package_dir}")
    

    def cleanup(self):
        for arch in self.archs:
            shutil.rmtree(TMP_DIR / f"{self.platform}_{self.config}_{arch}", ignore_errors=True)
        print("Cleaned up temporary directories")

    def run(self):
        self.parse_arguments()
        self.setup_depot_tools()
        self.sync_deps()

        if self.spm:
            # Build for macOS
            self.platform = "mac"
            self.archs = ["universal"]
            self.generate_build_files("universal")
            self.build_skia("universal")
            self.move_libs("universal")
            self.create_universal_binary()
            self.combine_libraries("mac", "universal")

            # Build for iOS
            self.platform = "ios"
            self.archs = ["x86_64", "arm64"]
            for arch in self.archs:
                self.generate_build_files(arch)
                self.build_skia(arch)
                self.move_libs(arch)
                self.combine_libraries("ios", arch)

            self.package_headers(BASE_DIR / "include")
            self.create_xcframework()
            self.create_swift_package()
        else:
            if "universal" in self.archs:
                self.archs = ["x86_64", "arm64"]

            for arch in self.archs:
                self.generate_build_files(arch)
                self.build_skia(arch)
                self.move_libs(arch)

            if self.platform == "mac" and self.archs == ["x86_64", "arm64"]:
                self.create_universal_binary()

            self.package_headers(BASE_DIR / "include")
            # self.cleanup()

        print(f"Build completed successfully for {self.platform} {self.config} configuration with architectures: {', '.join(self.archs)}")

if __name__ == "__main__":
    SkiaBuildScript().run()
