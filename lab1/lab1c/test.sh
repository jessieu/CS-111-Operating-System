#!/bin/bash

# Create test file
cat > test.txt <<EOF
if the heart of the bell breaks
catastrophe will strike: that is the legend

last time, a flood;
next, who can know but the bell, its cloven meat
quiet crack of bronze anticipating its own demise
drowned in the clang-like fist & metal murmur

what other creature keeps its heart inside its head full
of the hour’s secrets & tender prayers?

I do not doubt the stories—
how bells say & say even after
they have been lost
in the rubble or at sea

say & say how they paced
into hiding, or grieved their knell siblings
melted for war’s hungry rounds,
how they stilled their blood
in the chill of state restraint,
that longest & most silent hour
of the body’s yearning, say:
a silver bell is really a thumbscrew, no
heart at all, say: touch me,
I am no longer a bell if I do not ring

how bells say & say
perfectly tuned by centuries, their
hammered & quarreling overtures
haunting ringent ears
calling the forgotten, calling me, I’m called

to pray in the clamor, holy brazen speech
carapaced in its stony tower, devout lick of metal
to the ferrous brim, its chime sway waking my deadest earth

to pray which is to taste you ring you
loud to greet you a thousand times
offer my heart to your mouth & bellow the walls—
anything that reminds me how small we are:

to be swallowed by the boom of fullest sound,
the agony & wonder of wanting the Beloved’s more
crisp break of a mere day off the year’s chanted yearnings,
intonations that we will never last
longer than a breath in the face of the infinite

to want to be pealed by the hour, to say & say
to invite the lightning by this thunderlust,
lure the cry of livening a shatter
near to feel the crack of the ampersand
organ in the vesper’s dusky light

to know the catastrophe to come
but nonetheless
ring & ring & ring & ring
EOF


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
if [ $? == 0 ] && [ ! -s c3out.txt ] && [ ! -s 3err.txt ] && cmp -s a.txt 3out.txt
then
    echo "Test case 3 -- cat command -- passed."
else
    echo "Test case 3 -- cat command -- failed."
fi
rm -f a.txt 3out.txt 3err.txt c3out.txt

# Testcase 4 pipe and command
cp test.txt b2.txt
touch c9out.txt
./simpsh --pipe --rdwr b2.txt --creat --rdwr test9err.txt --creat \
  --rdwr test9out.txt --pipe --command 2 1 3 cat --command 0 6 3 less - \
  --command 5 4 3 wc >c9out.txt 2>&1
cat test.txt | less | wc > result.txt
sleep 1
if [ $? == 0 ] &&  [ -e test9err.txt ]&& [ -e test9out.txt ] \
  && [ ! -s test9err.txt ] && cmp -s result.txt test9out.txt \
  && [ ! -s c9out.txt ]
then
    echo "Test case 4 -- pipe -- passed"
else
    echo "Test case 4 -- pipe -- failed"
fi
rm -f b2.txt test9err.txt test9out.txt c9out.txt

# Testcase 5 use closed fd
touch c5out.txt
touch c5err.txt
./simpsh --wronly test5out.txt --creat --wronly test5err.txt --rdonly test.txt \
  --creat --rdwr a5.txt --close 3 --command 3 0 1 cat >c5out.txt 2>c5err.txt
if [ $? == 1 ] && [ ! -s test5err.txt ] && [ ! -s c5out.txt ] && [ -s c5err.txt ]
then
    echo "Test case 5 -- use closed fd -- passed"
else
    echo "Test case 5 -- use closed fd -- failed"
fi
rm -f c5out.txt c5err.txt a5.txt test5err.txt

# Test case 6 append
echo "hello world" > test6.txt
touch test6err.txt
touch c6out.txt
touch c6err.txt
./simpsh --rdonly test.txt --append --wronly test6.txt --wronly test6err.txt\
  --command 0 1 2 cat >c6out.txt 2>c6err.txt
if [ $? == 0 ] && grep -q -- "hello world" test6.txt\
   && grep -q -- "if the heart of the bell breaks" test6.txt
then
    echo "Test case 6 -- append -- passed"
else
    echo "Test case 6 -- append -- failed"
fi
rm -f c6out.txt c6err.txt test6err.txt test6.txt
rm -f test.txt
