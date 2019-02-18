#! /bin/sh
#
# @(#)trap.sh	1.3 19/02/05 Copyright 2017-2019 J. Schilling
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

#
# Check whether EXIT trap is not called with a non-existing command.
# This problem occurred with previous versions of the Bourne Shell
# when vfork() was used.
#
docommand trap04 "$SHELL -c 'trap \"echo TRAP\" EXIT; /dev/null/ne; trap - EXIT; echo end'" 0 "end\n" NONEMPTY

success
