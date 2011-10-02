#!/usr/bin/python

# this script will update the versions in plist and installer files to match that in resource.h

import optparse, plistlib, os, datetime, fileinput, sys
scriptpath = os.path.dirname(os.path.realpath(__file__))

def main():
  p = optparse.OptionParser()
  p.add_option('--major', '-b', default="0")
  p.add_option('--minor', '-s', default="0")
  p.add_option('--bug', '-t', default="0")
  o, arguments = p.parse_args()
  
  FullVersion = o.major + "." + o.minor + "." + o.bug
  CFBundleGetInfoString = FullVersion + ", Copyright DEFAULT_MFR, 2011"
  CFBundleVersion = FullVersion
  
  plistpath = scriptpath + "/resources/IPlugMultiTargets-VST2-Info.plist"
  vst2 = plistlib.readPlist(plistpath)
  vst2['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst2['CFBundleVersion'] = CFBundleVersion
  vst2['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(vst2, plistpath)
  
  plistpath = scriptpath + "/resources/IPlugMultiTargets-AU-Info.plist"
  au = plistlib.readPlist(plistpath)
  au['CFBundleGetInfoString'] = CFBundleGetInfoString
  au['CFBundleVersion'] = CFBundleVersion
  au['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(au, plistpath)
  
  plistpath = scriptpath + "/resources/IPlugMultiTargets-VST3-Info.plist"
  vst3 = plistlib.readPlist(plistpath)
  vst3['CFBundleGetInfoString'] = CFBundleGetInfoString
  vst3['CFBundleVersion'] = CFBundleVersion
  vst3['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(vst3, plistpath)
  
  plistpath = scriptpath + "/resources/IPlugMultiTargets-OSXAPP-Info.plist"
  app = plistlib.readPlist(plistpath)
  app['CFBundleGetInfoString'] = CFBundleGetInfoString
  app['CFBundleVersion'] = CFBundleVersion
  app['CFBundleShortVersionString'] = CFBundleVersion
  plistlib.writePlist(app, plistpath)

#   plistpath = scriptpath + "/resources/IPlugMultiTargets-IOSAPP-Info.plist"
#   iosapp = plistlib.readPlist(plistpath)
#   iosapp['CFBundleGetInfoString'] = CFBundleGetInfoString
#   iosapp['CFBundleVersion'] = CFBundleVersion
#   iosapp['CFBundleShortVersionString'] = CFBundleVersion
#   plistlib.writePlist(iosapp, plistpath)
  
  plistpath = scriptpath + "/installer/IPlugMultiTargets.packproj"
  installer = plistlib.readPlist(plistpath)
  installer['Hierarchy']['Attributes']['Settings']['Description']['International']['IFPkgDescriptionVersion'] = FullVersion
  installer['Hierarchy']['Attributes']['Settings']['Display Information']['CFBundleGetInfoString'] = CFBundleGetInfoString
  installer['Hierarchy']['Attributes']['Settings']['Display Information']['CFBundleShortVersionString'] = CFBundleVersion
  installer['Hierarchy']['Attributes']['Settings']['Version']['IFMajorVersion'] = int(o.major)
  installer['Hierarchy']['Attributes']['Settings']['Version']['IFMinorVersion'] = int(o.minor)
  
  for x in range(0,5):
    installer['Hierarchy']['Attributes']['Components'][x]['Attributes']['Settings']['Description']['International']['IFPkgDescriptionVersion'] = FullVersion
    installer['Hierarchy']['Attributes']['Components'][x]['Attributes']['Settings']['Display Information']['CFBundleGetInfoString'] = CFBundleGetInfoString
    installer['Hierarchy']['Attributes']['Components'][x]['Attributes']['Settings']['Display Information']['CFBundleShortVersionString'] = CFBundleVersion
    installer['Hierarchy']['Attributes']['Components'][x]['Attributes']['Settings']['Version']['IFMajorVersion'] = int(o.major)
    installer['Hierarchy']['Attributes']['Components'][x]['Attributes']['Settings']['Version']['IFMinorVersion'] = int(o.minor) 

  plistlib.writePlist(installer, plistpath)
  
  #do Windows (innosetup) installer version
  
  for line in fileinput.input(scriptpath + "/installer/IPlugMultiTargets.iss",inplace=1):
    if "AppVersion" in line:
      line="AppVersion=" + o.major + "." + o.minor + "." + o.bug + "\n"
    sys.stdout.write(line)
  

if __name__ == '__main__':
  main()