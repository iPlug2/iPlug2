#! /bin/sh
# shell script to validate your iplug audiounit using auval
# first argument is the config.h of the iplug project
# run from terminal with the 2nd argument leaks to perform the leaks test (See auval docs)

BASEDIR=$(dirname $0)

cd $BASEDIR

PUID=`echo | grep PLUG_UNIQUE_ID $1`
PUID=${PUID//\#define PLUG_UNIQUE_ID }
PUID=${PUID//\'}

PMID=`echo | grep PLUG_MFR_ID $1`
PMID=${PMID//\#define PLUG_MFR_ID }
PMID=${PMID//\'}

PTYPE=`echo | grep PLUG_TYPE $1`
PTYPE=${PTYPE//\#define PLUG_TYPE }

PDMI=`echo | grep PLUG_DOES_MIDI_IN $1`
PDMI=${PDMI//\#define PLUG_DOES_MIDI_IN }

TYPE=aufx

if [ "$PTYPE" == "0" ] # fx or midi controlled fx
then
  if [ "$PDMI" == "0" ] # fx
  then
    TYPE=aufx
  else # midi controlled fx
    TYPE=aumf
  fi
fi

if [ "$PTYPE" == "1" ] # instrument
then
  TYPE=aumu
fi

if [ "$PTYPE" == "2" ] # midi processor
then
  TYPE=aumi
fi

if [ "$2" == "leaks" ]
then
  echo "testing for leaks"
  echo 'launch a new shell and type: ps axc|awk "{if (\$5==\"auvaltool\") print \$1}" to get the pid';
  echo "then leaks PID"
  export MallocStackLogging=1
  set env MallocStackLoggingNoCompact=1
  auval -v $TYPE $PUID $PMID -w -q
  unset MallocStackLogging
elif [ "$2" == "rtsafe" ]
then
  echo "testing for real time safety"
  sudo auval -v $TYPE $PUID $PMID -real-time-safety
else
  echo "regular tests"
  auval -stress -v $TYPE $PUID $PMID
fi

exit
