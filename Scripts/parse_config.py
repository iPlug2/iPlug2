import fileinput

config = {}

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

def write_config(projectpath, incomingConfig):
  file = open(projectpath + "/config.h", "w")
    # file.write("#define PLUG_NAME " + "IPlugEffect")
    # file.write("#define PLUG_MFR " + "AcmeInc")
    # file.write("#define PLUG_VERSION_HEX " + "0x00010000)
    # file.write("#define PLUG_VERSION_STR " + "1.0.0")
    # file.write("#define PLUG_UNIQUE_ID " + "Ipef')
    # file.write("#define PLUG_MFR_ID " + "Acme')
    # file.write("#define PLUG_URL_STR " + "www.olilarkin.co.uk")
    # file.write("#define PLUG_EMAIL_STR " + "spam@me.com")
    # file.write("#define PLUG_COPYRIGHT_STR " + "Copyright 2017 Acme Inc")
    # file.write("#define PLUG_CLASS_NAME " + "IPlugEffect)
    # file.write("#define BUNDLE_NAME " + "IPlugEffect")
    # file.write("#define BUNDLE_MFR " + "AcmeInc")
    # file.write("#define BUNDLE_DOMAIN " + "com")
    # file.write("#define PLUG_CHANNEL_IO " + "1-1 2-2")
    # file.write("#define PLUG_LATENCY " + "0)
    # file.write("#define PLUG_IS_INSTRUMENT " + "0)
    # file.write("#define PLUG_DOES_MIDI " + "0)
    # file.write("#define PLUG_DOES_STATE_CHUNKS " + "0)
    # file.write("#define PLUG_HAS_UI " + "1)
    # file.write("#define PLUG_WIDTH " + "100)
    # file.write("#define PLUG_HEIGHT " + "100)
    # file.write("#define PLUG_FPS " + "60)
    # file.write("#define PLUG_SHARED_RESOURCES " + "1)
    # file.write("#define AUV2_ENTRY " + "IPlugEffect_Entry)
    # file.write("#define AUV2_ENTRY_STR " + " "IPlugEffect_Entry")
    # file.write("#define AUV2_FACTORY " + "IPlugEffect_Factory)
    # file.write("#define AUV2_VIEW_CLASS " + " IPlugEffect_View)
    # file.write("#define AUV2_VIEW_CLASS_STR " + ""IPlugEffect_View")
    # file.write("#define AAX_TYPE_IDS " + "'EFN1', 'EFN2')
    # file.write("#define AAX_TYPE_IDS_AUDIOSUITE '" + "EFA1', 'EFA2')
    # file.write("#define AAX_PLUG_MFR_STR " + "\"AcmeInc\\nAcmeInc\\nAcme")
    # file.write("#define AAX_PLUG_NAME_STR \"IPlugEffect\\nIPEF\"")
    # file.write("#define AAX_PLUG_CATEGORY_STR Effect")
    # file.write("#define AAX_DOES_AUDIOSUITE 1)
    # file.write("#define VST3_SUBCATEGORY Fx")
    # file.write("#define APP_ENABLE_SYSEX 0")
    # file.write("#define APP_ENABLE_MIDICLOCK 0")
    # file.write("#define APP_ENABLE_ACTIVE_SENSING 1")
    # file.write("#define APP_NUM_CHANNELS 2)
    # file.write("#define APP_N_VECTOR_WAIT 50)
    # file.write("#define APP_MULT 0.25")
  file.close()


def parse_config(projectpath):
  # extract values from config.h
  for line in fileinput.input(projectpath + "/config.h", inplace=0):
    if "#define PLUG_VERSION_HEX " in line:
      config["PLUG_VER_HEX"] = line[len("#define PLUG_VERSION_HEX "):-1]
      config["PLUG_VER_INT"] = int(config["PLUG_VER_HEX"], 16)
      MAJOR_INT = config["PLUG_VER_INT"] & 0xFFFF0000
      config["MAJOR_STR"] = str(MAJOR_INT >> 16)
      MINOR_INT = config["PLUG_VER_INT"] & 0x0000FF00
      config["MINOR_STR"] = str(MINOR_INT >> 8)
      config["BUGFIX_STR"] = str(config["PLUG_VER_INT"] & 0x000000FF)

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

  config["FULL_VER_STR"] = config["MAJOR_STR"] + "." + config["MINOR_STR"] + "." + config["BUGFIX_STR"]

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
