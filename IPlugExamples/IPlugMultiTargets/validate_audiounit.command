#! /bin/sh
# shell script to validate your iplug audiounit using auval
# run from terminal with the argument leaks to perform the leaks test (See auval docs) 

BASEDIR=$(dirname $0)

cd $BASEDIR

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

echo $PII
echo $PDM

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

	auval -v $TYPE $PUID $PMID -w -q
	
	unset MallocStackLogging

else
	
	echo "\nvalidating i386 32 bit... ------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	
	auval -v $TYPE $PUID $PMID
	
	echo "\nvalidating i386 64 bit... ------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	echo "--------------------------------------------------"
	
	auval -64 -v $TYPE $PUID $PMID
	
	#[ -e "/var/db/receipts/com.apple.pkg.Rosetta.plist" ] && echo Rosetta installed || echo Rosetta NOT installed
	#ppc auval not available on 10.6 
	
	#echo "\nvalidating ppc 32 bit... -------------------------"
	#echo "--------------------------------------------------"
	#echo "--------------------------------------------------"
	#echo "--------------------------------------------------"
	#echo "--------------------------------------------------"
	#echo "--------------------------------------------------"
	
	#auval -ppc -v $TYPE $PUID $PMID
fi

echo "done"
	
