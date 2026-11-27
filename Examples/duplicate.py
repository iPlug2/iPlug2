#!/usr/bin/python3

# Python shell script for Duplicating iPlug2 Projects
# Oli Larkin 2012-2024 http://www.olilarkin.co.uk
# License: WTFPL http://sam.zoy.org/wtfpl/COPYING
# Modified from this script by Bibha Tripathi http://code.activestate.com/recipes/435904-sedawk-python-script-to-rename-subdirectories-of-a/
# Author accepts no responsibilty for wiping your hd

# NOTES:
# should work with Python2 or Python3
# not designed to be fool proof- think carefully about what you choose for a project name
# best to stick to standard characters in your project names - avoid spaces, numbers and dots
# windows users need to install python and set it up so you can run it from the command line
# see http://www.voidspace.org.uk/python/articles/command_line.shtml
# this involves adding the python folder e.g. C:\Python27\ to your %PATH% environment variable

# USAGE:
# duplicate.py [inputprojectname] [outputprojectname] [manufacturername] (outputpath)

# TODO:
# - indentation of directory structure
# - variable manufacturer name


from __future__ import generators

import fileinput, glob, string, sys, os, re, uuid, pprint, random, base64, binascii, struct
from shutil import copy, copytree, ignore_patterns, rmtree
from os.path import join

scriptpath = os.path.dirname(os.path.realpath(__file__))

sys.path.insert(0, scriptpath + '/../Scripts/')

from parse_config import parse_config, parse_xcconfig, set_uniqueid

VERSION = "0.96"

# binary files that we don't want to do find and replace inside
FILTERED_FILE_EXTENSIONS = [".ico",".icns", ".pdf", ".png", ".zip", ".exe", ".wav", ".aif"]
# files that we don't want to duplicate
DONT_COPY = ("node_modules", ".vs", "*.exe", "*.dmg", "*.pkg", "*.mpkg", "*.svn", "*.ncb", "*.suo", "*sdf", "ipch", "build-*", "*.layout", "*.depend", ".DS_Store", "xcuserdata", "*.aps", ".reapeaks")

SUBFOLDERS_TO_SEARCH = [
"projects",
"config",
"resources",
"installer",
"scripts",
"manual",
"xcschemes",
"xcshareddata",
"xcuserdata",
"en-osx.lproj",
"project.xcworkspace",
"Images.xcassets",
"web-ui",
"ui",
"UI",
"DSP"
]

def randomFourChar(chars=string.ascii_letters + string.digits):
  return ''.join(random.choice(chars) for _ in range(4))

def checkdirname(name, searchproject):
  "check if directory name matches with the given pattern"
  print("")
  if name == searchproject:
    return True
  else:
    return False

def replacestrs(filename, s, r):
  files = glob.glob(filename)

  for line in fileinput.input(files,inplace=1):
    line = line.replace(s, r)
    sys.stdout.write(line)

def replacestrsChop(filename, s, r):
  files = glob.glob(filename)

  for line in fileinput.input(files,inplace=1):
    if(line.startswith(s)):
      line = r + "\n"
    sys.stdout.write(line)

def fourCharHex(fourcc):
  return ''.join('%02X' % b for b in bytearray(fourcc.encode('ascii')))

def vst3ProcessorGUID(mfrid, uniqueid):
  "hex string of the default VST3_PROCESSOR_UID, see IPlug_include_in_plug_src.h"
  return "F2AEE70D00DE4F4E" + fourCharHex(mfrid) + fourCharHex(uniqueid)

def reaperVST3ID(guid):
  "the integer REAPER stores alongside a VST3 class ID: 31-bit FNV-1a hash of the GUID in COM byte order"
  b = bytearray(binascii.unhexlify(guid))
  com = b[3::-1] + b[5:3:-1] + b[7:5:-1] + b[8:]
  h = 0x811c9dc5
  for c in com:
    h = ((h ^ c) * 0x01000193) & 0xffffffff
  return h & 0x7fffffff

def fixreaperproject(filename, projectname, guid):
  "point the VST3 FX for projectname in a REAPER project at the class ID guid"
  vstline = re.compile(br'^(\s*<VST "VST3[^"]*" ' + re.escape(projectname.encode('ascii')) + br'\.vst3 \d+ "[^"]*" )\d+\{[0-9A-Fa-f]{32}\}')
  newid = reaperVST3ID(guid)
  # the FX state starts with the same ID (little endian) and a 2 byte magic number, which is exactly 8 base64 chars
  statestart = base64.b64encode(struct.pack("<I", newid) + b"\xee\x5e")
  found = False
  fixstate = False

  with open(filename, 'rb') as f:
    lines = f.readlines()

  for i, line in enumerate(lines):
    if fixstate:
      fixstate = False
      stripped = line.lstrip()
      indent = line[:len(line) - len(stripped)]
      if len(stripped) >= 8 and base64.b64decode(stripped[:8])[4:] == b"\xee\x5e":
        lines[i] = indent + statestart + stripped[8:]
    elif vstline.match(line):
      lines[i] = vstline.sub(lambda m: m.group(1) + str(newid).encode('ascii') + b"{" + guid.encode('ascii') + b"}", line)
      found = True
      fixstate = True

  if found:
    with open(filename, 'wb') as f:
      f.writelines(lines)

  return found

