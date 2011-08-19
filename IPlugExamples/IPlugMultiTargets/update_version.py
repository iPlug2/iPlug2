#!/usr/bin/python

# this script will update the versions in plist and installer files to match that in resource.h

import optparse, plistlib, os, datetime
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
	
	plistpath = scriptpath + "/resources/IPlugMultiTargets-OSXAPP-Info.plist"
	app = plistlib.readPlist(plistpath)
	app['CFBundleGetInfoString'] = CFBundleGetInfoString
	app['CFBundleVersion'] = CFBundleVersion
	app['CFBundleShortVersionString'] = CFBundleVersion
	plistlib.writePlist(app, plistpath)
	
	plistpath = scriptpath + "/resources/IPlugMultiTargets-IOSAPP-Info.plist"
	iosapp = plistlib.readPlist(plistpath)
	iosapp['CFBundleGetInfoString'] = CFBundleGetInfoString
	iosapp['CFBundleVersion'] = CFBundleVersion
	iosapp['CFBundleShortVersionString'] = CFBundleVersion
	plistlib.writePlist(iosapp, plistpath)

	plistpath = scriptpath + "/installer/IPlugMultiTargets-plugins.packproj"
	pluginsInstaller = plistlib.readPlist(plistpath)
	pluginsInstaller['Hierarchy']['Attributes']['Settings']['Description']['International']['IFPkgDescriptionVersion'] = FullVersion
	pluginsInstaller['Hierarchy']['Attributes']['Settings']['Display Information']['CFBundleGetInfoString'] = CFBundleGetInfoString
	pluginsInstaller['Hierarchy']['Attributes']['Settings']['Display Information']['CFBundleShortVersionString'] = CFBundleVersion
	pluginsInstaller['Hierarchy']['Attributes']['Settings']['Version']['IFMajorVersion'] = int(o.major)
	pluginsInstaller['Hierarchy']['Attributes']['Settings']['Version']['IFMinorVersion'] = int(o.minor) 

	plistlib.writePlist(pluginsInstaller, plistpath)
	
	plistpath = scriptpath + "/installer/IPlugMultiTargets-all.packproj"
	allInstaller = plistlib.readPlist(plistpath)
	allInstaller['Hierarchy']['Attributes']['Settings']['Description']['International']['IFPkgDescriptionVersion'] = FullVersion
	allInstaller['Hierarchy']['Attributes']['Settings']['Display Information']['CFBundleGetInfoString'] = CFBundleGetInfoString
	allInstaller['Hierarchy']['Attributes']['Settings']['Display Information']['CFBundleShortVersionString'] = CFBundleVersion
	allInstaller['Hierarchy']['Attributes']['Settings']['Version']['IFMajorVersion'] = int(o.major)
	allInstaller['Hierarchy']['Attributes']['Settings']['Version']['IFMinorVersion'] = int(o.minor) 

	plistlib.writePlist(allInstaller, plistpath)

if __name__ == '__main__':
	main()