#! /bin/sh
#
# @(#)set.sh	1.5 18/03/11 Copyright 2016-2018 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether set works as expected.
#
docommand se00 "$SHELL -c 's=\"\$(set +o)\"; set -f; eval \"\$s\"; test \"\$s\" = \"\$(set +o)\" && echo OK'" 0 "OK\n" ""
docommand se01 "$SHELL -c 'echo \$#'" 0 "0\n" ""
docommand se02 "$SHELL -c 'set -- 1 2 3; echo \$#'" 0 "3\n" ""
docommand se03 "$SHELL -c 'set -- 1 2 3; set --; echo \$#'" 0 "0\n" ""

#
# "set -c cmd" must fail
#
docommand -noremove se04 "$SHELL -c 'LC_ALL=C; set -c cmd'" "!=0" "" IGNORE
err=`grep 'option' got.stderr`
if [ -z "$err" ]; then
	fail "Test $cmd_label failed: wrong error message"
fi
do_remove

docommand se05 "$SHELL -c 'echo bla'" 0 "bla\n" ""
docommand se06 "$SHELL -c -- 'echo bla'" 0 "bla\n" ""

#
# set with illegal long option and args must not set the arg vector
#
docommand se10 "$SHELL -c -- 'set 1 2 3;command set -o nonexistent a b 2>/dev/null; echo \$@'" 0 "1 2 3\n" ""

success
