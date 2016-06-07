#! /bin/sh
#
# @(#)break.sh	1.2 16/06/04 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether the supported operators work
#
docommand br00 "$SHELL -c 'for i in 1 2; do echo \$i; eval break; done'" 0 "1\n" ""
docommand br01 "$SHELL -c 'for i in 1 2; do echo \$i; command break; done'" 0 "1\n" ""

success
