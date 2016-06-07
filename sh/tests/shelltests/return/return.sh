#! /bin/sh
#
# @(#)return.sh	1.2 16/06/04 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

docommand r1 "$SHELL -c 'echo a; return; echo b'" 1 "a\n" IGNORE

success
