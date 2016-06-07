#! /bin/sh
#
# @(#)echo.sh	1.2 16/06/04 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check the echo builtin
#
docommand echo00 "$SHELL -c 'echo 1 2 3'" 0 "1 2 3\n" ""

#
# Default behavior is POSIX behavior
#
docommand echo01 "$SHELL -c 'unset PATH; echo -n 1 2 3'" 0 "-n 1 2 3\n" ""

#
# Switch to UCB behavior: honor -n but not escape sequences
#
docommand echo02 "$SHELL -c 'PATH=/usr/ucb:$PATH; echo -n 1 2 3'" 0 "1 2 3" ""
docommand echo03 "$SHELL -c 'PATH=/usr/ucb:$PATH; echo 1 2 3\\\\t'" 0 "1 2 3\\\\t\n" ""

#
# Switch to SVR3 behavior: honor -n and escape sequences
#
docommand echo04 "$SHELL -c 'SYSV3=; export SYSV3; echo 1 2 3\\\\t'" 0 "1 2 3\t\n" ""
docommand echo05 "$SHELL -c 'SYSV3=; export SYSV3; echo -n 1 2 3'" 0 "1 2 3" ""

success
