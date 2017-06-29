#! /bin/sh
#
# @(#)exit.sh	1.6 17/06/18 Copyright 2016-2017 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# These tests require a POSIX compliant operating system that returns
# all 32 bits from the exit() code via waitid().
#
# AIX, Cygwin, HP-UX, Linux, OSX and some BSDs are not POSIX compliant
# with respect to exit() and waitid() and thus will fail with the next tests.
# POSIX compliant with respect to exit() and waitid() are:
#	Solaris, SCO UnixWare, FreeBSD, NetBSD.
#
expect_fail_save=$expect_fail
expect_fail=true
docommand e0 "$SHELL -c 'set -o fullexitcode; (exit 1234567890); echo \$?; exit 0'" 0 "1234567890\n" ""
expect_fail=$expect_fail_save

if [ "$failed" = true ]; then
	if [ "$is_bosh" = true ]; then
	echo
	echo "Test 'e0' failed. If the test returned 210 instead of 1234567890"
	echo "the operating system is not POSIX compliant and just implements"
	echo "the historical wait() interface from before 1990."
	echo
	echo "waitid() related tests require a POSIX compliant operating system"
	echo "but this operating system is not POSIX compliant."
	echo "We skip tests 'e1' ... 'e3' as they are expected to fail as well."
	else
	echo "Test 'e0' failed. If the test returned 210 instead of 1234567890"
	echo "this shell may not support waitid() or the operating system"
	echo "is not POSIX compliant."
	echo "We skip tests 'e1' ... 'e3' as they are expected to fail as well."
	fi
else
	docommand e1 "$SHELL -c 'if $SHELL -c \"exit 256\" ; then echo TRUE; else echo FALSE; fi'" 0 "TRUE\n" ""
	docommand e2 "$SHELL -c 'set -o fullexitcode; if $SHELL -c \"exit 256\" ; then echo TRUE; else echo FALSE; fi'" 0 "FALSE\n" ""
	docommand e3 "$SHELL -c '$SHELL -c \"exit 256\" ; echo \$?; exit 0'" 0 "0\n" ""
fi

#
# Check whether the exit code from a matched case label is used
#
docommand e10 "$SHELL -c 'case 10 in 10) (exit 123) ;; esac'" 123 "" ""
#
# Check whether the exit code is 0 when no case label matches
#
docommand e11 "$SHELL -c '(exit 99); case 0 in 10) (exit 123) ;; esac'" 0 "" ""

#
# Check the exit code when "exit" is called without parameter.
# We now use the exit code from the last regular command but
# previously used the exit code from the last command substitution
# if that happened later.
#
docommand e50 "$SHELL -c 'f() { exit \$(exit 1); } ; (f); echo \$?'" 0 "0\n" ""
docommand e51 "$SHELL -c 'f() { return \$(exit 1); } ; f; echo \$?'" 0 "0\n" ""

docommand e52 "$SHELL -c 'f() { (exit 3); a=\$(exit 1) return; } ; f; echo \$?'" 0 "3\n" ""
docommand e53 "$SHELL -c 'f() { (exit 3); return\$(exit 1); } ; f; echo \$?'" 0 "3\n" ""


success
