#!/usr/bin/python

# this script will update the versions in packages and innosetup installer files to match that in config.h

import plistlib, os, datetime, fileinput, glob, sys, string
scriptpath = os.path.dirname(os.path.realpath(__file__))
projectpath = os.path.abspath(os.path.join(scriptpath, os.pardir))

IPLUG2_ROOT = "../../.."

sys.path.insert(0, os.path.join(os.getcwd(), IPLUG2_ROOT + '/Scripts'))

from parse_config import parse_config

def replacestrs(filename, s, r):
  files = glob.glob(filename)
  
  for line in fileinput.input(files,inplace=1):
    string.find(line, s)
    line = line.replace(s, r)
    sys.stdout.write(line)

def main():
  demo = 0
  
  if len(sys.argv) != 2:
    print("Usage: update_installer_version.py demo(0 or 1)")
    sys.exit(1)
  else:
    demo=int(sys.argv[1])

  config = parse_config(projectpath)

# MAC INSTALLER

  print "Updating Mac Installer version info..."
  
  plistpath = projectpath + "/installer/" + config['BUNDLE_NAME'] + ".pkgproj"
  installer = plistlib.readPlist(plistpath)
  
  # range  = number of items in the installer (VST 2, VST 3, app, audiounit, aax)
  for x in range(0,5):
    installer['PACKAGES'][x]['PACKAGE_SETTINGS']['VERSION'] = config['FULL_VER_STR']

  if demo:
    installer['PROJECT']['PROJECT_PRESENTATION']['TITLE']['LOCALIZATIONS'][0]['VALUE'] = config['BUNDLE_NAME'] + " Demo"
    installer['PROJECT']['PROJECT_PRESENTATION']['INTRODUCTION']['LOCALIZATIONS'][0]['VALUE']['PATH'] = "intro-demo.rtf"
  else:
    installer['PROJECT']['PROJECT_PRESENTATION']['TITLE']['LOCALIZATIONS'][0]['VALUE'] = config['BUNDLE_NAME']
    installer['PROJECT']['PROJECT_PRESENTATION']['INTRODUCTION']['LOCALIZATIONS'][0]['VALUE']['PATH'] = "intro.rtf"

  plistlib.writePlist(installer, plistpath)
#   replacestrs(plistpath, "//Apple//", "//Apple Computer//");
  
# WIN INSTALLER
  print "Updating Windows Installer version info..."
  
  for line in fileinput.input(projectpath + "/installer/" + config['BUNDLE_NAME'] + ".iss",inplace=1):
    if "AppVersion" in line:
      line="AppVersion=" + config['FULL_VER_STR'] + "\n"
    if "OutputBaseFilename" in line:
      if demo:
        line="OutputBaseFilename=IPlugEffect Demo Installer\n"
      else:
        line="OutputBaseFilename=IPlugEffect Installer\n"
        
    if 'Source: "readme' in line:
     if demo:
      line='Source: "readme-win-demo.rtf"; DestDir: "{app}"; DestName: "readme.rtf"; Flags: isreadme\n'
     else:
      line='Source: "readme-win.rtf"; DestDir: "{app}"; DestName: "readme.rtf"; Flags: isreadme\n'
    
    if "WelcomeLabel1" in line:
     if demo:
       line="WelcomeLabel1=Welcome to the IPlugEffect Demo installer\n"
     else:
       line="WelcomeLabel1=Welcome to the IPlugEffect installer\n"
       
    if "SetupWindowTitle" in line:
     if demo:
       line="SetupWindowTitle=IPlugEffect Demo installer\n"
     else:
       line="SetupWindowTitle=IPlugEffect installer\n"
       
    sys.stdout.write(line)
    
    
    
if __name__ == '__main__':
  main()
