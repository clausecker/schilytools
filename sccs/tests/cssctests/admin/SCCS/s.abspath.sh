hV6,sum=11107
s 00001/00001/00015
d D 1.4 2018/04/30 13:07:11+0200 joerg 4 3
S s 27482
c Pfadnamen quoten, damit SPACE darin sein kann
e
s 00001/00001/00015
d D 1.3 2015/06/03 00:06:43+0200 joerg 3 2
S s 27404
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00015
d D 1.2 2015/06/01 23:55:23+0200 joerg 2 1
S s 27265
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00016/00000/00000
d D 1.1 2011/04/25 19:11:35+0200 joerg 1 0
S s 26046
c date and time created 11/04/25 19:11:35 by joerg
e
u
U
f e 0
f y 
G r 0e46e8eba623f
G p sccs/tests/cssctests/admin/abspath.sh
t
T
I 1
#! /bin/sh

# abspath.sh:  Testing for running admin when the s-file 
#              is specified by an absolute path name.

# Import common functions & definitions.
D 3
. ../common/test-common
E 3
I 3
. ../../common/test-common
E 3

remove s.bar 
D 2
d=`../../testutils/realpwd`
E 2
I 2
d=`${SRCROOT}/tests/testutils/realpwd`
E 2
s=${d}/s.bar

D 4
docommand P1 "${vg_admin} -n ${s}" 0 "" IGNORE
E 4
I 4
docommand P1 "${vg_admin} -n '${s}'" 0 "" IGNORE
E 4

remove s.bar 
success
E 1
