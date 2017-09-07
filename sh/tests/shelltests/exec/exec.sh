#! /bin/sh
#
# @(#)exec.sh	1.2 17/09/02 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to verify exec redirection function:
#
docommand exec01 "$SHELL -c 'exec 5>f.out; (echo OK 1>&5) > /dev/null; cat f.out'" 0 "OK\n" ""
remove f.out

docommand exec50 "$SHELL -c 'var=bla exec sh -c \"echo \\\$var\"'" 0 "bla\n" ""
docommand exec51 "$SHELL -c 'var=bla command exec sh -c \"echo \\\$var\"'" 0 "bla\n" ""


success
