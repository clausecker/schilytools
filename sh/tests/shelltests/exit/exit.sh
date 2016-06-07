#! /bin/sh
#
# @(#)exit.sh	1.3 16/06/04 Copyright 2016 J. Schilling
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
	echo
	echo "Test 'e0' failed. If the test returned 210 instead of 1234567890"
	echo "the operating system is not POSIX compliant and just implements"
	echo "the historical wait() interface from before 1990."
	echo
	echo "waitid() related tests require a POSIX compliant operating system"
	echo "but this operating system is not POSIX compliant."
	echo "We skip tests 'e1' ... 'e3' as they are expected to fail as well."
else
	docommand e1 "$SHELL -c 'if $SHELL -c \"exit 256\" ; then echo TRUE; else echo FALSE; fi'" 0 "TRUE\n" ""
	docommand e2 "$SHELL -c 'set -o fullexitcode; if $SHELL -c \"exit 256\" ; then echo TRUE; else echo FALSE; fi'" 0 "FALSE\n" ""
	docommand e3 "$SHELL -c '$SHELL -c \"exit 256\" ; echo \$?; exit 0'" 0 "0\n" ""
fi

success
