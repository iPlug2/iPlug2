#!/usr/bin/python

# this script will create/update info plist files based on config.h and copy resources to the ~/Music/PLUG_NAME folder

kAudioUnitType_MusicDevice      = "aumu"
kAudioUnitType_MusicEffect      = "aumf"
kAudioUnitType_Effect           = "aufx"

DONT_COPY = ("")

import plistlib, os, datetime, fileinput, glob, sys, string, shutil

scriptpath = os.path.dirname(os.path.realpath(__file__))
projectpath = os.path.abspath(os.path.join(scriptpath, os.pardir))

sys.path.insert(0, projectpath + '/../../scripts/')

from parse_config import parse_config

def main():
  config = parse_config(projectpath)

  CFBundleGetInfoString = config['BUNDLE_NAME'] + " v" + config['FULL_VER_STR'] + " " + config['PLUG_COPYRIGHT']
  CFBundleVersion = config['FULL_VER_STR']
  CFBundlePackageType = "BNDL"
  CSResourcesFileMapped = True

  fileinput.close()

  LSMinimumSystemVersion = "10.7.0"

  BASE_SDK = "macosx10.13"
  DEPLOYMENT_TARGET = "10.7"

  # extract values from common.xcconfig
  for line in fileinput.input(projectpath + "/../../common.xcconfig", inplace=0):
    if not "//" in line:
      if "BASE_SDK_MAC = " in line:
        BASE_SDK = string.lstrip(line, "BASE_SDK_MAC = ")
      if "DEPLOYMENT_TARGET = " in line:
        DEPLOYMENT_TARGET = string.lstrip(line, "DEPLOYMENT_TARGET = ")

  BASE_SDK = BASE_SDK[0:-1]
  DEPLOYMENT_TARGET = DEPLOYMENT_TARGET[0:-1]
  DEPLOYMENT_TARGET += ".0"

  LSMinimumSystemVersion = DEPLOYMENT_TARGET

  print "Copying resources to shared folder..."

  if config['PLUG_SHARED_RESOURCES']:
    dst = os.path.expanduser("~") + "/Music/IPlugEffect/Resources"
    if os.path.exists(dst):
     shutil.rmtree(dst)
     #os.makedirs(dst)

    shutil.copytree(projectpath + "/resources/img/", dst, ignore=shutil.ignore_patterns(*DONT_COPY))

  print "Processing Info.plist files..."

