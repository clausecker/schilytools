#! /bin/sh

# abspath.sh:  Testing for running admin when the s-file 
#              is specified by an absolute path name.

# Import common functions & definitions.
. ../../common/test-common

remove s.bar 
d=`${SRCROOT}/tests/testutils/realpwd`
s=${d}/s.bar

docommand P1 "${vg_admin} -n '${s}'" 0 "" IGNORE

remove s.bar 
success
