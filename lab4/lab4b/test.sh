#!/bin/bash

{ echo "PERIOD=3"; sleep 9; echo "STOP"; sleep 2; echo "SCALE=C"; sleep 2; echo "START"; sleep 10; echo "OFF";} | ./lab4b --log=temp_log.txt > /dev/null

if [ $? -ne 0 ]
then
    echo "Incorrect exit code $?"
else
    echo "Correct exit code 0"
fi

for i in PERIOD=3 STOP SCALE=C START OFF SHUTDOWN
do
    grep "$i" temp_log.txt > /dev/null
    if [ $? -ne 0 ]
    then
	echo "Logging Error $i"
    else
	echo "$i logged successfully"
    fi
done

egrep '[0-9][0-9]:[0-9][0-9]:[0-9][0-9] [0-9]+\.[0-9]\>' temp_log.txt > /dev/null

if [ $? -ne 0 ]
then
	echo "Invalid report -- check temperature"
else
	echo "Valid report"
fi

rm -f temp_log.txt
