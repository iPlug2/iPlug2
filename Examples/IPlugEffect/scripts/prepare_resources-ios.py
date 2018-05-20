#!/usr/bin/python

# this script will update info plist files based on config.h

kAudioUnitType_MusicDevice      = "aumu"
kAudioUnitType_MusicEffect      = "aumf"
kAudioUnitType_Effect           = "aufx"

DONT_COPY = ("")

import plistlib, os, datetime, fileinput, glob, sys, string, shutil

scriptpath = os.path.dirname(os.path.realpath(__file__))
projectpath = os.path.abspath(os.path.join(scriptpath, os.pardir))

print projectpath

def replacestrs(filename, s, r):
  files = glob.glob(filename)

  for line in fileinput.input(files,inplace=1):
    string.find(line, s)
    line = line.replace(s, r)
    sys.stdout.write(line)

def main():
  MAJORSTR = ""
  MINORSTR = ""
  BUGFIXSTR = ""
  PLUG_VER_STR = ""

  BUNDLE_MFR = ""
  BUNDLE_NAME = ""
  BUNDLE_DOMAIN = ""
  PLUG_NAME_STR = ""
  PLUG_MFR_NAME_STR = ""
  PLUG_CHANNEL_IO = ""
  PLUG_COPYRIGHT = ""
  PLUG_UID = ""
  PLUG_MFR_UID = ""
  PLUG_IS_INSTRUMENT = 0
  PLUG_DOES_MIDI = 0
  PLUG_HAS_UI = 0
  PLUG_SHARED_RESOURCES = 0

  # extract values from config.h
  for line in fileinput.input(projectpath + "/config.h", inplace=0):
    if "#define PLUG_VERSION_HEX " in line:
      PLUG_VER_STR = string.lstrip(line, "#define PLUG_VERSION_HEX ")
      PLUG_VER = int(PLUG_VER_STR, 16)
      MAJOR = PLUG_VER & 0xFFFF0000
      MAJORSTR = str(MAJOR >> 16)
      MINOR = PLUG_VER & 0x0000FF00
      MINORSTR = str(MINOR >> 8)
      BUGFIXSTR = str(PLUG_VER & 0x000000FF)

    if "#define PLUG_NAME " in line:
      PLUG_NAME_STR = string.lstrip(line, "#define PLUG_NAME ")

    if "#define PLUG_MFR " in line:
      PLUG_MFR_NAME_STR = string.lstrip(line, "#define PLUG_MFR ")

    if "#define BUNDLE_MFR " in line:
      BUNDLE_MFR = string.lstrip(line, "#define BUNDLE_MFR ")

    if "#define BUNDLE_NAME " in line:
      BUNDLE_NAME = string.lstrip(line, "#define BUNDLE_NAME ")

    if "#define BUNDLE_DOMAIN " in line:
      BUNDLE_DOMAIN = string.lstrip(line, "#define BUNDLE_DOMAIN ")

    if "#define PLUG_CHANNEL_IO " in line:
      PLUG_CHANNEL_IO = string.lstrip(line, "#define PLUG_CHANNEL_IO ")

    if "#define PLUG_COPYRIGHT_STR " in line:
      PLUG_COPYRIGHT = string.lstrip(line, "#define PLUG_COPYRIGHT_STR ")

    if "#define PLUG_UNIQUE_ID " in line:
      PLUG_UID = string.lstrip(line, "#define PLUG_UNIQUE_ID ")

    if "#define PLUG_MFR_ID " in line:
      PLUG_MFR_UID = string.lstrip(line, "#define PLUG_MFR_ID ")

    if "#define PLUG_IS_INSTRUMENT " in line:
      PLUG_IS_INSTRUMENT = int(string.lstrip(line, "#define PLUG_IS_INSTRUMENT "), 16)

    if "#define PLUG_DOES_MIDI " in line:
      PLUG_DOES_MIDI = int(string.lstrip(line, "#define PLUG_DOES_MIDI "), 16)

    if "#define PLUG_HAS_UI " in line:
      PLUG_HAS_UI = int(string.lstrip(line, "#define PLUG_HAS_UI "), 16)

    if "#define PLUG_SHARED_RESOURCES " in line:
      PLUG_SHARED_RESOURCES = int(string.lstrip(line, "#define PLUG_SHARED_RESOURCES "), 16)

  FULLVERSIONSTR = MAJORSTR + "." + MINORSTR + "." + BUGFIXSTR

  #strip quotes and newlines
  PLUG_VER_STR = PLUG_VER_STR[0:-1]
  BUNDLE_MFR = BUNDLE_MFR[1:-2]
  BUNDLE_NAME = BUNDLE_NAME[1:-2]
  BUNDLE_DOMAIN = BUNDLE_DOMAIN[1:-2]

  PLUG_NAME_STR = PLUG_NAME_STR[1:-2]
  PLUG_MFR_NAME_STR = PLUG_MFR_NAME_STR[1:-2]
  PLUG_CHANNEL_IO = PLUG_CHANNEL_IO[1:-2]
  PLUG_COPYRIGHT = PLUG_COPYRIGHT[1:-2]
  PLUG_MFR_UID = PLUG_MFR_UID[1:-2]
  PLUG_UID = PLUG_UID[1:-2]

  CFBundleGetInfoString = BUNDLE_NAME + " v" + FULLVERSIONSTR + " " + PLUG_COPYRIGHT
  CFBundleVersion = FULLVERSIONSTR
  CFBundlePackageType = "BNDL"
  CSResourcesFileMapped = True

  fileinput.close()

  LSMinimumSystemVersion = "11.0"

  BASE_SDK = "iphoneos11.3"
  DEPLOYMENT_TARGET = "11"

  # extract values from common.xcconfig
  for line in fileinput.input(projectpath + "/../../common-ios.xcconfig", inplace=0):
    if not "//" in line:
      if "BASE_SDK_IOS = " in line:
        BASE_SDK = string.lstrip(line, "BASE_SDK_IOS = ")
      if "DEPLOYMENT_TARGET = " in line:
        DEPLOYMENT_TARGET = string.lstrip(line, "DEPLOYMENT_TARGET = ")

  BASE_SDK = BASE_SDK[0:-1]
  DEPLOYMENT_TARGET = DEPLOYMENT_TARGET[0:-1]
  DEPLOYMENT_TARGET += ".0"

  LSMinimumSystemVersion = DEPLOYMENT_TARGET

