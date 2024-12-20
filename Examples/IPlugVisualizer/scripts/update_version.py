#!/usr/bin/python3

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

  print("update_version.py - setting version to " + config['FULL_VER_STR'])
  print("Updating plist version info...")
  
  plistpath = scriptpath + "/resources/IPlugVisualizer-VST2-Info.plist"
  with open(plistpath, 'rb') as fp:
    vst2 = plistlib.load(fp)
  vst2['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst2['CFBundleVersion'] = CFBundleVersion
  vst2['CFBundleShortVersionString'] = CFBundleVersion
  with open(plistpath, 'wb') as fp:
    plistlib.dump(vst2, fp)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")
  
  plistpath = scriptpath + "/resources/IPlugVisualizer-AU-Info.plist"
  with open(plistpath, 'rb') as fp:
    au = plistlib.load(fp)
  au['CFBundleGetInfoString'] = CFBundleGetInfoString
  au['CFBundleVersion'] = CFBundleVersion
  au['CFBundleShortVersionString'] = CFBundleVersion
  with open(plistpath, 'wb') as fp:
    plistlib.dump(au, fp)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")
  
  plistpath = scriptpath + "/resources/IPlugVisualizer-VST3-Info.plist"
  with open(plistpath, 'rb') as fp:
    vst3 = plistlib.load(fp)
  vst3['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst3['CFBundleVersion'] = CFBundleVersion
  vst3['CFBundleShortVersionString'] = CFBundleVersion
  with open(plistpath, 'wb') as fp:
    plistlib.dump(vst3, fp)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")
  
  plistpath = scriptpath + "/resources/IPlugVisualizer-macOS-Info.plist"
  with open(plistpath, 'rb') as fp:
    app = plistlib.load(fp)
  app['CFBundleGetInfoString'] = CFBundleGetInfoString
  app['CFBundleVersion'] = CFBundleVersion
  app['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(app, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")
  
  plistpath = scriptpath + "/resources/IPlugVisualizer-AAX-Info.plist"
  with open(plistpath, 'rb') as fp:
    aax = plistlib.load(fp)
  aax['CFBundleGetInfoString'] = CFBundleGetInfoString
  aax['CFBundleVersion'] = CFBundleVersion
  aax['CFBundleShortVersionString'] = CFBundleVersion
  with open(plistpath, 'wb') as fp:
    plistlib.dump(aax, fp)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")

  print("Updating Mac Installer version info...")
  
  plistpath = scriptpath + "/installer/IPlugVisualizer.pkgproj"
  with open(plistpath, 'rb') as fp:
    installer = plistlib.load(fp)
  
  for x in range(0,5):
    installer['PACKAGES'][x]['PACKAGE_SETTINGS']['VERSION'] = config['FULL_VER_STR']
  
  with open(plistpath, 'wb') as fp:
    plistlib.dump(installer, fp)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")
  
  print("Updating Windows Installer version info...")
  
  for line in fileinput.input(scriptpath + "/installer/IPlugVisualizer.iss",inplace=1):
    if "AppVersion" in line:
      line="AppVersion=" + config['FULL_VER_STR'] + "\n"
    sys.stdout.write(line)

if __name__ == '__main__':
  main()
