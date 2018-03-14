cd "${0%/*}"

LLDBINIT_PATH=~/.lldbinit
SCRIPT_PATH="$PWD/iplug_lldb_xcode.py"

if [ -e "$LLDBINIT_PATH" ]
then
  echo "***********************************************"
  echo "WARNING!!! ~/.lldbinit already exists!"
  echo "Please edit manually to include the line below:"
  echo "command script import $SCRIPT_PATH"
  echo "***********************************************"
else
  echo "command script import $SCRIPT_PATH" >> "$LLDBINIT_PATH"
  chmod +x "$LLDBINIT_PATH"
fi
