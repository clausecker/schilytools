#! /bin/sh
#
# @(#)for.sh	1.1 20/02/09 Copyright 2020 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether the for loop work
#
docommand for00 "$SHELL -c 'for i in 1 2; do echo \$i; done'" 0 "1\n2\n" ""
docommand for01 "$SHELL -c 'set a b; for i in 1 2; do echo \$i; done'" 0 "1\n2\n" ""
docommand for02 "$SHELL -c 'set 1 2; for i; do echo \$i; done'" 0 "1\n2\n" ""
docommand for03 "$SHELL -c 'set 1 2; for i in; do echo \$i; done'" 0 "" ""
docommand for04 "$SHELL -c 'set 1 2; for i do echo \$i; done'" 0 "1\n2\n" ""
docommand for05 "$SHELL -c 'set 1 2; for i; do echo \$i; done'" 0 "1\n2\n" ""

success
