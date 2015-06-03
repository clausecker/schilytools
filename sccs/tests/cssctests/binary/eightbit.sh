#! /bin/sh
# eightbit.sh:  Testing for 8-bit clean operation 

# Import common functions & definitions.
. ../../common/test-common
. ../../common/real-thing
. ../../common/config-data


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
if ${SRCROOT}/tests/testutils/uu_decode --decode < s.umsp.uue
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
if ${SRCROOT}/tests/testutils/uu_decode --decode < char255.uue
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
