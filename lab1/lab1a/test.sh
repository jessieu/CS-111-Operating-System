#!/bin/bash

#Test case 1 one invalid option
touch a.txt
touch c1out.txt
touch c1err.txt
./simpsh --rdonly a.txt --nosuchcommand > c1out.txt 2>c1err.txt
if [ $? == 1 ] && [ ! -s c1out.txt ] && [ -s c1err.txt ]
then
    echo "Test case 1 -- Mixed valid and invalid option -- passed."
else
    echo "Test case 1 -- Mixed valid and invalid option -- failed."
fi
rm -f a.txt c1out.txt c1err.txt


#Test case 2 verbose command
touch a.txt
touch 2out.txt
touch 2err.txt
touch c2out.txt
./simpsh --verbose --rdonly a.txt --wronly 2out.txt --wronly 2err.txt --command 0 1 2 cat >c2out.txt 2>&1
if [ $? == 0 ] && [ -s c2out.txt ] && [ ! -s 2err.txt ] && grep -q -- "rdonly a.txt" c2out.txt && grep -q -- "wronly 2out.txt" c2out.txt && grep -q -- "wronly 2err.txt" c2out.txt && grep -q -- "command 0 1 2 cat" c2out.txt
then
    echo "Test case 2 --verbose for command after it -- passes."
else
    echo "Test case 2 --verbose for command after it -- failed."
fi
rm -f a.txt 2out.txt 2err.txt c2out.txt

# Testcase 3 command
echo "Hello World" > a.txt
touch 3out.txt
touch 3err.txt
touch c3out.txt
./simpsh --rdonly a.txt --wronly 3out.txt --wronly 3err.txt --command 0 1 2 cat >c3out.txt 2>&1
if [ $? == 0 ] && [ ! -s c3out.txt ] && [ ! -s 3err.txt ] && cmp a.txt 3out.txt
then
    echo "Test case 3 -- cat command -- passed."
else
    echo "Test case 3 -- cat command -- failed."
fi
rm -f a.txt 3out.txt 3err.txt c3out.txt

