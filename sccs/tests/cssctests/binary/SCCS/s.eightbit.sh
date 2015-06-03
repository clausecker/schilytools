h57639
s 00003/00003/00075
d D 1.3 15/06/03 00:06:43 joerg 3 2
c ../common/test-common -> ../../common/test-common
e
s 00002/00002/00076
d D 1.2 15/06/01 23:55:23 joerg 2 1
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00078/00000/00000
d D 1.1 10/04/29 02:05:14 joerg 1 0
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh
# eightbit.sh:  Testing for 8-bit clean operation 

# Import common functions & definitions.
D 3
. ../common/test-common
. ../common/real-thing
. ../common/config-data
E 3
I 3
. ../../common/test-common
. ../../common/real-thing
. ../../common/config-data
E 3


if $binary_support
then
    true
else
    echo "Skipping these tests -- no binary file support."
    exit 0
fi 


g=8bit.txt
p=p.$g
s=s.$g
x=x.$g
z=z.$g


cleanup() {
   remove command.log log log.stdout log.stderr
   remove char255.txt s.char255.txt s.umsp.txt
   remove $p $x $s $z $g
   remove passwd command.log last.command
   remove got.stdout expected.stdout got.stderr expected.stderr
   rm -rf test
}

cleanup

# At the moment, just create an SCCS file from 8-bit characters and
# make sure the checksum is OK.

# If the next line is incomprehensible to you, that's OK. 
# It contains ISO-8859-1 characters.  But the important thing
# is that they are outside the range 0...127.
remove $g
echo "garחon maסana בףהזטךכלסועפשי" >$g
docommand a1 "${vg_admin} -i$g $s" 0 IGNORE IGNORE
docommand a2 "${vg_get} -p $s" 0 "garחon maסana בףהזטךכלסועפשי\n" IGNORE

echo_nonl a3...
D 2
if ../../testutils/uu_decode --decode < s.umsp.uue
E 2
I 2
if ${SRCROOT}/tests/testutils/uu_decode --decode < s.umsp.uue
E 2
then
    echo passed
else
    miscarry uudecode failed.
fi

docommand a4 "${vg_get} -p s.umsp.txt" 0 "garחon maסana בףהזטךכלסועפשי\n" IGNORE


## We must be able to manipulate normally files containing 
## the ISO 8859 character whose code is 255 (y-umlaut).
## EOF is often (-1) and if a char has carelessly been used
## to hold the result of a getchar(), we may detect y-umlaut
## as EOF.  That would be a bug.

echo_nonl a5...
D 2
if ../../testutils/uu_decode --decode < char255.uue
E 2
I 2
if ${SRCROOT}/tests/testutils/uu_decode --decode < char255.uue
E 2
then
    echo passed
else
    miscarry uudecode failed.
fi

remove s.char255.txt
docommand a6 "${vg_admin} -ichar255.txt s.char255.txt" 0 IGNORE IGNORE
docommand a7 "${vg_get} -k -p s.char255.txt" 0 "ÿ\n" "1.1\n1 lines\n"

cleanup
success
E 1
