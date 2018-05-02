h02457
s 00001/00001/00015
d D 1.4 18/04/30 13:07:11 joerg 4 3
c Pfadnamen quoten, damit SPACE darin sein kann
e
s 00001/00001/00015
d D 1.3 15/06/03 00:06:43 joerg 3 2
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00015
d D 1.2 15/06/01 23:55:23 joerg 2 1
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00016/00000/00000
d D 1.1 11/04/25 19:11:35 joerg 1 0
c date and time created 11/04/25 19:11:35 by joerg
e
u
U
f e 0
f y 
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
