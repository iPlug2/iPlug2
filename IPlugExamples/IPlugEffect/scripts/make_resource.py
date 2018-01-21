#!/usr/bin/python

# populate config.h with tags for resource filenames and ids based on images in ../resources/img

import os, sys, fileinput, string
scriptpath = os.path.dirname(os.path.realpath(__file__))
projectpath = os.path.abspath(os.path.join(scriptpath, os.pardir))

def main():

  print "Processing Windows resource file..."

  MajorStr = ""
  MinorStr = "" 
  BugfixStr = ""
  
  BUNDLE_MFR = ""
  BUNDLE_NAME = ""
  PLUG_NAME_STR = ""
  PLUG_MFR_NAME_STR = ""
  PLUG_CHANNEL_IO = ""
  PLUG_COPYRIGHT = ""
  PLUG_TRADEMARKS = ""
  PLUG_UID = ""
  PLUG_MFR_UID = ""
  PLUG_TYPE = 0
  
  # extract values from config.h
  for line in fileinput.input(projectpath + "\config.h",inplace=0):
    if "#define PLUG_VERSION_HEX " in line:
      PLUG_VERSION_STR = string.lstrip(line, "#define PLUG_VERSION_HEX ")
      PLUG_VER = int(PLUG_VERSION_STR, 16)
      MAJOR = PLUG_VER & 0xFFFF0000
      MAJORSTR = str(MAJOR >> 16)
      MINOR = PLUG_VER & 0x0000FF00
      MINORSTR = str(MINOR >> 8)
      BUGFIXSTR = str(PLUG_VER & 0x000000FF)
      
    if "#define BUNDLE_MFR " in line:
      BUNDLE_MFR = string.lstrip(line, "#define BUNDLE_MFR ")
      
    if "#define BUNDLE_NAME " in line:
      BUNDLE_NAME = string.lstrip(line, "#define BUNDLE_NAME ")
            
    if "#define PLUG_NAME_STR " in line:
      PLUG_NAME_STR = string.lstrip(line, "#define PLUG_NAME_STR ")
      
    if "#define PLUG_MFR_NAME_STR " in line:
      PLUG_MFR_NAME_STR = string.lstrip(line, "#define PLUG_MFR_NAME_STR ")
       
    if "#define PLUG_CHANNEL_IO " in line:
      PLUG_CHANNEL_IO = string.lstrip(line, "#define PLUG_CHANNEL_IO ")
      
    if "#define PLUG_COPYRIGHT " in line:
      PLUG_COPYRIGHT = string.lstrip(line, "#define PLUG_COPYRIGHT ")

    if "#define PLUG_TRADEMARKS " in line:
      PLUG_TRADEMARKS = string.lstrip(line, "#define PLUG_TRADEMARKS ")
      
    if "#define PLUG_UID " in line:
      PLUG_UID = string.lstrip(line, "#define PLUG_UID ")
      
    if "#define PLUG_MFR_UID " in line:
      PLUG_MFR_UID = string.lstrip(line, "#define PLUG_MFR_UID ")
      
    if "#define PLUG_TYPE " in line:
      PLUG_TYPE = int(string.lstrip(line, "#define PLUG_TYPE "), 16)
  
  FULLVERSIONSTR = MAJORSTR + "." + MINORSTR + "." + BUGFIXSTR
  
  #strip quotes and newlines
  PLUG_VER_STR = PLUG_VER_STR[0:-1]
  BUNDLE_MFR = BUNDLE_MFR[1:-2]
  BUNDLE_NAME = BUNDLE_NAME[1:-2]
  PLUG_NAME_STR = PLUG_NAME_STR[1:-2]
  PLUG_MFR_NAME_STR = PLUG_MFR_NAME_STR[1:-2]
  PLUG_CHANNEL_IO = PLUG_CHANNEL_IO[1:-2]
  PLUG_COPYRIGHT = PLUG_COPYRIGHT[1:-2]
  PLUG_TRADEMARKS = PLUG_TRADEMARKS[1:-2]
  PLUG_MFR_UID = PLUG_MFR_UID[1:-2]
  PLUG_UID = PLUG_UID[1:-2]
  
  rc = open(projectpath + "/" + BUNDLE_NAME + ".rc", "w")
  
  rc.write("\n")
  rc.write("/////////////////////////////////////////////////////////////////////////////\n")
  rc.write("// Version\n")
  rc.write("/////////////////////////////////////////////////////////////////////////////\n")
  rc.write("VS_VERSION_INFO VERSIONINFO\n")
  rc.write("FILEVERSION " + MAJORSTR + "," + MINORSTR + "," + BUGFIXSTR + ",0\n")
  rc.write("PRODUCTVERSION " + MAJORSTR + "," + MINORSTR + "," + BUGFIXSTR + ",0\n")
  rc.write(" FILEFLAGSMASK 0x3fL\n")
  rc.write("#ifdef _DEBUG\n")
  rc.write(" FILEFLAGS 0x1L\n")
  rc.write("#else\n")
  rc.write(" FILEFLAGS 0x0L\n")
  rc.write("#endif\n")
  rc.write(" FILEOS 0x40004L\n")
  rc.write(" FILETYPE 0x1L\n")
  rc.write(" FILESUBTYPE 0x0L\n")
  rc.write("BEGIN\n")
  rc.write('    BLOCK "StringFileInfo"\n')
  rc.write("    BEGIN\n")
  rc.write('        BLOCK "040004e4"\n')
  rc.write("        BEGIN\n")
  rc.write('            VALUE "FileVersion", "' + FULLVERSIONSTR + '"\0\n')
  rc.write('            VALUE "ProductVersion", "' + FULLVERSIONSTR + '"0\n')
  rc.write("#ifdef VST2_API\n")
  rc.write('            VALUE "OriginalFilename", "' + BUNDLE_NAME + '.dll"\0\n')
  rc.write("#elif defined VST3_API\n")
  rc.write('            VALUE "OriginalFilename", "' + BUNDLE_NAME + '.vst3"\0\n')
  rc.write("#elif defined AAX_API\n")
  rc.write('            VALUE "OriginalFilename", "' + BUNDLE_NAME + '.aaxplugin"\0\n')
  rc.write("#elif defined SA_API\n")
  rc.write('            VALUE "OriginalFilename", "' + BUNDLE_NAME + '.exe"\0\n')
  rc.write("#endif\n")  
  rc.write('            VALUE "FileDescription", "' + PLUG_NAME_STR + '"\0\n')
  rc.write('            VALUE "InternalName", "' + PLUG_NAME_STR + '"\0\n')
  rc.write('            VALUE "ProductName", "' + PLUG_NAME_STR + '"\0\n')
  rc.write('            VALUE "CompanyName", "' + PLUG_MFR_NAME_STR + '"\0\n')
  rc.write('            VALUE "LegalCopyright", "' + PLUG_COPYRIGHT + '"\0\n')
  rc.write('            VALUE "LegalTrademarks", "' + PLUG_TRADEMARKS + '"\0\n')
  rc.write("        END\n")
  rc.write("    END\n")
  rc.write('    BLOCK "VarFileInfo"\n')
  rc.write("    BEGIN\n")
  rc.write('        VALUE "Translation", 0x400, 1252\n')
  rc.write("    END\n")
  rc.write("END\n")
  rc.write("\n")

if __name__ == '__main__':
  main()