#  print "Copying resources to shared folder..."
#
#  if PLUG_SHARED_RESOURCES:
#    dst = os.path.expanduser("~") + "/Music/IPlugEffect/Resources"
#    if os.path.exists(dst):
#     shutil.rmtree(dst)
#     #os.makedirs(dst)
#
#    shutil.copytree(projectpath + "/resources/img/", dst, ignore=shutil.ignore_patterns(*DONT_COPY))

  print "Processing Info.plist files..."

# AUDIOUNIT v3

  if PLUG_IS_INSTRUMENT:
    COMP_TYPE = kAudioUnitType_MusicDevice
  elif PLUG_DOES_MIDI:
    COMP_TYPE = kAudioUnitType_MusicEffect
  else:
    COMP_TYPE = kAudioUnitType_Effect

  if PLUG_HAS_UI:
    NSEXTENSIONPOINTIDENTIFIER  = "com.apple.AudioUnit-UI"
  else:
    NSEXTENSIONPOINTIDENTIFIER  = "com.apple.AudioUnit"

  plistpath = projectpath + "/resources/" + BUNDLE_NAME + "-iOS-AUv3-Info.plist"
  auv3 = plistlib.readPlist(plistpath)
#  auv3['AudioUnit Version'] = PLUG_VER_STR
  auv3['CFBundleExecutable'] = BUNDLE_NAME + "AppExtension"
#  auv3['CFBundleGetInfoString'] = CFBundleGetInfoString
  auv3['CFBundleIdentifier'] = BUNDLE_DOMAIN + "." + BUNDLE_MFR + "." + BUNDLE_NAME + "App.AUv3"
  auv3['CFBundleName'] = BUNDLE_NAME + "AppExtension"
  auv3['CFBundleDisplayName'] = BUNDLE_NAME + "AppExtension"
  auv3['CFBundleVersion'] = CFBundleVersion
  auv3['CFBundleShortVersionString'] = CFBundleVersion
#  auv3['LSMinimumSystemVersion'] = "10.12.0"
  auv3['CFBundlePackageType'] = "XPC!"
  auv3['NSExtension'] = dict(
  NSExtensionAttributes = dict(
#                               AudioComponentBundle = "com.AcmeInc.app.IPlugEffect.AUv3.framework",
                               AudioComponents = [{}]),
#                               NSExtensionServiceRoleType = "NSExtensionServiceRoleTypeEditor",
#                               NSExtensionPrincipalClass = "IPlugViewController",
                             NSExtensionMainStoryboard = "IPlugEffect-iOS-MainInterface",
                             NSExtensionPointIdentifier = NSEXTENSIONPOINTIDENTIFIER,
                             )
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'] = [{}]
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['description'] = PLUG_NAME_STR
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['manufacturer'] = PLUG_MFR_UID
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['name'] = PLUG_MFR_NAME_STR + ": " + PLUG_NAME_STR
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['subtype'] = PLUG_UID
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['type'] = COMP_TYPE
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['version'] = PLUG_VER
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['sandboxSafe'] = True
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['tags'] = [{}]

  if PLUG_IS_INSTRUMENT:
    auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['tags'][0] = "Synth"
  else:
    auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['tags'][0] = "Effects"

  plistlib.writePlist(auv3, plistpath)
#  replacestrs(plistpath, "//Apple//", "//Apple Computer//");

# APP

  plistpath = projectpath + "/resources/" + BUNDLE_NAME + "-iOS-Info.plist"
  macOSapp = plistlib.readPlist(plistpath)
  macOSapp['CFBundleExecutable'] = BUNDLE_NAME
#  macOSapp['CFBundleGetInfoString'] = CFBundleGetInfoString
  macOSapp['CFBundleIdentifier'] = BUNDLE_DOMAIN + "." + BUNDLE_MFR + "." + BUNDLE_NAME + "App"
  macOSapp['CFBundleName'] = BUNDLE_NAME
  macOSapp['CFBundleVersion'] = CFBundleVersion
  macOSapp['CFBundleShortVersionString'] = CFBundleVersion
#  macOSapp['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  macOSapp['CFBundlePackageType'] = "APPL"
#  macOSapp['CFBundleSignature'] = PLUG_UID
#  macOSapp['CSResourcesFileMapped'] = CSResourcesFileMapped
  macOSapp['LSApplicationCategoryType'] = "public.app-category.music"
#  macOSapp['CFBundleIconFile'] = BUNDLE_NAME + ".icns"

  plistlib.writePlist(macOSapp, plistpath)
#  replacestrs(plistpath, "//Apple//", "//Apple Computer//");

if __name__ == '__main__':
  main()
