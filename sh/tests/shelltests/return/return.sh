#! /bin/sh

# Read test core functions
. ../../common/test-common

docommand r1 "$SHELL -c 'echo a; return; echo b'" 1 "a\n" IGNORE

success