# VST3

  plistpath = projectpath + "/resources/" + config['BUNDLE_NAME'] + "-VST3-Info.plist"
  vst3 = plistlib.readPlist(plistpath)
  vst3['CFBundleExecutable'] = config['BUNDLE_NAME']
  vst3['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst3['CFBundleIdentifier'] = config['BUNDLE_DOMAIN'] + "." + config['BUNDLE_MFR'] + ".vst3." + config['BUNDLE_NAME'] + ""
  vst3['CFBundleName'] = config['BUNDLE_NAME']
  vst3['CFBundleVersion'] = CFBundleVersion
  vst3['CFBundleShortVersionString'] = CFBundleVersion
  vst3['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  vst3['CFBundlePackageType'] = CFBundlePackageType
  vst3['CFBundleSignature'] = config['PLUG_UID']
  vst3['CSResourcesFileMapped'] = CSResourcesFileMapped

  plistlib.writePlist(vst3, plistpath)

# VST2

  plistpath = projectpath + "/resources/" + config['BUNDLE_NAME'] + "-VST2-Info.plist"
  vst2 = plistlib.readPlist(plistpath)
  vst2['CFBundleExecutable'] = config['BUNDLE_NAME']
  vst2['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst2['CFBundleIdentifier'] = config['BUNDLE_DOMAIN'] + "." + config['BUNDLE_MFR'] + ".vst." + config['BUNDLE_NAME'] + ""
  vst2['CFBundleName'] = config['BUNDLE_NAME']
  vst2['CFBundleVersion'] = CFBundleVersion
  vst2['CFBundleShortVersionString'] = CFBundleVersion
  vst2['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  vst2['CFBundlePackageType'] = CFBundlePackageType
  vst2['CFBundleSignature'] = config['PLUG_UID']
  vst2['CSResourcesFileMapped'] = CSResourcesFileMapped

  plistlib.writePlist(vst2, plistpath)

# AUDIOUNIT v2

  plistpath = projectpath + "/resources/" + config['BUNDLE_NAME'] + "-AU-Info.plist"
  au = plistlib.readPlist(plistpath)
  au['CFBundleExecutable'] = config['BUNDLE_NAME']
  au['CFBundleGetInfoString'] = CFBundleGetInfoString
  au['CFBundleIdentifier'] = config['BUNDLE_DOMAIN'] + "." + config['BUNDLE_MFR'] + ".audiounit." + config['BUNDLE_NAME'] + ""
  au['CFBundleName'] = config['BUNDLE_NAME']
  au['CFBundleVersion'] = CFBundleVersion
  au['CFBundleShortVersionString'] = CFBundleVersion
  au['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  au['CFBundlePackageType'] = CFBundlePackageType
  au['CFBundleSignature'] = config['PLUG_UID']
  au['CSResourcesFileMapped'] = CSResourcesFileMapped

  if config['PLUG_IS_INSTRUMENT']:
    COMP_TYPE = kAudioUnitType_MusicDevice
  elif config['PLUG_DOES_MIDI']:
     COMP_TYPE = kAudioUnitType_MusicEffect
  else:
     COMP_TYPE = kAudioUnitType_Effect

  au['AudioUnit Version'] = config['PLUG_VER_HEX']
  au['AudioComponents'] = [{}]
  au['AudioComponents'][0]['description'] = config['PLUG_NAME']
  au['AudioComponents'][0]['factoryFunction'] = config['AUV2_FACTORY']
  au['AudioComponents'][0]['manufacturer'] = config['PLUG_MFR_UID']
  au['AudioComponents'][0]['name'] = config['PLUG_MFR'] + ": " + config['PLUG_NAME']
  au['AudioComponents'][0]['subtype'] = config['PLUG_UID']
  au['AudioComponents'][0]['type'] = COMP_TYPE
  au['AudioComponents'][0]['version'] = config['PLUG_VER_INT']
  au['AudioComponents'][0]['sandboxSafe'] = True

  plistlib.writePlist(au, plistpath)


# AUDIOUNIT v3

  if config['PLUG_HAS_UI']:
    NSEXTENSIONPOINTIDENTIFIER  = "com.apple.AudioUnit-UI"
  else:
    NSEXTENSIONPOINTIDENTIFIER  = "com.apple.AudioUnit"

  plistpath = projectpath + "/resources/" + config['BUNDLE_NAME'] + "-macOS-AUv3-Info.plist"
  auv3 = plistlib.readPlist(plistpath)
  auv3['CFBundleExecutable'] = config['BUNDLE_NAME']
  auv3['CFBundleGetInfoString'] = CFBundleGetInfoString
  auv3['CFBundleIdentifier'] = config['BUNDLE_DOMAIN'] + "." + config['BUNDLE_MFR'] + ".app." + config['BUNDLE_NAME'] + ".AUv3"
  auv3['CFBundleName'] = config['BUNDLE_NAME']
  auv3['CFBundleVersion'] = CFBundleVersion
  auv3['CFBundleShortVersionString'] = CFBundleVersion
  auv3['LSMinimumSystemVersion'] = "10.12.0"
  auv3['CFBundlePackageType'] = "XPC!"
  auv3['NSExtension'] = dict(
  NSExtensionAttributes = dict(
#                               AudioComponentBundle = "com.AcmeInc.app.IPlugEffect.AUv3.framework",
                               AudioComponents = [{}]),
#                               NSExtensionServiceRoleType = "NSExtensionServiceRoleTypeEditor",
  NSExtensionPointIdentifier = NSEXTENSIONPOINTIDENTIFIER,
  NSExtensionPrincipalClass = "IPlugViewController"
                             )
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'] = [{}]
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['description'] = config['PLUG_NAME']
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['manufacturer'] = config['PLUG_MFR_UID']
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['name'] = config['PLUG_MFR'] + ": " + config['PLUG_NAME']
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['subtype'] = config['PLUG_UID']
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['type'] = COMP_TYPE
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['version'] = config['PLUG_VER_INT']
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['sandboxSafe'] = True
  auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['tags'] = [{}]

  if config['PLUG_IS_INSTRUMENT']:
    auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['tags'][0] = "Synth"
  else:
    auv3['NSExtension']['NSExtensionAttributes']['AudioComponents'][0]['tags'][0] = "Effects"

  plistlib.writePlist(auv3, plistpath)

# AAX

  plistpath = projectpath + "/resources/" + config['BUNDLE_NAME'] + "-AAX-Info.plist"
  aax = plistlib.readPlist(plistpath)
  aax['CFBundleExecutable'] = config['BUNDLE_NAME']
  aax['CFBundleGetInfoString'] = CFBundleGetInfoString
  aax['CFBundleIdentifier'] = config['BUNDLE_DOMAIN'] + "." + config['BUNDLE_MFR'] + ".aax." + config['BUNDLE_NAME'] + ""
  aax['CFBundleName'] = config['BUNDLE_NAME']
  aax['CFBundleVersion'] = CFBundleVersion
  aax['CFBundleShortVersionString'] = CFBundleVersion
  aax['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  aax['CSResourcesFileMapped'] = CSResourcesFileMapped

  plistlib.writePlist(aax, plistpath)

# APP

  plistpath = projectpath + "/resources/" + config['BUNDLE_NAME'] + "-macOS-Info.plist"
  macOSapp = plistlib.readPlist(plistpath)
  macOSapp['CFBundleExecutable'] = config['BUNDLE_NAME']
  macOSapp['CFBundleGetInfoString'] = CFBundleGetInfoString
  macOSapp['CFBundleIdentifier'] = config['BUNDLE_DOMAIN'] + "." + config['BUNDLE_MFR'] + ".app." + config['BUNDLE_NAME'] + ""
  macOSapp['CFBundleName'] = config['BUNDLE_NAME']
  macOSapp['CFBundleVersion'] = CFBundleVersion
  macOSapp['CFBundleShortVersionString'] = CFBundleVersion
  macOSapp['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  macOSapp['CFBundlePackageType'] = CFBundlePackageType
  macOSapp['CFBundleSignature'] = config['PLUG_UID']
  macOSapp['CSResourcesFileMapped'] = CSResourcesFileMapped
  macOSapp['NSPrincipalClass'] = "SWELLApplication"
  macOSapp['NSMainNibFile'] = "IPlugEffect-macOS-MainMenu"
  macOSapp['LSApplicationCategoryType'] = "public.app-category.music"
  macOSapp['CFBundleIconFile'] = config['BUNDLE_NAME'] + ".icns"

  plistlib.writePlist(macOSapp, plistpath)


#  print "Processing .exp symbol export file for audiounit v2 entry points..."

#  expfile = open(config['BUNDLE_NAME'] + ".exp", "w")
#  expfile.write("_" + config['AUV2_FACTORY'] + "\n")
#  #if !AU_NO_COMPONENT_ENTRY
#  expfile.write("_" + AUV2_ENTRY + "\n")
#  expfile.close()

if __name__ == '__main__':
  main()
