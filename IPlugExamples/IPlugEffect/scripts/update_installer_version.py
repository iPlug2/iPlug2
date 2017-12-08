#!/usr/bin/python

# this script will update the versions in packages and innosetup installer files to match that in config.h

import plistlib, os, datetime, fileinput, glob, sys, string
scriptpath = os.path.dirname(os.path.realpath(__file__))
projectpath = os.path.abspath(os.path.join(scriptpath, os.pardir))

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

  MajorStr = ""
  MinorStr = "" 
  BugfixStr = ""
  BUNDLE_NAME = ""
  
  # extract values from config.h
  for line in fileinput.input(projectpath + "config.h", inplace=0):
    if "#define PLUG_VERSION_HEX " in line:
      PLUG_VERSION_HEX_STR = string.lstrip(line, "#define PLUG_VERSION_HEX ")
      PLUG_VERSION_HEX = int(PLUG_VERSION_HEX_STR, 16)
      MAJOR = PLUG_VERSION_HEX & 0xFFFF0000
      MAJORSTR = str(MAJOR >> 16)
      MINOR = PLUG_VERSION_HEX & 0x0000FF00
      MINORSTR = str(MINOR >> 8)
      BUGFIXSTR = str(PLUG_VERSION_HEX & 0x000000FF)
      
    if "#define BUNDLE_NAME " in line:
      BUNDLE_NAME = string.lstrip(line, "#define BUNDLE_NAME ")
  
  FULLVERSIONSTR = MAJORSTR + "." + MINORSTR + "." + BUGFIXSTR
  
  #strip quotes and newlines
  PLUG_VERSION_HEX_STR = PLUG_VERSION_HEX_STR[0:-1]
  BUNDLE_NAME = BUNDLE_NAME[1:-2]

# MAC INSTALLER

  print "Updating Mac Installer version info..."
  
  plistpath = projectpath + "/installer/" + BUNDLE_NAME + ".pkgproj"
  installer = plistlib.readPlist(plistpath)
  
  for x in range(0,9):
    installer['PACKAGES'][x]['PACKAGE_SETTINGS']['VERSION'] = FULLVERSIONSTR

  if demo:
    installer['PROJECT']['PROJECT_PRESENTATION']['TITLE']['LOCALIZATIONS'][0]['VALUE'] = BUNDLE_NAME + " Demo"
    installer['PROJECT']['PROJECT_PRESENTATION']['INTRODUCTION']['LOCALIZATIONS'][0]['VALUE']['PATH'] = "intro-demo.rtf"
  else:
    installer['PROJECT']['PROJECT_PRESENTATION']['TITLE']['LOCALIZATIONS'][0]['VALUE'] = BUNDLE_NAME
    installer['PROJECT']['PROJECT_PRESENTATION']['INTRODUCTION']['LOCALIZATIONS'][0]['VALUE']['PATH'] = "intro.rtf"

  plistlib.writePlist(installer, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");
  
# WIN INSTALLER
  print "Updating Windows Installer version info..."
  
  for line in fileinput.input(projectpath + "/installer/" + BUNDLE_NAME + ".iss",inplace=1):
    if "AppVersion" in line:
      line="AppVersion=" + FULLVERSIONSTR + "\n"
    if "OutputBaseFilename" in line:
      if demo:
        line="OutputBaseFilename=VirtualCZ Demo Installer\n"
      else:
        line="OutputBaseFilename=VirtualCZ Installer\n"
        
    if 'Source: "readme' in line:
     if demo:
      line='Source: "readme-win-demo.rtf"; DestDir: "{app}"; DestName: "readme.rtf"; Flags: isreadme\n'
     else:
      line='Source: "readme-win.rtf"; DestDir: "{app}"; DestName: "readme.rtf"; Flags: isreadme\n'
    
    if "WelcomeLabel1" in line:
     if demo:
       line="WelcomeLabel1=Welcome to the VirtualCZ Demo installer\n"
     else:
       line="WelcomeLabel1=Welcome to the VirtualCZ installer\n"
       
    if "SetupWindowTitle" in line:
     if demo:
       line="SetupWindowTitle=VirtualCZ Demo installer\n"
     else:
       line="SetupWindowTitle=VirtualCZ installer\n"
       
    sys.stdout.write(line)
    
    
    
if __name__ == '__main__':
  main()