hV6,sum=04421
s 00001/00001/00027
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 44485
c ../common/test-common -> ../../common/test-common
e
s 00028/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 44346
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ec480c9
G p sccs/tests/cssctests/delta/n-option.sh
t
T
I 1
#! /bin/sh
# n-option.sh:  Testing for the -n option of "delta"

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g=foo
s=s.$g

remove $s $g p.$g z.$g


# Create an SCCS file.
docommand d1 "${admin} -n $s"    0 "" IGNORE

# Check the file out for editing.
docommand d2 "${get} -e $s"      0 IGNORE IGNORE

# Check the file back in with delta, using the -n option.
docommand d3 "${vg_delta} -n -y $s" 0 IGNORE IGNORE

# Make sure the g-file hasn't been deleted, and is still writable.
docommand d4 "test -w $g" 0 "" IGNORE



remove $s $g
success
E 1
