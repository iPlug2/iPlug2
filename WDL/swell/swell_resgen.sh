#!/bin/sh

DIR="$(dirname $0)"
if [ "v$DIR" = "v" ]; then DIR=. ; fi

"$DIR/swell_resgen.pl" $*
