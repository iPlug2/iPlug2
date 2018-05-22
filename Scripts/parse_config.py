import fileinput

def parse_config(projectpath):
  config = {}

  MAJOR_STR = ""
  MINOR_STR = ""
  BUGFIX_STR = ""
  PLUG_VER_STR = ""

  config["BUNDLE_MFR"] = ""
  config["BUNDLE_NAME"] = ""
  config["BUNDLE_DOMAIN"] = ""
  config["PLUG_NAME"] = ""
  config["PLUG_MFR"] = ""
  config["PLUG_CHANNEL_IO"] = ""
  config["PLUG_COPYRIGHT"] = ""
  config["PLUG_UID"] = ""
  config["PLUG_MFR_UID"] = ""
  config["AUV2_FACTORY"] = ""
  config["AUV2_ENTRY"] = ""
  config["PLUG_IS_INSTRUMENT"] = 0
  config["PLUG_DOES_MIDI"] = 0
  config["PLUG_HAS_UI"] = 0
  config["PLUG_SHARED_RESOURCES"] = 0
  config["PLUG_VER_INT"] = 0
  config["PLUG_VER_HEX"] = 0
  config["MAJOR_STR"] = ""
  config["MINOR_STR"] = ""
  config["BUGFIX_STR"] = ""

  # extract values from config.h
  for line in fileinput.input(projectpath + "/config.h", inplace=0):
    if "#define PLUG_VERSION_HEX " in line:
      config["PLUG_VER_HEX"] = line[len("#define PLUG_VERSION_HEX "):-1]
      config["PLUG_VER_INT"] = int(config["PLUG_VER_HEX"], 16)
      MAJOR_INT = config["PLUG_VER_INT"] & 0xFFFF0000
      MAJOR_STR = str(MAJOR_INT >> 16)
      MINOR_INT = config["PLUG_VER_INT"] & 0x0000FF00
      MINOR_STR = str(MINOR_INT >> 8)
      BUGFIX_STR = str(config["PLUG_VER_INT"] & 0x000000FF)

    if "#define PLUG_NAME " in line:
      config["PLUG_NAME"] = line[len("#define PLUG_NAME "):-1].strip('\"')

    if "#define PLUG_MFR " in line:
      config["PLUG_MFR"] = line[len("#define PLUG_MFR "):-1].strip('\"')

    if "#define BUNDLE_MFR " in line:
      config["BUNDLE_MFR"] = line[len("#define BUNDLE_MFR "):-1].strip('\"')

    if "#define BUNDLE_NAME " in line:
      config["BUNDLE_NAME"] = line[len("#define BUNDLE_NAME "):-1].strip('\"')

    if "#define BUNDLE_DOMAIN " in line:
      config["BUNDLE_DOMAIN"] = line[len("#define BUNDLE_DOMAIN "):-1].strip('\"')

    if "#define PLUG_CHANNEL_IO " in line:
      config["PLUG_CHANNEL_IO"] = line[len("#define PLUG_CHANNEL_IO "):-1].strip('\"')

    if "#define PLUG_COPYRIGHT_STR " in line:
      config["PLUG_COPYRIGHT"] = line[len("#define PLUG_COPYRIGHT_STR "):-1].strip('\"')

    if "#define PLUG_UNIQUE_ID " in line:
      config["PLUG_UID"] = line[len("#define PLUG_UNIQUE_ID "):-1].strip('\'')

    if "#define PLUG_MFR_ID " in line:
      config["PLUG_MFR_UID"] = line[len("#define PLUG_MFR_ID "):-1].strip('\'')

    if "#define AUV2_ENTRY " in line:
      config["AUV2_ENTRY"] = line[len("#define AUV2_ENTRY "):-1]

    if "#define AUV2_FACTORY " in line:
      config["AUV2_FACTORY"] = line[len("#define AUV2_FACTORY "):-1]

    if "#define PLUG_IS_INSTRUMENT " in line:
      config["PLUG_IS_INSTRUMENT"] = int(line[len("#define PLUG_IS_INSTRUMENT "):], 16)

    if "#define PLUG_DOES_MIDI " in line:
      config["PLUG_DOES_MIDI"] = int(line[len("#define PLUG_DOES_MIDI "):], 16)

    if "#define PLUG_HAS_UI " in line:
      config["PLUG_HAS_UI"] = int(line[len("#define PLUG_HAS_UI "):], 16)

    if "#define PLUG_SHARED_RESOURCES " in line:
      config["PLUG_SHARED_RESOURCES"] = int(line[len("#define PLUG_SHARED_RESOURCES "):], 16)

  config["FULL_VER_STR"] = MAJOR_STR + "." + MINOR_STR + "." + BUGFIX_STR

  fileinput.close()

  return config

def parse_xcconfig(configFile):
  xcconfig = {}

  xcconfig['BASE_SDK'] = "macosx10.13"
  xcconfig['DEPLOYMENT_TARGET'] = "10.7"

  for line in fileinput.input(configFile, inplace=0):
    if not "//" in line:
      if "BASE_SDK_MAC = " in line:
        xcconfig['BASE_SDK_MAC'] = line[len("BASE_SDK_MAC = "):-1].strip('\"')
      if "BASE_SDK_IOS = " in line:
        xcconfig['BASE_SDK_IOS'] = line[len("BASE_SDK_IOS = "):-1].strip('\"')
      if "DEPLOYMENT_TARGET = " in line:
        xcconfig['DEPLOYMENT_TARGET'] = line[len("DEPLOYMENT_TARGET = "):-1].strip('\"') + ".0"

  fileinput.close()

  return xcconfig

if __name__ == '__main__':
  import sys
  parse_config(sys.argv[1])
