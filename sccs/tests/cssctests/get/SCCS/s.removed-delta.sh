h61753
s 00073/00000/00000
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
# removed-delta.sh:  Tests for behavious when a delta has been removed.

# Import common functions & definitions.
. ../common/test-common


g=X
s=s.$g
x=x.$g 
z=z.$g
p=p.$g

remove $g $s $x $z $p


# Create an SCCS file with two deltas 1.1 and 2.1; then remove 
# the 2.1 delta with rmdel - getting the 1.1 revision for editing 
# should result in SID 2.1 being re-used.   
#
# CSSC used not to do that - SourceForge bug number #450900.

docommand rd1 "${admin} -n $s" 0 IGNORE IGNORE
docommand rd2 "${vg_get} -r2 -e $s"   0 "1.1
new delta 2.1
0 lines
" IGNORE

docommand rd3 "${delta} -yNoComment $s"   0 IGNORE IGNORE
docommand rd4 "${rmdel} -r2.1 $s"         0 IGNORE IGNORE

# It's the second get -e which we exp[ect to fail if we are 
# suffering from SourceForge bug number #450900.
docommand rd5 "${vg_get} -r2 -e $s"   0 "1.1
new delta 2.1
0 lines
" IGNORE



###
### Now we re-do the whole test again, with two removed deltas, 
### to see if that makes a difference. 
remove $g $s $x $z $p

docommand rd10 "${admin} -n $s" 0 IGNORE IGNORE
docommand rd11 "${vg_get} -r2 -e $s"   0 "1.1
new delta 2.1
0 lines
" IGNORE

docommand rd12 "${delta} -yNoComment $s"   0 IGNORE IGNORE

docommand rd13 "${vg_get} -r2.1 -e $s"   0 "2.1
new delta 2.2
0 lines
" IGNORE

docommand rd14 "${delta} -yNoComment $s"   0 IGNORE IGNORE


docommand rd15 "${rmdel} -r2.2 $s"         0 IGNORE IGNORE
docommand rd16 "${rmdel} -r2.1 $s"         0 IGNORE IGNORE

docommand rd17 "${vg_get} -r2 -e $s"   0 "1.1
new delta 2.1
0 lines
" IGNORE



remove $g $s $x $z $p
success
E 1
