#!/usr/bin/python3

# this script will update the versions in plist and installer files to match that in config.h

import plistlib, os, datetime, fileinput, glob, sys, string
scriptpath = os.path.dirname(os.path.realpath(__file__))

def replacestrs(filename, s, r):
  files = glob.glob(filename)
  
  for line in fileinput.input(files,inplace=1):
    string.find(line, s)
    line = line.replace(s, r)
    sys.stdout.write(line)

def main():

  MajorStr = ""
  MinorStr = "" 
  BugfixStr = ""

  for line in fileinput.input(scriptpath + "/config.h",inplace=0):
    if "#define PLUG_VER " in line:
      FullVersion = int(string.lstrip(line, "#define PLUG_VER "), 16)
      major = FullVersion & 0xFFFF0000
      MajorStr = str(major >> 16)
      minor = FullVersion & 0x0000FF00
      MinorStr = str(minor >> 8)
      BugfixStr = str(FullVersion & 0x000000FF)
      
  
  FullVersionStr = MajorStr + "." + MinorStr + "." + BugfixStr
  
  today = datetime.date.today()
  CFBundleGetInfoString = FullVersionStr + ", Copyright AcmeInc, " + str(today.year)
  CFBundleVersion = FullVersionStr
  
  print "update_version.py - setting version to " + FullVersionStr
  print("Updating plist version info...")
  
  plistpath = scriptpath + "/resources/IPlugReaperExtension-VST2-Info.plist"
  with open(plistpath, 'rb') as fp:
    vst2 = plistlib.load(fp)
  vst2['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst2['CFBundleVersion'] = CFBundleVersion
  vst2['CFBundleShortVersionString'] = CFBundleVersion
  with open(plistpath, 'wb') as fp:
    plistlib.dump(vst2, fp)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")
  
  plistpath = scriptpath + "/resources/IPlugReaperExtension-AU-Info.plist"
  with open(plistpath, 'rb') as fp:
    au = plistlib.load(fp)
  au['CFBundleGetInfoString'] = CFBundleGetInfoString
  au['CFBundleVersion'] = CFBundleVersion
  au['CFBundleShortVersionString'] = CFBundleVersion
  with open(plistpath, 'wb') as fp:
    plistlib.dump(au, fp)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")
  
  plistpath = scriptpath + "/resources/IPlugReaperExtension-VST3-Info.plist"
  with open(plistpath, 'rb') as fp:
    vst3 = plistlib.load(fp)
  vst3['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst3['CFBundleVersion'] = CFBundleVersion
  vst3['CFBundleShortVersionString'] = CFBundleVersion
  with open(plistpath, 'wb') as fp:
    plistlib.dump(vst3, fp)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")
  
  plistpath = scriptpath + "/resources/IPlugReaperExtension-OSXAPP-Info.plist"
  with open(plistpath, 'rb') as fp:
    app = plistlib.load(fp)
  app['CFBundleGetInfoString'] = CFBundleGetInfoString
  app['CFBundleVersion'] = CFBundleVersion
  app['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(app, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")
  
  plistpath = scriptpath + "/resources/IPlugReaperExtension-AAX-Info.plist"
  with open(plistpath, 'rb') as fp:
    aax = plistlib.load(fp)
  aax['CFBundleGetInfoString'] = CFBundleGetInfoString
  aax['CFBundleVersion'] = CFBundleVersion
  aax['CFBundleShortVersionString'] = CFBundleVersion
  with open(plistpath, 'wb') as fp:
    plistlib.dump(aax, fp)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")

  print("Updating Mac Installer version info...")
  
  plistpath = scriptpath + "/installer/IPlugReaperExtension.pkgproj"
  with open(plistpath, 'rb') as fp:
    installer = plistlib.load(fp)
  
  for x in range(0,6):
    installer['PACKAGES'][x]['PACKAGE_SETTINGS']['VERSION'] = FullVersionStr
  
  with open(plistpath, 'wb') as fp:
    plistlib.dump(installer, fp)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//")
  
  print("Updating Windows Installer version info...")
  
  for line in fileinput.input(scriptpath + "/installer/IPlugReaperExtension.iss",inplace=1):
    if "AppVersion" in line:
      line="AppVersion=" + FullVersionStr + "\n"
    sys.stdout.write(line)

if __name__ == '__main__':
  main()
