#! /bin/sh
#
# @(#)test.sh	1.3 16/06/12 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check the test builtin
#

#
# test without arguments fails
#
docommand test00 "$SHELL -c 'test || echo FAIL'" 0 "FAIL\n" ""
#
# test with a zero length argument fails
#
docommand test01 "$SHELL -c 'test \"\" || echo FAIL'" 0 "FAIL\n" ""
#
# test with a non zero length argument succeeds
#
docommand test02 "$SHELL -c 'test \"a\" && echo OK'" 0 "OK\n" ""
#
# Repeat the same with negation
#
docommand test03 "$SHELL -c 'test ! \"\" && echo OK'" 0 "OK\n" ""
docommand test04 "$SHELL -c 'test ! \"a\" || echo FAIL'" 0 "FAIL\n" ""

#
# POSIX introduced an incompatible interface change and requires to
# select the behavior on the number of arguments only.
# A UNIX test with "-r" as argument complains with "missing argument".
# A POSIX test with the same argument reports non-zero string length.
#
docommand test05 "$SHELL -c 'test \"-r\" && echo OK'" 0 "OK\n" ""
docommand test06 "$SHELL -c 'test ! \"-r\" || echo FAIL'" 0 "FAIL\n" ""

touch a

# 
# Now with a readable file argument, "-r" is UNIX compatible again.
#
docommand test07 "$SHELL -c 'test \"-r\" a && echo OK'" 0 "OK\n" ""

#
# Tests for the ksh enhancements -nt/-ot (newer/older than)
#
expect_fail_save=$expect_fail
expect_fail=true
docommand test08 "$SHELL -c 'test a -nt b && echo nt OK || echo nt BAD'" 0 "nt OK\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	echo
	echo "Test $cmd_label is a ksh enhancement."
	echo "Skipping test09."
	echo
else
docommand test09 "$SHELL -c 'test b -ot a && echo ot OK || echo ot BAD'" 0 "ot OK\n" ""
fi

#
# More POSIX tests based on the number of arguments
#
docommand test10 "$SHELL -c 'test 1 -lt 2 && echo OK'" 0 "OK\n" ""
docommand test11 "$SHELL -c 'test ! -r a || echo FAIL'" 0 "FAIL\n" ""
docommand test12 "$SHELL -c 'test ! -r b && echo OK'" 0 "OK\n" ""
docommand test13 "$SHELL -c 'test \( \"-r\" \) && echo OK'" 0 "OK\n" ""
docommand test14 "$SHELL -c 'test \( ! \"-r\" \) || echo FAIL'" 0 "FAIL\n" ""
docommand test15 "$SHELL -c 'test  \"-r\" \) || echo FAIL'" 0 "FAIL\n" ""
docommand test16 "$SHELL -c 'test ! \"-r\" \) && echo OK'" 0 "OK\n" ""
#
# Fail because ")" is missing
#
docommand test17 "$SHELL -c 'test \( \"-r\" || echo FAIL'" 0 "FAIL\n" IGNORE
#
# Fail because ")" is missing
#
docommand test18 "$SHELL -c 'test \( \"-r\" a || echo FAIL'" 0 "FAIL\n" IGNORE

#
# This case is unspecified by POSIX, so there may be an error message
#
docommand test19 "$SHELL -c 'test \( \) || echo FAIL'" 0 "FAIL\n" IGNORE
#
# The 3 and 4 argument POSIX case with ( )
#
docommand test20 "$SHELL -c 'test \( a \) && echo OK'" 0 "OK\n" IGNORE
docommand test21 "$SHELL -c 'test ! \( a \) || echo FAIL'" 0 "FAIL\n" IGNORE
#
# This is the "most complex" case defined by POSIX
#
docommand test22 "$SHELL -c 'test ! 2 -lt 1 && echo OK'" 0 "OK\n" ""

remove a

#
# Tests to check test builtin syntax error behavior
#
# POSIX hat test geändert: mit einem Argument ist immer strlen(a) > 0 gemeint
#
docommand test100 "$SHELL -c 'test -r; echo \$?'" 0 "0\n" ""

success

exit

From mksh:
        test 2005/10/08 \< 2005/08/21 && echo ja || echo nein 
        test 2005/08/21 \< 2005/10/08 && echo ja || echo nein 
        test 2005/10/08 \> 2005/08/21 && echo ja || echo nein 
        test 2005/08/21 \> 2005/10/08 && echo ja || echo nein 
expected-stdout: 
        nein 
        ja 
        ja 
        nein 
