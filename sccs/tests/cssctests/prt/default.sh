#! /bin/sh

# default.sh:  Test the default behaviour of prt.

# Import common functions & definitions.
. ../../common/test-common
. ../../common/need-prt

s=s.testfile

remove $s
${SRCROOT}/tests/testutils/uu_decode --decode < testfile.uue || miscarry could not uudecode testfile.uue.


do_output d1 "${vg_prt} $s" 0 expected/default.1 IGNORE

remove $s
success
