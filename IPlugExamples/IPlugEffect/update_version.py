#!/usr/bin/python

# this script will update the versions in plist and installer files to match that in resource.h

import plistlib, os, datetime, fileinput, sys, string
scriptpath = os.path.dirname(os.path.realpath(__file__))

def main():

  MajorStr = ""
  MinorStr = "" 
  BugfixStr = ""

  for line in fileinput.input(scriptpath + "/resource.h",inplace=0):
    if "#define PLUG_VER " in line:
      FullVersion = int(string.lstrip(line, "#define PLUG_VER "), 16)
      major = FullVersion & 0xFFFF0000
      MajorStr = str(major >> 16)
      minor = FullVersion & 0x0000FF00
      MinorStr = str(minor >> 8)
      BugfixStr = str(FullVersion & 0x000000FF)
      
  
  FullVersionStr = MajorStr + "." + MinorStr + "." + BugfixStr
  CFBundleGetInfoString = FullVersionStr + ", Copyright DEFAULT_MFR, 2011"
  CFBundleVersion = FullVersionStr
  
  print "update_version.py - setting version to " + FullVersionStr
  print "Updating plist version info..."
  
  plistpath = scriptpath + "/resources/IPlugEffect-VST2-Info.plist"
  vst2 = plistlib.readPlist(plistpath)
  vst2['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst2['CFBundleVersion'] = CFBundleVersion
  vst2['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(vst2, plistpath)
  
  plistpath = scriptpath + "/resources/IPlugEffect-AU-Info.plist"
  au = plistlib.readPlist(plistpath)
  au['CFBundleGetInfoString'] = CFBundleGetInfoString
  au['CFBundleVersion'] = CFBundleVersion
  au['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(au, plistpath)
  
  plistpath = scriptpath + "/resources/IPlugEffect-VST3-Info.plist"
  vst3 = plistlib.readPlist(plistpath)
  vst3['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst3['CFBundleVersion'] = CFBundleVersion
  vst3['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(vst3, plistpath)
  
  plistpath = scriptpath + "/resources/IPlugEffect-OSXAPP-Info.plist"
  app = plistlib.readPlist(plistpath)
  app['CFBundleGetInfoString'] = CFBundleGetInfoString
  app['CFBundleVersion'] = CFBundleVersion
  app['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(app, plistpath)

#   plistpath = scriptpath + "/resources/IPlugEffect-IOSAPP-Info.plist"
#   iosapp = plistlib.readPlist(plistpath)
#   iosapp['CFBundleGetInfoString'] = CFBundleGetInfoString
#   iosapp['CFBundleVersion'] = CFBundleVersion
#   iosapp['CFBundleShortVersionString'] = CFBundleVersion
#   plistlib.writePlist(iosapp, plistpath)

  print "Updating Mac Installer version info..."
  
  plistpath = scriptpath + "/installer/IPlugEffect.packproj"
  installer = plistlib.readPlist(plistpath)
  installer['Hierarchy']['Attributes']['Settings']['Description']['International']['IFPkgDescriptionVersion'] = FullVersionStr
  installer['Hierarchy']['Attributes']['Settings']['Display Information']['CFBundleGetInfoString'] = CFBundleGetInfoString
  installer['Hierarchy']['Attributes']['Settings']['Display Information']['CFBundleShortVersionString'] = CFBundleVersion
  installer['Hierarchy']['Attributes']['Settings']['Version']['IFMajorVersion'] = int(MajorStr)
  installer['Hierarchy']['Attributes']['Settings']['Version']['IFMinorVersion'] = int(MinorStr)
  
  for x in range(0,5):
    installer['Hierarchy']['Attributes']['Components'][x]['Attributes']['Settings']['Description']['International']['IFPkgDescriptionVersion'] = FullVersionStr
    installer['Hierarchy']['Attributes']['Components'][x]['Attributes']['Settings']['Display Information']['CFBundleGetInfoString'] = CFBundleGetInfoString
    installer['Hierarchy']['Attributes']['Components'][x]['Attributes']['Settings']['Display Information']['CFBundleShortVersionString'] = CFBundleVersion
    installer['Hierarchy']['Attributes']['Components'][x]['Attributes']['Settings']['Version']['IFMajorVersion'] = int(MajorStr)
    installer['Hierarchy']['Attributes']['Components'][x]['Attributes']['Settings']['Version']['IFMinorVersion'] = int(MinorStr) 

  plistlib.writePlist(installer, plistpath)

  print "Updating Windows Installer version info..."
  
  for line in fileinput.input(scriptpath + "/installer/IPlugEffect.iss",inplace=1):
    if "AppVersion" in line:
      line="AppVersion=" + FullVersionStr + "\n"
    sys.stdout.write(line)

if __name__ == '__main__':
  main()