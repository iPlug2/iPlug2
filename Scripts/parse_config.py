import fileinput, sys

config = {}

StringElements = {
"PLUG_VERSION_HEX",
"PLUG_NAME",
"PLUG_MFR",
"BUNDLE_MFR",
"BUNDLE_NAME",
"BUNDLE_DOMAIN",
"PLUG_CHANNEL_IO",
"PLUG_COPYRIGHT_STR",
"PLUG_UNIQUE_ID",
"PLUG_MFR_ID",
"AUV2_ENTRY",
"AUV2_ENTRY_STR",
"AUV2_FACTORY",
"AUV2_VIEW_CLASS",
"AUV2_VIEW_CLASS_STR",
"AAX_TYPE_IDS",
"AAX_TYPE_IDS_AUDIOSUITE",
"AAX_PLUG_MFR_STR",
"AAX_PLUG_NAME_STR",
"AAX_PLUG_CATEGORY_STR",
"VST3_SUBCATEGORY",
"SHARED_RESOURCES_SUBPATH"
}

IntElements = {
"PLUG_TYPE",
"PLUG_DOES_MIDI_IN",
"PLUG_DOES_MIDI_OUT",
"PLUG_HAS_UI",
"PLUG_SHARED_RESOURCES",
"PLUG_WIDTH",
"PLUG_HEIGHT",
"PLUG_FPS",
"APP_COPY_AUV3",
"AAX_DOES_AUDIOSUITE",
}

for stringElement in StringElements:
  config[stringElement] = ""

for intElement in IntElements:
  config[intElement] = 0

def extractInt(line, macro):
  lineText = "#define " + macro + " "
  if lineText in line:
    config[macro] = int(line[len(lineText):])

def extractStringElement(line, macro):
  lineText = "#define " + macro + " "
  if lineText in line:
    if '\"' in line:
      config[macro] = line[len(lineText):-1].strip('\"')
    elif "\'" in line:
      config[macro] = line[len(lineText):-1].strip('\'')
    else:
      config[macro] = line[len(lineText):-1]
    return True
  else:
    return False

def set_uniqueid(projectpath, id):
  for line in fileinput.input(projectpath + "/config.h", inplace=1):
    found = extractStringElement(line, "PLUG_UNIQUE_ID")
    if(found):
      sys.stdout.write(line.replace(config["PLUG_UNIQUE_ID"], id))
    else:
      sys.stdout.write(line)

  fileinput.close()

def parse_config(projectpath):
  # extract values from config.h
  for line in fileinput.input(projectpath + "/config.h", inplace=0):
    found = False
    while found == False:
      for stringElement in StringElements:
        found = extractStringElement(line, stringElement)
      for intElement in IntElements:
        found = extractInt(line, intElement)

  # add some derived vals
  config["PLUG_VERSION_INT"] = int(config["PLUG_VERSION_HEX"], 16)
  MAJOR_INT = config["PLUG_VERSION_INT"] & 0xFFFF0000
  config["MAJOR_STR"] = str(MAJOR_INT >> 16)
  MINOR_INT = config["PLUG_VERSION_INT"] & 0x0000FF00
  config["MINOR_STR"] = str(MINOR_INT >> 8)
  config["BUGFIX_STR"] = str(config["PLUG_VERSION_INT"] & 0x000000FF)
  config["FULL_VER_STR"] = config["MAJOR_STR"] + "." + config["MINOR_STR"] + "." + config["BUGFIX_STR"]

  fileinput.close()

  return config

def parse_xcconfig(configFile):

  def extractXCInt(line, setting):
    lineText = setting + " = "
    if lineText in line:
      xcconfig[setting] = int(line[len(lineText):], 16)

  def extractXCStringElement(line, setting):
    lineText = setting + " = "
    if lineText in line:
      xcconfig[setting] = line[len(lineText):-1].strip('\"')

  xcconfig = {}

  xcconfig['BASE_SDK_MAC'] = "macosx"
  xcconfig['MACOSX_DEPLOYMENT_TARGET'] = "10.9"

  for line in fileinput.input(configFile, inplace=0):
    if not "//" in line:
      extractXCStringElement(line, 'BASE_SDK_MAC')
      extractXCStringElement(line, 'IPLUG2_ROOT')

      if "MACOSX_DEPLOYMENT_TARGET = " in line:
        xcconfig['DEPLOYMENT_TARGET'] = line[len("MACOSX_DEPLOYMENT_TARGET = "):-1].strip('\"') + ".0"

      if "IPHONEOS_DEPLOYMENT_TARGET = " in line:
        xcconfig['DEPLOYMENT_TARGET'] = line[len("IPHONEOS_DEPLOYMENT_TARGET = "):-1].strip('\"') + ".0"

  fileinput.close()

  return xcconfig

if __name__ == '__main__':
  import sys
  parse_config(sys.argv[1])
