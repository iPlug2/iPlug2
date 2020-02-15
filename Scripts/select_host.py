#!/usr/bin/python

#python shell script to modify common-win.props to select a host for debugging VST plug-ins

SAVIHOST_PATH = "$(ProgramFiles)\\vsthost\savihost.exe"
LIVE_PATH = "$(ProgramData)\Ableton\Live 10 Suite\Program\Ableton Live 10 Suite.exe"
FL_PATH = "$(ProgramFiles)\Image-Line\FL Studio 20\FL.exe"
CUBASE_PATH = "$(ProgramFiles)\Steinberg\Cubase 10.5\Cubase10.5.exe"
S1_PATH = "$(ProgramFiles)\PreSonus\Studio One 4\Studio One.exe"
REAPER_PATH = "$(ProgramFiles)\REAPER\\reaper.exe"
SONAR_PATH = "$(ProgramFiles)\Cakewalk\SONAR X3 Producer\SONARPDR.exe"
VST3TESTHOST_PATH = "$(ProgramFiles)\Steinberg\VST3PluginTestHost\VST3PluginTestHost.exe"

SAVIHOST_X64_PATH = "$(ProgramW6432)\\vsthost\savihost.exe"
LIVE_X64_PATH = "$(ProgramData)\Ableton\Live 10 Suite\Program\Ableton Live 10 Suite.exe"
FL_X64_PATH = "$(ProgramFiles)\Image-Line\FL Studio 20\FL64.exe"
CUBASE_X64_PATH = "$(ProgramW6432)\Steinberg\Cubase 10.5\Cubase10.5.exe"
S1_X64_PATH = "$(ProgramW6432)\PreSonus\Studio One 4\Studio One.exe"
REAPER_X64_PATH = "$(ProgramW6432)\REAPER (x64)\\reaper.exe"
SONAR_X64_PATH = "$(ProgramW6432)\Cakewalk\SONAR X3 Producer\SONARPDR.exe"
VST3TESTHOST_X64_PATH = "$(ProgramW6432)\Steinberg\VST3PluginTestHost\VST3PluginTestHost.exe"

SAVIHOST_ARGS = "$(TargetPath) /noload /nosave /noexc /noft"
REAPER_ARGS = "$(SolutionDir)$(SolutionName).RPP"

PATHS = [SAVIHOST_PATH, LIVE_PATH,  FL_PATH,  CUBASE_PATH,  S1_PATH,  REAPER_PATH,  SONAR_PATH, VST3TESTHOST_PATH]
PATHS_X64 = [SAVIHOST_X64_PATH, LIVE_X64_PATH,  FL_X64_PATH,  CUBASE_X64_PATH,  S1_X64_PATH,  REAPER_X64_PATH,  SONAR_X64_PATH, VST3TESTHOST_X64_PATH]
ARGS = [SAVIHOST_ARGS, "", "", "", "", REAPER_ARGS, "", ""]

from xml.dom import minidom as md
doc  = md.parse('..\common-win.props')

print("HOST options:")
print(" 1 - Savihost")
print(" 2 - Live")
print(" 3 - FLStudio")
print(" 4 - Cubase")
print(" 5 - StudioOne")
print(" 6 - Reaper")
print(" 7 - Sonar")
print(" 8 - VST3 Test Host")

# vst2/32bit
print("choose a host to use for 32bit VST2 debugging...")
choice = int(raw_input("Choice>>"))

elem = doc.getElementsByTagName('VST2_32_HOST_PATH')[0]
for child in elem.childNodes:
	elem.removeChild(child)
if PATHS[choice-1]:
	text = doc.createTextNode(PATHS[choice-1])
	elem.appendChild(text)
	
elem = doc.getElementsByTagName('VST2_32_COMMAND_ARGS')[0]
for child in elem.childNodes:
	elem.removeChild(child)
if ARGS[choice-1]:
	text = doc.createTextNode(ARGS[choice-1])
	elem.appendChild(text)	

# vst2/64bit
print("choose a host to use for 64bit VST2 debugging...")
choice = int(raw_input("Choice>>"))

elem = doc.getElementsByTagName('VST2_64_HOST_PATH')[0]
for child in elem.childNodes:
	elem.removeChild(child)
if PATHS_X64[choice-1]:
	text = doc.createTextNode(PATHS_X64[choice-1])
	elem.appendChild(text)
	
elem = doc.getElementsByTagName('VST2_64_COMMAND_ARGS')[0]
for child in elem.childNodes:
	elem.removeChild(child)
if ARGS[choice-1]:
	text = doc.createTextNode(ARGS[choice-1])
	elem.appendChild(text)	

# vst3/32bit
print("choose a host to use for 32bit VST3 debugging...")
choice = int(raw_input("Choice>>"))

elem = doc.getElementsByTagName('VST3_32_HOST_PATH')[0]
for child in elem.childNodes:
	elem.removeChild(child)
if PATHS[choice-1]:
	text = doc.createTextNode(PATHS[choice-1])
	elem.appendChild(text)
	
elem = doc.getElementsByTagName('VST3_32_COMMAND_ARGS')[0]
for child in elem.childNodes:
	elem.removeChild(child)
if ARGS[choice-1]:
	text = doc.createTextNode(ARGS[choice-1])
	elem.appendChild(text)

# vst3/64bit
print("choose a host to use for 64bit VST3 debugging...")
choice = int(raw_input("Choice>>"))

elem = doc.getElementsByTagName('VST3_64_HOST_PATH')[0]
for child in elem.childNodes:
	elem.removeChild(child)
if PATHS_X64[choice-1]:
	text = doc.createTextNode(PATHS_X64[choice-1])
	elem.appendChild(text)
	
elem = doc.getElementsByTagName('VST3_64_COMMAND_ARGS')[0]
for child in elem.childNodes:
	elem.removeChild(child)
if ARGS[choice-1]:
	text = doc.createTextNode(ARGS[choice-1])
	elem.appendChild(text)

#elem = doc.getElementsByTagName('COPY_VST2')
#elem[0].firstChild.nodeValue = (choice != 1) 

xml_file = open('../common-win.props', "w")
doc.writexml(xml_file, encoding="utf-8")
xml_file.close()

print("now restart visual studio...");