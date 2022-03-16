#!/usr/bin/python3

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
  for line in fileinput.input(projectpath + "/config.h", inplace=0):
    if "#define PLUG_VERSION_HEX " in line:
      PLUG_VERSION_STR = string.lstrip(line, "#define PLUG_VERSION_HEX ")
      PLUG_VERSION = int(PLUG_VERSION_STR, 16)
      MAJOR = PLUG_VERSION & 0xFFFF0000
      MAJORSTR = str(MAJOR >> 16)
      MINOR = PLUG_VERSION & 0x0000FF00
      MINORSTR = str(MINOR >> 8)
      BUGFIXSTR = str(PLUG_VERSION & 0x000000FF)
      
    if "#define BUNDLE_NAME " in line:
      BUNDLE_NAME = string.lstrip(line, "#define BUNDLE_NAME ")
  
  FULLVERSIONSTR = MAJORSTR + "." + MINORSTR + "." + BUGFIXSTR
  
  #strip quotes and newlines
  PLUG_VERSION_STR = PLUG_VERSION_STR[0:-1]
  BUNDLE_NAME = BUNDLE_NAME[1:-2]

# MAC INSTALLER

  print("Updating Mac Installer version info...")
  
  plistpath = projectpath + "/installer/" + BUNDLE_NAME + ".pkgproj"
  with open(plistpath, 'rb') as fp:
    installer = plistlib.load(fp)
  
  # range  = number of items in the installer (VST 2, VST 3, app, audiounit, aax)
  for x in range(0,5):
    installer['PACKAGES'][x]['PACKAGE_SETTINGS']['VERSION'] = FULLVERSIONSTR

  if demo:
    installer['PROJECT']['PROJECT_PRESENTATION']['TITLE']['LOCALIZATIONS'][0]['VALUE'] = BUNDLE_NAME + " Demo"
    installer['PROJECT']['PROJECT_PRESENTATION']['INTRODUCTION']['LOCALIZATIONS'][0]['VALUE']['PATH'] = "intro-demo.rtf"
  else:
    installer['PROJECT']['PROJECT_PRESENTATION']['TITLE']['LOCALIZATIONS'][0]['VALUE'] = BUNDLE_NAME
    installer['PROJECT']['PROJECT_PRESENTATION']['INTRODUCTION']['LOCALIZATIONS'][0]['VALUE']['PATH'] = "intro.rtf"

  with open(plistpath, 'wb') as fp:
    plistlib.dump(installer, fp)
#   replacestrs(plistpath, "//Apple//", "//Apple Computer//")
  
# WIN INSTALLER
  print("Updating Windows Installer version info...")
  
  for line in fileinput.input(projectpath + "/installer/" + BUNDLE_NAME + ".iss",inplace=1):
    if "AppVersion" in line:
      line="AppVersion=" + FULLVERSIONSTR + "\n"
    if "OutputBaseFilename" in line:
      if demo:
        line="OutputBaseFilename=IPlugReaperExtension Demo Installer\n"
      else:
        line="OutputBaseFilename=IPlugReaperExtension Installer\n"
        
    if 'Source: "readme' in line:
     if demo:
      line='Source: "readme-win-demo.rtf"; DestDir: "{app}"; DestName: "readme.rtf"; Flags: isreadme\n'
     else:
      line='Source: "readme-win.rtf"; DestDir: "{app}"; DestName: "readme.rtf"; Flags: isreadme\n'
    
    if "WelcomeLabel1" in line:
     if demo:
       line="WelcomeLabel1=Welcome to the IPlugReaperExtension Demo installer\n"
     else:
       line="WelcomeLabel1=Welcome to the IPlugReaperExtension installer\n"
       
    if "SetupWindowTitle" in line:
     if demo:
       line="SetupWindowTitle=IPlugReaperExtension Demo installer\n"
     else:
       line="SetupWindowTitle=IPlugReaperExtension installer\n"
       
    sys.stdout.write(line)
    
    
    
if __name__ == '__main__':
  main()