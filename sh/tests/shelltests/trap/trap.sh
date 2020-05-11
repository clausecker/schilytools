#! /bin/sh
#
# @(#)trap.sh	1.6 20/04/25 Copyright 2017-2019 J. Schilling
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
if $is_osh; then
	:
else
docommand trap03 "$SHELL -c '(trap \"echo exited\" EXIT; $ECHO bla)'" 0 "bla\nexited\n" ""
fi
fi

#
# Check whether EXIT trap is not called with a non-existing command.
# This problem occurred with previous versions of the Bourne Shell
# when vfork() was used.
#
docommand trap04 "$SHELL -c 'trap \"echo TRAP\" EXIT; /dev/null/ne; trap - EXIT; echo end'" 0 "end\n" NONEMPTY

#
# Ceck the ERR trap that has been introduced by ksh88.
# Note that /bin/false returns 255 on Solaris.
#
if $is_bosh; then
docommand trap50 "$SHELL -c 'trap \"echo exit code \\\$?\" ERR; false; true'" 0 "exit code 1\n" ""
docommand trap51 "$SHELL -c 'trap \"echo exit code \\\$?\" ERR; false'" 1 "exit code 1\n" ""
docommand trap51 "$SHELL -c 'trap \"echo exit code \\\$?\" ERR; false || true'" 0 "" ""
docommand trap53 "$SHELL -c 'trap \"[ \\\$? -ne 0 ] && echo exit code != 0\" ERR; /bin/false; /bin/true'" 0 "exit code != 0\n" ""
docommand trap54 "$SHELL -c 'trap \"[ \\\$? -ne 0 ] && echo exit code != 0\" ERR; /bin/false'" "!=0" "exit code != 0\n" ""
docommand trap55 "$SHELL -c 'trap \"echo exit code \\\$?\" ERR; /bin/false || /bin/true'" 0 "" ""

docommand trap60 "$SHELL -ce 'trap \"echo exit code \\\$?\" ERR; false; true'" 1 "exit code 1\n" ""
docommand trap61 "$SHELL -ce 'trap \"echo exit code \\\$?\" ERR; false'" 1 "exit code 1\n" ""
docommand trap61 "$SHELL -ce 'trap \"echo exit code \\\$?\" ERR; false || true'" 0 "" ""
docommand trap63 "$SHELL -ce 'trap \"[ \\\$? -ne 0 ] && echo exit code != 0\" ERR; /bin/false; /bin/true'" "!=0" "exit code != 0\n" ""
docommand trap64 "$SHELL -ce 'trap \"[ \\\$? -ne 0 ] && echo exit code != 0\" ERR; /bin/false'" "!=0" "exit code != 0\n" ""
docommand trap65 "$SHELL -ce 'trap \"echo exit code \\\$?\" ERR; /bin/false || /bin/true'" 0 "" ""
fi

success
