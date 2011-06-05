#! /bin/sh

# cutoff.sh:  Tests for prs -e and prs -l (with both -c and -r).

# Import common functions & definitions.
. ../common/test-common

s="../year-2000/s.y2k.txt"
cleanup () {
    remove command.log
}
cleanup

# First, basic tests with a cutoff which does not co-incide precisely
# with the date of an existing delta.
docommand co1 "${prs} -e -c70 -d:I: $s" 0 "1.1\n" IGNORE
docommand co2 "${prs} -l -c70 -d:I: $s" 0 "1.5\n1.4\n1.3\n1.2\n" IGNORE

# Now, choose a data which does coincide.  The delta whose time matches
# should always be printed (in this case, 1.2).
docommand co3 "${prs} -e -c991231235959 -d:I: $s" 0 "1.2\n1.1\n" IGNORE
docommand co4 "${prs} -l -c991231235959 -d:I: $s" 0 "1.5\n1.4\n1.3\n1.2\n" IGNORE


# These tests are fundamentally the same but with -r instead of -c.
docommand co5 "${prs} -e -r1.2 -d:I: $s" 0 "1.2\n1.1\n" IGNORE
docommand co6 "${prs} -l -r1.2 -d:I: $s" 0 "1.5\n1.4\n1.3\n1.2\n" IGNORE

cleanup