def dirwalk(dir, searchproject, replaceproject, searchman, replaceman, oldroot= "", newroot=""):
  for f in os.listdir(dir):
    fullpath = os.path.join(dir, f)

    if os.path.isdir(fullpath) and not os.path.islink(fullpath):
      if checkdirname(f, searchproject + "-macOS.xcodeproj"):
        os.rename(fullpath, os.path.join(dir, replaceproject + "-macOS.xcodeproj"))
        fullpath = os.path.join(dir, replaceproject + "-macOS.xcodeproj")

        print("recursing in macOS xcode project directory: ")
        for x in dirwalk(fullpath, searchproject, replaceproject, searchman, replaceman, oldroot, newroot):
          yield x
      elif checkdirname(f, searchproject + "-iOS.xcodeproj"):
        os.rename(fullpath, os.path.join(dir, replaceproject + "-iOS.xcodeproj"))
        fullpath = os.path.join(dir, replaceproject + "-iOS.xcodeproj")

        print("recursing in iOS xcode project directory: ")
        for x in dirwalk(fullpath, searchproject, replaceproject, searchman, replaceman, oldroot, newroot):
          yield x
      elif checkdirname(f, searchproject + ".xcworkspace"):
        os.rename(fullpath, os.path.join(dir, replaceproject + ".xcworkspace"))
        fullpath = os.path.join(dir, replaceproject + ".xcworkspace")

        print("recursing in main xcode workspace directory: ")
        for x in dirwalk(fullpath, searchproject, replaceproject, searchman, replaceman, oldroot, newroot):
          yield x
      elif checkdirname(f, searchproject + "iOSAppIcon.appiconset"):
        os.rename(fullpath, os.path.join(dir, replaceproject + "iOSAppIcon.appiconset"))
        fullpath = os.path.join(dir, replaceproject + "iOSAppIcon.appiconset")

        print("recursing in iOSAppIcon directory: ")
        for x in dirwalk(fullpath, searchproject, replaceproject, searchman, replaceman, oldroot, newroot):
          yield x
      elif (f in SUBFOLDERS_TO_SEARCH):
        print('recursing in ' + f + ' directory: ')
        for x in dirwalk(fullpath, searchproject, replaceproject, searchman, replaceman, oldroot, newroot):
          yield x

    if os.path.isfile(fullpath):
      filename = os.path.basename(fullpath)
      newfilename = filename.replace(searchproject, replaceproject)
      base, extension = os.path.splitext(filename)

      if (not(extension in FILTERED_FILE_EXTENSIONS)):

        print("Replacing project name strings in file " + filename)
        replacestrs(fullpath, searchproject, replaceproject)

        print("Replacing captitalized project name strings in file " + filename)
        replacestrs(fullpath, searchproject.upper(), replaceproject.upper())

        print("Replacing manufacturer name strings in file " + filename)
        replacestrs(fullpath, searchman, replaceman)

        if (oldroot and newroot):
          print ("Replacing iPlug2 root folder in file  " + filename)
          replacestrs(fullpath, oldroot, newroot)
          replacestrs(fullpath, oldroot.replace('/', '\\'), newroot.replace('/', '\\'))

      else:
        print("NOT replacing name strings in file " + filename)

      if filename != newfilename:
        print("Renaming file " + filename + " to " + newfilename)
        os.rename(fullpath, os.path.join(dir, newfilename))

      yield f, fullpath
    else:
      yield f, fullpath

def update_uniqueid_in_resources(projectpath, oldid, newid, mfrid=""):
  """PLUG_UNIQUE_ID also appears in the plists (e.g. the AU subtype and CFBundleSignature), the AAX page table and the
  auval arguments of the Xcode schemes. The Xcode build regenerates the plists from config.h, but other builds
  (e.g. CMake) use them as they are, so a duplicate would share the original's IDs. Returns the updated files"""
  # The manufacturer ID is also a <string> in the plists, so if it is the same as the unique ID it can't be told apart
  if oldid == mfrid:
    print("warning: PLUG_UNIQUE_ID is the same as PLUG_MFR_ID, so it was not updated in the plists")
    return []

  patterns = [
    (re.compile(r"<string>" + re.escape(oldid) + r"</string>"), "<string>" + newid + "</string>"),
    (re.compile(r"prodID='" + re.escape(oldid) + r"'"), "prodID='" + newid + "'"),
    (re.compile(r"\b(au[a-z]{2}) " + re.escape(oldid) + r"\b"), r"\1 " + newid),
  ]

  updated = []

  for root, dirs, files in os.walk(projectpath):
    for f in files:
      if not (f.endswith(".plist") or f.endswith("-Pages.xml") or f.endswith(".xcscheme")):
        continue

      path = os.path.join(root, f)

      # newline="" keeps each file's line endings
      try:
        with open(path, encoding="utf-8", newline="") as fh:
          text = fh.read()
      except UnicodeDecodeError:
        print("warning: skipping " + path + ", it is not UTF-8 text")
        continue

      newtext = text
      for pattern, replacement in patterns:
        newtext = pattern.sub(replacement, newtext)

      if newtext != text:
        with open(path, "w", encoding="utf-8", newline="") as fh:
          fh.write(newtext)
        updated.append(os.path.relpath(path, projectpath))

  return updated

