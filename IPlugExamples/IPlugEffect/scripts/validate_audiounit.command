#! /bin/sh
# shell script to validate your iplug audiounit using auval
# run from terminal with the argument leaks to perform the leaks test (See auval docs) 

BASEDIR=$(dirname $0)

cd $BASEDIR

OS_VERSION=`sw_vers -productVersion | egrep -o '10\.[0-9]+'`

x86_ARGS=""
x64_ARGS=""

if [[ $OS_VERSION == "10.9" ]] || [[ $OS_VERSION == "10.10" ]]
then
  x86_ARGS="-32"
  x64_ARGS=""
else
  x86_ARGS=""
  x64_ARGS="-64"
fi

PUID=`echo | grep PLUG_UNIQUE_ID resource.h`
PUID=${PUID//\#define PLUG_UNIQUE_ID }
PUID=${PUID//\'}

PMID=`echo | grep PLUG_MFR_ID resource.h`
PMID=${PMID//\#define PLUG_MFR_ID }
PMID=${PMID//\'}

PII=`echo | grep PLUG_IS_INST resource.h`
PII=${PII//\#define PLUG_IS_INST }

PDM=`echo | grep PLUG_DOES_MIDI resource.h`
PDM=${PDM//\#define PLUG_DOES_MIDI }

TYPE=aufx

if [ $PII == 1 ] # instrument
then
	TYPE=aumu
else
	if [ $PDM == 1 ] # midi effect
	then
		TYPE=aumf
	fi
fi

if [ "$1" == "leaks" ]
then
	echo "testing for leaks (i386 32 bit)"
	echo 'launch a new shell and type: ps axc|awk "{if (\$5==\"auvaltool\") print \$1}" to get the pid';
	echo "then leaks PID"
	
	export MallocStackLogging=1
	set env MallocStackLoggingNoCompact=1

	auval $x86_ARGS -v $TYPE $PUID $PMID -w -q
	
	unset MallocStackLogging

else
	
	echo "\nvalidating i386 32 bit... ------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	
	auval $x86_ARGS -v $TYPE $PUID $PMID
	
	echo "\nvalidating i386 64 bit... ------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	
	auval $x64_ARGS -v $TYPE $PUID $PMID

fi

echo "done"
	
