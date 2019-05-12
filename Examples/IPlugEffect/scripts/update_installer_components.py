#!/usr/bin/python

# this script will update the components in installer files depending on user input

import plistlib, os, datetime, fileinput, glob, sys, string

scriptpath = os.path.dirname(os.path.realpath(__file__))
projectpath = os.path.abspath(os.path.join(scriptpath, os.pardir))

sys.path.insert(0, projectpath + '/../../scripts/')

from parse_config import parse_config

def str2bool(v):
  return v.lower() in ("yes", "true", "t", "1")

def enableLine(textSnippet, line, enable):
  if textSnippet in line:
    if enable:
      line = line[1:] if line[0] == ';' else line
    else:
      line = line if line[0] == ';' else ';' + line
  return line

def enableStandalone(line, config, formatEnabled):
  line = enableLine("Name: \"app\";", line, formatEnabled)
  line = enableLine("Components:app", line, formatEnabled)
  line = enableLine(config['BUNDLE_NAME'] + ".exe", line, formatEnabled)
  return line

def enableAAX32(line, config, formatEnabled):
  line = enableLine("Name: \"aax_32\";", line, formatEnabled)
  line = enableLine("Components:aax_32", line, formatEnabled)
  return line

def enableAAX64(line, config, formatEnabled):
  line = enableLine("Name: \"aax_64\";", line, formatEnabled)
  line = enableLine("Components:aax_64", line, formatEnabled)
  return line

def enableVST2(line, config, formatEnabled):
  # VST2 32bit
  line = enableLine("Name: \"vst2_32\";", line, formatEnabled)
  line = enableLine("Components:vst2_32", line, formatEnabled)
  # VST2 64bit
  line = enableLine("Name: \"vst2_64\";", line, formatEnabled)
  line = enableLine("Components:vst2_64", line, formatEnabled)
  
  if "procedure InitializeWizard" in line:
    line = "procedure InitializeWizard;\n" if formatEnabled else "procedure InitializeWizardDisabled;\n"  

  return line

def enableVST3(line, config, formatEnabled):
  # VST3 32bit
  line = enableLine("Name: \"vst3_32\";", line, formatEnabled)
  line = enableLine("Components:vst3_32", line, formatEnabled)
  # VST3 64bit
  line = enableLine("Name: \"vst3_64\";", line, formatEnabled)
  line = enableLine("Components:vst3_64", line, formatEnabled)
  return line

def main():
  config = parse_config(projectpath)

  demo = False
  standalone = False
  aax32 = False
  aax64 = False
  vst2 = False
  vst3 = False

  if len(sys.argv) != 8:
    print("Usage: update_installer_components.py demo(0 or 1) standalone(0 or 1) aax32(0 or 1) aax64(0 or 1) vst2(0 or 1) vst3(0 or 1)")
    sys.exit(1)
  else:
    demo = str2bool(sys.argv[1])
    standalone = str2bool(sys.argv[2])
    aax32 = str2bool(sys.argv[3])
    aax64 = str2bool(sys.argv[4])
    vst2 = str2bool(sys.argv[5])
    vst3 = str2bool(sys.argv[6])

  # MAC INSTALLER

  #  print "Updating Mac Installer version info..."
  #
  #  plistpath = projectpath + "/installer/" + config['BUNDLE_NAME'] + ".pkgproj"
  #  installer = plistlib.readPlist(plistpath)
  #
  #  # range  = number of items in the installer (VST 2, VST 3, app, audiounit, aax)
  #  for x in range(0,5):
  #    installer['PACKAGES'][x]['PACKAGE_SETTINGS']['VERSION'] = config['FULL_VER_STR']
  #
  #  if demo:
  #    installer['PROJECT']['PROJECT_PRESENTATION']['TITLE']['LOCALIZATIONS'][0]['VALUE'] = config['BUNDLE_NAME'] + " Demo"
  #    installer['PROJECT']['PROJECT_PRESENTATION']['INTRODUCTION']['LOCALIZATIONS'][0]['VALUE']['PATH'] = "intro-demo.rtf"
  #  else:
  #    installer['PROJECT']['PROJECT_PRESENTATION']['TITLE']['LOCALIZATIONS'][0]['VALUE'] = config['BUNDLE_NAME']
  #    installer['PROJECT']['PROJECT_PRESENTATION']['INTRODUCTION']['LOCALIZATIONS'][0]['VALUE']['PATH'] = "intro.rtf"
  #
  #  plistlib.writePlist(installer, plistpath)
  #   replacestrs(plistpath, "//Apple//", "//Apple Computer//");

  # WIN INSTALLER
  print "Updating Windows Installer component list ..."
  print("demo=" + str(demo) + ", standalone=" + str(standalone) + ", aax32=" + str(aax32) + ", aax64=" + str(aax64) + ", vst2=" + str(vst2) + ", vst3=" + str(vst3) )

  for line in fileinput.input(projectpath + "/installer/" + config['BUNDLE_NAME'] + ".iss", inplace=1):
    line = enableStandalone(line, config, standalone)
    line = enableAAX32(line, config, aax32)
    line = enableAAX64(line, config, aax64)
    line = enableVST2(line, config, vst2)
    line = enableVST3(line, config, vst3)
    sys.stdout.write(line)


if __name__ == '__main__':
  main()
