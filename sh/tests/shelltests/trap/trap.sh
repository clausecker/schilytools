#! /bin/sh
#
# @(#)trap.sh	1.7 20/05/11 Copyright 2017-2020 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# NetBSD has only /usr/bin/true, Linux has only /bin/true
#
# Fallback assignment first:
PTRUE=/bin/true
PFALSE=/bin/false
#
# Now a check...
#
if /usr/bin/true 2>/dev/null; then
	PTRUE=/usr/bin/true
	PFALSE=/usr/bin/false
elif /bin/true 2>/dev/null; then
	PTRUE=/bin/true
	PFALSE=/bin/false
fi

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
docommand trap53 "$SHELL -c 'trap \"[ \\\$? -ne 0 ] && echo exit code != 0\" ERR; $PFALSE; $PTRUE'" 0 "exit code != 0\n" ""
docommand trap54 "$SHELL -c 'trap \"[ \\\$? -ne 0 ] && echo exit code != 0\" ERR; $PFALSE'" "!=0" "exit code != 0\n" ""
docommand trap55 "$SHELL -c 'trap \"echo exit code \\\$?\" ERR; $PFALSE || $PTRUE'" 0 "" ""

docommand trap60 "$SHELL -ce 'trap \"echo exit code \\\$?\" ERR; false; true'" 1 "exit code 1\n" ""
docommand trap61 "$SHELL -ce 'trap \"echo exit code \\\$?\" ERR; false'" 1 "exit code 1\n" ""
docommand trap61 "$SHELL -ce 'trap \"echo exit code \\\$?\" ERR; false || true'" 0 "" ""
docommand trap63 "$SHELL -ce 'trap \"[ \\\$? -ne 0 ] && echo exit code != 0\" ERR; $PFALSE; $PTRUE'" "!=0" "exit code != 0\n" ""
docommand trap64 "$SHELL -ce 'trap \"[ \\\$? -ne 0 ] && echo exit code != 0\" ERR; $PFALSE'" "!=0" "exit code != 0\n" ""
docommand trap65 "$SHELL -ce 'trap \"echo exit code \\\$?\" ERR; $PFALSE || $PTRUE'" 0 "" ""
fi

success
