#!/usr/bin/python

# this script will rename all the xcode oli.pbxuser
# user settings to yourosxusername.pbxuser to provide 
# the default debugging setup for Xcode 3

import os, getpass, glob, shutil
 
path = os.path.dirname(os.path.realpath(__file__))
username = getpass.getuser()

for r,d,f in os.walk(path):
    for files in f:
        if files.endswith(".pbxuser"):
        	noext = os.path.splitext(files)[0]
        	if noext == "oli":
        		shutil.copy(os.path.join(r, files), os.path.join(r, username + ".pbxuser"))