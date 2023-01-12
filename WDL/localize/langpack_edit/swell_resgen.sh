#!/bin/sh

DIR="$(dirname $0)"
if [ "v$DIR" = "v" ]; then DIR=. ; fi

if [ -x /usr/bin/php ]; then EXE=/usr/bin/php
elif [ -x /opt/homebrew/bin/php ]; then EXE=/opt/homebrew/bin/php
else EXE=php ; fi

if [ "v$1" != "v" ] && [ "v$1" != "v--quiet" ]; then
  $EXE "$DIR/../../swell/swell_resgen.php" --quiet $1
else
  cd "$DIR"
  $EXE ../../swell/swell_resgen.php $1 *.rc */*.rc */*/*.rc ../jesusonic/*.rc
fi
