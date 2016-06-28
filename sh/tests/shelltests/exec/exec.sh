#! /bin/sh
#
# @(#)exec.sh	1.1 16/06/22 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to verify exec redirection function:
#
docommand exec01 "$SHELL -c 'exec 5>f.out; (echo OK 1>&5) > /dev/null; cat f.out'" 0 "OK\n" ""
remove f.out

success
