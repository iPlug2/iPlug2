import fileinput, sys
from parse_config import parse_config

def get_archive_name(projectpath, platform, demo):
  config = parse_config(projectpath)
  
  archive = config["PLUG_NAME"] + "-v" + config['FULL_VER_STR']
  
  if demo == "demo":
    archive = archive + "-" + platform + "-demo"
  else:
    archive = archive + "-" + platform
  
  return archive

def main():
  numargs = len(sys.argv) - 1

  if not (numargs == 3 or numargs == 4):
    print("Usage: get_archive_name.py projectpath platform[win/mac] demo[demo/full]")
    sys.exit(1)
  else:
    return print(get_archive_name(sys.argv[1], sys.argv[2], sys.argv[3]))

if __name__ == '__main__':
  main()
