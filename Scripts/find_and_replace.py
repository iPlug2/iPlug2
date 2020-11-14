#!/usr/bin/env python3

from __future__ import generators

import fileinput, glob, string, sys, os, re
from shutil import copytree, ignore_patterns, rmtree
from os.path import join

def replacestrs(filename, s, r):
  files = glob.glob(filename)
  print("replacing " + s + " with " + r + " in " + filename)

  for line in fileinput.input(files,inplace=1):
    string.find(line, s)
    line = line.replace(s, r)
    sys.stdout.write(line)

def dirwalk(dir, sbase, sext, s, r):
  for f in os.listdir(dir):
    fullpath = os.path.join(dir, f)
    
    if not os.path.islink(fullpath):
      if os.path.isdir(fullpath):
          for file in dirwalk(fullpath, sbase, sext, s, r):
            pass
        
      if os.path.isfile(fullpath):
        base, extension = os.path.splitext(f)
        
        if sbase == "any" or base == sbase:				
          if extension == "." + sext:
            replacestrs(fullpath, s, r)
            yield fullpath


def main():
  if len(sys.argv) != 5:
    print("Usage: find_and_replace.py search_base search_ext search_string replace_string")
    print("search base can be 'any'")
    sys.exit(1)
  else:
    sbase=sys.argv[1]
    sext=sys.argv[2]
    search_string=sys.argv[3]
    replace_string=sys.argv[4]
        
    for file in dirwalk(os.getcwd(), sbase, sext, search_string, replace_string):
      pass
    
if __name__ == '__main__':
  main()