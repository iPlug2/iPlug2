#!/usr/bin/python

# Python shell script for Duplicating WDL-OL IPlug Projects
# Oli Larkin 2012-2014 http://www.olilarkin.co.uk
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
# duplicate.py [inputprojectname] [outputprojectname] [manufacturername]

# TODO:
# - indentation of directory structure
# - variable manufacturer name


from __future__ import generators

import fileinput, glob, string, sys, os, re, uuid
from shutil import copy, copytree, ignore_patterns, rmtree
from os.path import join

VERSION = "0.9"

# binary files that we don't want to do find and replace inside
FILTERED_FILE_EXTENSIONS = [".ico",".icns", ".pdf", ".png", ".zip", ".exe", ".wav", ".aif"]
# files that we don't want to duplicate
DONT_COPY = ("*.exe", "*.dmg", "*.pkg", "*.mpkg", "*.svn", "*.ncb", "*.suo", "*sdf", "ipch", "build-*", "*.layout", "*.depend", ".DS_Store")

SUBFOLDERS_TO_SEARCH = [
"app_wrapper",
"resources",
"installer",
"scripts",
"manual",
"ios_wrapper",
"xcschemes",
"xcshareddata",
"xcuserdata",
"en-osx.lproj",
"project.xcworkspace"
]

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
    line.find(s)
    line = line.replace(s, r)
    sys.stdout.write(line)
    
def replacestrsChop(filename, s, r):
  files = glob.glob(filename)
  
  for line in fileinput.input(files,inplace=1):
    if(line.startswith(s)): 
      line = r + "\n"
    sys.stdout.write(line)

def dirwalk(dir, searchproject, replaceproject, searchman, replaceman):
  for f in os.listdir(dir):
    fullpath = os.path.join(dir, f)
    
    if os.path.isdir(fullpath) and not os.path.islink(fullpath):
      if checkdirname(f, searchproject + ".xcodeproj"):
        os.rename(fullpath, os.path.join(dir, replaceproject + ".xcodeproj"))
        fullpath = os.path.join(dir, replaceproject + ".xcodeproj")
        
        print("recursing in main xcode project directory: ")
        for x in dirwalk(fullpath, searchproject, replaceproject, searchman, replaceman):
          yield x
      elif checkdirname(f, searchproject + "-ios.xcodeproj"):
        os.rename(fullpath, os.path.join(dir, replaceproject + "-ios.xcodeproj"))
        fullpath = os.path.join(dir, replaceproject + "-ios.xcodeproj")
        
        print("recursing in ios xcode project directory: ")
        for x in dirwalk(fullpath, searchproject, replaceproject, searchman, replaceman):
          yield x
      elif (f in SUBFOLDERS_TO_SEARCH):
        print('recursing in ' + f + ' directory: ')
        for x in dirwalk(fullpath, searchproject, replaceproject, searchman, replaceman):
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
      else:
        print("NOT replacing name strings in file " + filename)
      
      if filename != newfilename:
        print("Renaming file " + filename + " to " + newfilename)
        os.rename(fullpath, os.path.join(dir, newfilename))
  
      yield f, fullpath 
    else:
      yield f, fullpath

def main():
  global VERSION
  print("\nIPlug Project Duplicator v" + VERSION + " by Oli Larkin ------------------------------\n")
  
  if len(sys.argv) != 4:
    print("Usage: duplicate.py inputprojectname outputprojectname [manufacturername]")
    sys.exit(1)
  else:
    input=sys.argv[1]
    output=sys.argv[2]
    manufacturer=sys.argv[3]

    if ' ' in input:
      print("error: input project name has spaces")
      sys.exit(1)
      
    if ' ' in output:
      print("error: output project name has spaces")
      sys.exit(1)
    
    if ' ' in manufacturer:
      print("error: manufacturer name has spaces")
      sys.exit(1)
    
    # remove a trailing slash if it exists
    if input[-1:] == "/":
      input = input[0:-1]
    
    if output[-1:] == "/":
      output = output[0:-1]
      
    #check that the folders are OK
    if os.path.isdir(input) == False:
      print("error: input project not found")
      sys.exit(1)
        
    if os.path.isdir(output):
      print("error: output folder allready exists")
      sys.exit(1)
    # rmtree(output)
        
    print("copying " + input + " folder to " + output)
    copytree(input, output, ignore=ignore_patterns(*DONT_COPY))
    cpath = os.path.join(os.getcwd(), output)

    #replace manufacturer name strings
    for dir in dirwalk(cpath, input, output, "AcmeInc", manufacturer):
      pass
    
    print("\ncopying gitignore template into project folder")

    copy('gitignore_template', output + "/.gitignore")

    print("\ndone - don't forget to change PLUG_UID and MFR_UID in config.h")
    
if __name__ == '__main__':
  main()
