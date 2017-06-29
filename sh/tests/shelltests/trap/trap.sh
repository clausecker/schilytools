#! /bin/sh
#
# @(#)trap.sh	1.2 17/06/28 Copyright 2017 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether trap works as expected.
#
docommand trap00 "$SHELL -c 'trap \"echo exited\" EXIT; echo bla'" 0 "bla\nexited\n" ""
docommand trap01 "$SHELL -c '(trap \"echo exited\" EXIT; echo bla)'" 0 "bla\nexited\n" ""

ECHO=""
type /bin/echo > /dev/null 2> /dev/null && ECHO=/bin/echo
[ $? -ne 0 ] && type /usr/bin/echo > /dev/null 2> /dev/null && ECHO=/usr/bin/echo

if [ -z "$ECHO" ]; then
echo "No PATH for echo found, skipping test trap03"
else
#
# Check whether EXIT trap is called even when the shell might
# optimise out a fork for the last command in a subshell.
#
docommand trap03 "$SHELL -c '(trap \"echo exited\" EXIT; $ECHO bla)'" 0 "bla\nexited\n" ""
fi

success
