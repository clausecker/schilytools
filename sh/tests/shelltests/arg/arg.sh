#! /bin/sh
#
# @(#)arg.sh	1.1 17/07/26 Copyright 2017 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether command line args works as expected.
#
docommand arg00 "$SHELL -c 'echo bla'" 0 "bla\n" ""
docommand arg01 "$SHELL -x -c 'echo bla'" 0 "bla\n" NONEMPTY
docommand arg02 "$SHELL -c -x 'echo bla'" 0 "bla\n" NONEMPTY
docommand arg03 "$SHELL -o xtrace -c 'echo bla'" 0 "bla\n" NONEMPTY
docommand arg04 "$SHELL -c -o xtrace 'echo bla'" 0 "bla\n" NONEMPTY


docommand arg10 "$SHELL -c 'echo bla' -x" 0 "bla\n" ""
docommand arg11 "$SHELL -c 'echo bla \$0 \$@' -x" 0 "bla -x\n" ""
docommand arg12 "$SHELL -c 'echo bla \$0 \$@' -x a b c" 0 "bla -x a b c\n" ""

success
