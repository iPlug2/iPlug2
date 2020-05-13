#!/usr/bin/python

# this script will update the versions in plist and installer files to match that in config.h

import plistlib, os, datetime, fileinput, glob, sys, string, shutil

scriptpath = os.path.dirname(os.path.realpath(__file__))
projectpath = os.path.abspath(os.path.join(scriptpath, os.pardir))

IPLUG2_ROOT = "../../.."

sys.path.insert(0, os.path.join(os.getcwd(), IPLUG2_ROOT + '/Scripts'))

from parse_config import parse_config, parse_xcconfig

def replacestrs(filename, s, r):
  files = glob.glob(filename)
  
  for line in fileinput.input(files,inplace=1):
    string.find(line, s)
    line = line.replace(s, r)
    sys.stdout.write(line)

def main():

  config = parse_config(projectpath)
    
  today = datetime.date.today()
  
  CFBundleGetInfoString = config['BUNDLE_NAME'] + " v" + config['FULL_VER_STR'] + " " + config['PLUG_COPYRIGHT_STR']
  CFBundleVersion = config['FULL_VER_STR']

  print "update_version.py - setting version to " + config['FULL_VER_STR']
  print "Updating plist version info..."
  
  plistpath = scriptpath + "/resources/IPlugEffect-VST2-Info.plist"
  vst2 = plistlib.readPlist(plistpath)
  vst2['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst2['CFBundleVersion'] = CFBundleVersion
  vst2['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(vst2, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");
  
  plistpath = scriptpath + "/resources/IPlugEffect-AU-Info.plist"
  au = plistlib.readPlist(plistpath)
  au['CFBundleGetInfoString'] = CFBundleGetInfoString
  au['CFBundleVersion'] = CFBundleVersion
  au['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(au, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");
  
  plistpath = scriptpath + "/resources/IPlugEffect-VST3-Info.plist"
  vst3 = plistlib.readPlist(plistpath)
  vst3['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst3['CFBundleVersion'] = CFBundleVersion
  vst3['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(vst3, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");
  
  plistpath = scriptpath + "/resources/IPlugEffect-macOS-Info.plist"
  app = plistlib.readPlist(plistpath)
  app['CFBundleGetInfoString'] = CFBundleGetInfoString
  app['CFBundleVersion'] = CFBundleVersion
  app['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(app, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");
  
  plistpath = scriptpath + "/resources/IPlugEffect-AAX-Info.plist"
  aax = plistlib.readPlist(plistpath)
  aax['CFBundleGetInfoString'] = CFBundleGetInfoString
  aax['CFBundleVersion'] = CFBundleVersion
  aax['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(aax, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");

  print "Updating Mac Installer version info..."
  
  plistpath = scriptpath + "/installer/IPlugEffect.pkgproj"
  installer = plistlib.readPlist(plistpath)
  
  for x in range(0,5):
    installer['PACKAGES'][x]['PACKAGE_SETTINGS']['VERSION'] = config['FULL_VER_STR']
  
  plistlib.writePlist(installer, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");
  
  print "Updating Windows Installer version info..."
  
  for line in fileinput.input(scriptpath + "/installer/IPlugEffect.iss",inplace=1):
    if "AppVersion" in line:
      line="AppVersion=" + config['FULL_VER_STR'] + "\n"
    sys.stdout.write(line)

if __name__ == '__main__':
  main()
