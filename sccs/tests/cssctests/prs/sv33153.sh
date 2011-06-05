#! /bin/sh

# Tests for Savannah bug #33153 ("prs" includes "AUTO NULL DELTAS").


# Import common functions & definitions.
. ../common/test-common

s=s.sv33153
cleanup () {
    remove command.log
}
cleanup
# Deltas 2.1, 3.1, 4.1, 5.1 are "AUTO NULL DELTA"s and all 
# have the same timestamp.  Nevertheless, "prs -l -r4.1" and
# "prs -e -r4.1" should start/stop based on the SID itself, not
# on the timestamp of the delta.
docommand nd1 "${prs} -l -r4.1 -d:I: $s" 0 "6.1\n5.1\n4.1\n" ""
docommand nd2 "${prs} -e -r4.1 -d:I: $s" 0 "4.1\n3.1\n2.1\n1.1\n" ""
docommand nd2 "${prs}    -r4.1 -d:I: $s" 0 "4.1\n" ""

cleanup
