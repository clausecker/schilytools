hV6,sum=47235
s 00001/00001/00032
d D 1.3 2015/06/03 00:06:44+0200 joerg 3 2
S s 05195
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00032
d D 1.2 2015/06/01 23:55:23+0200 joerg 2 1
S s 05056
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00033/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 03837
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8edbef87
G p sccs/tests/cssctests/large/admin.sh
t
T
I 1
#! /bin/sh
# admin.sh:  The creation of very large (>99999 lines) SCCS files.

# Import common functions & definitions.
D 3
. ../common/test-common
E 3
I 3
. ../../common/test-common
E 3

g=bigfile.txt
s=s.$g
z=z.$g
x=x.$g
p=p.$g

lines=100002

remove command.log log log.stdout log.stderr $g $s $z $x $p

D 2
( ../../testutils/yes '%C%' | head -${lines} > $g )  || miscarry Cannot create large input file.
E 2
I 2
( ${SRCROOT}/tests/testutils/yes '%C%' | head -${lines} > $g )  || miscarry Cannot create large input file.
E 2

docommand A1 "${vg_admin} -i${g} ${s}" 0 "" ""
mv ${g} old.${g} || miscarry "Rename failed"

# Make sure we can retrieve the file.
docommand A2 "${vg_get} -k $s" 0 "1.1\n100002 lines\n" ""
diff old.${g} ${g} || fail A2 "cannot correctly retrieve stored file"
remove old.${g} ${g}
docommand A3 "${get} $s" 0 "1.1\n100002 lines\n" ""

# Make sure the number of lines is correct.
docommand A4 "${prs} -d:Li: $s" 0 "99999\n" ""


remove command.log log log.stdout log.stderr $g $s $z $x $p
success
E 1