def main():
  global VERSION
  print("\nIPlug Project Duplicator v" + VERSION + " by Oli Larkin ------------------------------\n")

  numargs = len(sys.argv) - 1

  if not (numargs == 3 or numargs == 4):
    print("Usage: duplicate.py inputprojectname outputprojectname manufacturername (outputprojectpath)")
    sys.exit(1)
  else:
    inputprojectname=sys.argv[1]
    outputprojectname=sys.argv[2]
    manufacturer=sys.argv[3]

  if numargs == 4:
    outputbasepath=os.path.abspath(sys.argv[4])
  else:
    outputbasepath=os.getcwd()

  if not (os.path.isdir(outputbasepath)):
    print("error: Output path does not exist")
    sys.exit(1)

  outputpath = os.path.join(outputbasepath, outputprojectname)

  if ' ' in inputprojectname:
    print("error: input project name has spaces")
    sys.exit(1)

  if inputprojectname not in os.listdir(os.curdir):
    print("error: input project " +  inputprojectname + " doesn't exist, check spelling/case?")
    sys.exit(1)

  if ' ' in outputprojectname:
    print("error: output project name has spaces")
    sys.exit(1)

  if ' ' in manufacturer:
    print("error: manufacturer name has spaces")
    sys.exit(1)

  # remove a trailing slash if it exists
  if inputprojectname[-1:] == "/":
    inputprojectname = inputprojectname[0:-1]

  if outputprojectname[-1:] == "/":
    outputprojectname = outputprojectname[0:-1]

  #check that the folders are OK
  if os.path.isdir(inputprojectname) == False:
    print("error: input project not found")
    sys.exit(1)

  if os.path.isdir(outputpath):
    print("error: output project allready exists")
    sys.exit(1)
  # rmtree(output)

  print("copying " + inputprojectname + " folder to " + outputpath)
  copytree(inputprojectname, outputpath, ignore=ignore_patterns(*DONT_COPY))

  oldroot = ""
  newroot = ""
  
  if numargs == 4:
    configpath = os.path.join(inputprojectname, "config")
    xcconfig = parse_xcconfig(configpath + "/" + inputprojectname + "-mac.xcconfig")
    oldroot = xcconfig["IPLUG2_ROOT"]
    iplug2folder = os.path.abspath(os.path.join(configpath, oldroot))
    newroot = os.path.relpath(iplug2folder, os.path.join(outputpath, "config"))
  else:
    newroot = ""

  #replace manufacturer name strings
  for dir in dirwalk(outputpath, inputprojectname, outputprojectname, "AcmeInc", manufacturer, oldroot, newroot):
    pass

  print("\ncopying gitignore template into project folder\n")

  copy('gitignore_template', outputpath + "/.gitignore")

  try:
    config = parse_config(outputpath)

    olduniqueid = config["PLUG_UNIQUE_ID"]
    uniqueid = randomFourChar()

    # set_uniqueid() re-parses config.h into the same dict, leaving the old ID in it
    set_uniqueid(outputpath, uniqueid)
    config["PLUG_UNIQUE_ID"] = uniqueid

    for f in update_uniqueid_in_resources(outputpath, olduniqueid, uniqueid, config.get("PLUG_MFR_ID", "")):
      print("Updated PLUG_UNIQUE_ID in " + f)

    # the VST3 class ID is derived from PLUG_MFR_ID and PLUG_UNIQUE_ID, so the REAPER project must be updated to match
    if len(config["PLUG_MFR_ID"]) == 4:
      guid = vst3ProcessorGUID(config["PLUG_MFR_ID"], config["PLUG_UNIQUE_ID"])
      for f in os.listdir(outputpath):
        if f.lower().endswith(".rpp") and fixreaperproject(os.path.join(outputpath, f), outputprojectname, guid):
          print("Updated VST3 class ID in " + f + " to {" + guid + "}")

    pp = pprint.PrettyPrinter(indent=4)
    pp.pprint(config)

    print("\ndone - don't forget to change PLUG_UNIQUE_ID and PLUG_MFR_ID in config.h")
  except (ValueError, KeyError) as e:
    print("\nnote: could not parse full plugin config (this is expected for REAPER extensions)")
    print("done")

if __name__ == '__main__':
  main()
