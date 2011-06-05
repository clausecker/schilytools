h34350
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
. ../common/test-common

remove s.bar 
d=`../../testutils/realpwd`
s=${d}/s.bar

docommand P1 "${vg_admin} -n ${s}" 0 "" IGNORE

remove s.bar 
success
E 1
