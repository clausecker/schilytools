hV6,sum=52587
s 00001/00001/00037
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 27401
c ../common/test-common -> ../../common/test-common
e
s 00038/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 27262
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8eee7e17
G p sccs/tests/cssctests/rmdel/basic.sh
t
T
I 1
#! /bin/sh
# basic.sh:  Tests for the basic operation of "rmdel".

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g=testfile.txt
s=s.$g
d=d.$g
z=z.$g
x=x.$g
p=p.$g

remove command.log log log.stdout log.stderr $g $s $z $x $p $d q.$g

remove $g
echo "hello, this is a test file" > $g

# Prepare an SCCS file with two revisions.
docommand a1 "${admin} -i${g} $s" 0 "" IGNORE
remove $g
docommand a2 "${get} -e $s" 0 "1.1\nnew delta 1.2\n1 lines\n" ""
docommand a3 "${delta} -yNoComment $s" 0 \
	"1.2\n0 inserted\n0 deleted\n1 unchanged\n"   \
    IGNORE
# Remove the second revision.
docommand a4 "${vg_rmdel} -r1.2 $s" 0 "" ""

# Now get the first revision and check it still contains the right data.
# CSSC had a bug in 0.06alpha-pl3 (and earlier), where the delta control
# lines ^AI, ^AE etc. were all deleted.   Bug reported by Peter Kjellerstedt.
docommand a5 "${get} -r1.1 -p $s" 0 "hello, this is a test file\n" IGNORE

# Make sure that the revision we tried to remove is actually now absent.
docommand a6 "${get} -r1.2 -p $s" 1 "" IGNORE

remove command.log log log.stdout log.stderr $g $s $z $x $p $d
success
E 1
