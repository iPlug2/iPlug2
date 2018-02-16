#!/usr/bin/python

# this script will create the info plist files based on config.h

# <?xml version="1.0" encoding="UTF-8"?>
# <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
# <plist version="1.0">
# <array>
# 	<dict>
# 		<key>manufacturer</key>
# 		<string></string>
# 		<key>subtype</key>
# 		<string></string>
# 		<key>type</key>
# 		<string></string>
# 		<key>description</key>
# 		<string></string>
# 		<key>name</key>
# 		<string></string>
# 		<key>version</key>
# 		<integer>65536</integer>
# 		<key>factoryFunction</key>
# 		<string></string>
# 		<key>sandboxSafe</key>
# 		<false/>
# 	</dict>
# </array>
# </plist>

import plistlib, os, datetime, fileinput, glob, sys, string
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

  print "Processing Info.plist files..."

  MAJORSTR = ""
  MINORSTR = "" 
  BUGFIXSTR = ""
  PLUG_VER_STR = ""
  
  BUNDLE_MFR = ""
  BUNDLE_NAME = ""
  PLUG_NAME_STR = ""
  PLUG_MFR_NAME_STR = ""
  PLUG_CHANNEL_IO = ""
  PLUG_COPYRIGHT = ""
  PLUG_UID = ""
  PLUG_MFR_UID = ""
  PLUG_FACTORY = ""
  PLUG_ENTRY = ""
  PLUG_VIEW_ENTRY = ""
  PLUG_IS_INST = 0
  PLUG_DOES_MIDI = 0
  
  # extract values from config.h
  for line in fileinput.input(projectpath + "/config.h", inplace=0):
    if "#define PLUG_VER " in line:
      PLUG_VER_STR = string.lstrip(line, "#define PLUG_VER ")
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
       
    if "#define PLUG_CHANNEL_IO " in line:
      PLUG_CHANNEL_IO = string.lstrip(line, "#define PLUG_CHANNEL_IO ")
      
    if "#define PLUG_COPYRIGHT " in line:
      PLUG_COPYRIGHT = string.lstrip(line, "#define PLUG_COPYRIGHT ")

    if "#define PLUG_UNIQUE_ID " in line:
      PLUG_UID = string.lstrip(line, "#define PLUG_UNIQUE_ID ")
      
    if "#define PLUG_MFR_ID " in line:
      PLUG_MFR_UID = string.lstrip(line, "#define PLUG_MFR_ID ")
    
    if "#define PLUG_ENTRY " in line:
      PLUG_ENTRY = string.lstrip(line, "#define PLUG_ENTRY ")
     
    if "#define PLUG_FACTORY " in line:
      PLUG_FACTORY = string.lstrip(line, "#define PLUG_FACTORY ")
    
    if "#define PLUG_VIEW_ENTRY " in line:
      PLUG_VIEW_ENTRY = string.lstrip(line, "#define PLUG_VIEW_ENTRY")
      
    if "#define PLUG_IS_INST " in line:
      PLUG_IS_INST = int(string.lstrip(line, "#define PLUG_IS_INST "), 16)
    
    if "#define PLUG_DOES_MIDI " in line:
      PLUG_DOES_MIDI = int(string.lstrip(line, "#define PLUG_DOES_MIDI "), 16)
      
  FULLVERSIONSTR = MAJORSTR + "." + MINORSTR + "." + BUGFIXSTR
  
  #strip quotes and newlines
  PLUG_VER_STR = PLUG_VER_STR[0:-1]
  BUNDLE_MFR = BUNDLE_MFR[1:-2]
  BUNDLE_NAME = BUNDLE_NAME[1:-2]
  PLUG_NAME_STR = PLUG_NAME_STR[1:-2]
  PLUG_MFR_NAME_STR = PLUG_MFR_NAME_STR[1:-2]
  PLUG_CHANNEL_IO = PLUG_CHANNEL_IO[1:-2]
  PLUG_COPYRIGHT = PLUG_COPYRIGHT[1:-2]
  PLUG_MFR_UID = PLUG_MFR_UID[1:-2]
  PLUG_UID = PLUG_UID[1:-2]
  PLUG_FACTORY = PLUG_FACTORY[0:-1]
  PLUG_ENTRY = PLUG_ENTRY[0:-1]
  PLUG_VIEW_ENTRY = PLUG_VIEW_ENTRY[0:-1]

  CFBundleGetInfoString = BUNDLE_NAME + " v" + FULLVERSIONSTR + " " + PLUG_COPYRIGHT
  CFBundleVersion = FULLVERSIONSTR
  CFBundlePackageType = "BNDL"
  CSResourcesFileMapped = True
  
  fileinput.close()
  
  LSMinimumSystemVersion = "10.6.0"
  
  BASE_SDK = "macosx10.6"
  DEPLOYMENT_TARGET = "10.7"

  # extract values from common.xcconfig
  for line in fileinput.input(projectpath + "/../../common.xcconfig", inplace=0):
    if not "//" in line:
      if "BASE_SDK = " in line:
        BASE_SDK = string.lstrip(line, "BASE_SDK = ")
      if "DEPLOYMENT_TARGET = " in line:
        DEPLOYMENT_TARGET = string.lstrip(line, "DEPLOYMENT_TARGET = ")

  BASE_SDK = BASE_SDK[0:-1]
  DEPLOYMENT_TARGET = DEPLOYMENT_TARGET[0:-1]
  DEPLOYMENT_TARGET += ".0"
  
  LSMinimumSystemVersion = DEPLOYMENT_TARGET

  plistpath = projectpath + "/resources/" + BUNDLE_NAME + "-VST2-Info.plist"
  vst2 = plistlib.readPlist(plistpath)
  vst2['CFBundleExecutable'] = BUNDLE_NAME
  vst2['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst2['CFBundleIdentifier'] = "com." + BUNDLE_MFR + ".vst." + BUNDLE_NAME + ""
  vst2['CFBundleName'] = BUNDLE_NAME
  vst2['CFBundleVersion'] = CFBundleVersion
  vst2['CFBundleShortVersionString'] = CFBundleVersion
  vst2['LSMinimumSystemVersion'] = LSMinimumSystemVersion
  vst2['CFBundlePackageType'] = CFBundlePackageType
  vst2['CFBundleSignature'] = PLUG_UID
  vst2['CSResourcesFileMapped'] = CSResourcesFileMapped

  plistlib.writePlist(vst2, plistpath)
  replacestrs(plistpath, "//Apple//", "//Apple Computer//");

if __name__ == '__main__':
  main()
