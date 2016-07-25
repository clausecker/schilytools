#! /bin/sh
#
# @(#)test.sh	1.5 16/07/16 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check the test builtin
#

#
# POSIX zero args
# test without arguments fails
#
docommand test00 "$SHELL -c 'test || echo FAIL'" 0 "FAIL\n" ""

#
# POSIX one arg
# test with a zero length argument fails
#
docommand test01 "$SHELL -c 'test \"\" || echo FAIL'" 0 "FAIL\n" ""
#
# test with a non zero length argument succeeds
#
docommand test02 "$SHELL -c 'test \"a\" && echo OK'" 0 "OK\n" ""

#
# POSIX two args -> ! + one arg
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
# POSIX two args -> -r file
# Now with a readable file argument, "-r" is UNIX compatible again.
#
docommand test07 "$SHELL -c 'test \"-r\" a && echo OK'" 0 "OK\n" ""

if [ "$is_bosh" = true ]; then
#
# UNIX kompatibility for "-t" without parameter is important
#
docommand test08 "$SHELL -c 'test \"-t\" > /dev/tty && echo OK'" 0 "OK\n" ""
docommand test09 "$SHELL -c 'set +o posix; test \"-t\" > /dev/null || echo FAIL'" 0 "FAIL\n" ""
docommand test10 "$SHELL -c 'set -o posix; test \"-t\" > /dev/null && echo OK'" 0 "OK\n" ""
fi

#
# Tests for the ksh enhancements -nt/-ot/-ef (newer/older than & equal file)
#
expect_fail_save=$expect_fail
expect_fail=true
docommand -silent -esilent test20 "$SHELL -c 'test a -nt b && echo nt OK || echo nt BAD'" 0 "nt OK\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	echo
	echo "Test $cmd_label is a ksh enhancement."
	echo "Skipping test20..test21."
	echo
else
docommand test20 "$SHELL -c 'test a -nt b && echo nt OK || echo nt BAD'" 0 "nt OK\n" ""
docommand test21 "$SHELL -c 'test b -ot a && echo ot OK || echo ot BAD'" 0 "ot OK\n" ""
docommand test22 "$SHELL -c 'test a -ef a && echo ot OK || echo ot BAD'" 0 "ot OK\n" ""
fi


#
# More POSIX tests based on the number of arguments
#
#
# POSIX three args
#
docommand test30 "$SHELL -c 'test 1 -lt 2 && echo OK'" 0 "OK\n" ""
docommand test31 "$SHELL -c 'test 1 -a 2 && echo OK'" 0 "OK\n" ""
docommand test32 "$SHELL -c 'test ! -a 2 && echo OK'" 0 "OK\n" ""
docommand test33 "$SHELL -c 'test ! -a ! && echo OK'" 0 "OK\n" ""
docommand test34 "$SHELL -c 'test ! != b && echo OK'" 0 "OK\n" ""
docommand test35 "$SHELL -c 'test \"\" -a 2 || echo FAIL'" 0 "FAIL\n" ""
docommand test36 "$SHELL -c 'test \"\" -o 2 && echo OK'" 0 "OK\n" ""
#
# non-valid binary operator
#
docommand test37 "$SHELL -c 'test bla -r a || echo FAIL'" 0 "FAIL\n" NONEMPTY


#
# POSIX three args -> ! + two args
#
docommand test38 "$SHELL -c 'test ! -r a || echo FAIL'" 0 "FAIL\n" ""
docommand test39 "$SHELL -c 'test ! -r b && echo OK'" 0 "OK\n" ""
#
# POSIX three args -> ! + ! + one arg
#
docommand test40 "$SHELL -c 'test ! ! bla && echo OK'" 0 "OK\n" ""

#
# POSIX three args -> ( $2 ) () + one arg
#
docommand test41 "$SHELL -c 'test \( \"bla\" \) && echo OK'" 0 "OK\n" ""
docommand test42 "$SHELL -c 'test \( \"-r\" \) && echo OK'" 0 "OK\n" ""

#
# POSIX four args -> ! + three args
#
docommand test43 "$SHELL -c 'test ! one \"-a\" two || echo FAIL'" 0 "FAIL\n" ""
docommand test44 "$SHELL -c 'test ! ! \"-a\" two || echo FAIL'" 0 "FAIL\n" ""

#
# POSIX four args -> ( $2 $3 ) + () two args
#
docommand test45 "$SHELL -c 'test \( \"-r\" a \) && echo OK'" 0 "OK\n" ""
docommand test46 "$SHELL -c 'test \( ! \"-r\" \) || echo FAIL'" 0 "FAIL\n" ""
docommand test47 "$SHELL -c 'test \( ! \"\" \) && echo OK'" 0 "OK\n" ""


docommand test50 "$SHELL -c 'test  \"-r\" \) || echo FAIL'" 0 "FAIL\n" ""
docommand test51 "$SHELL -c 'test ! \"-r\" \) && echo OK'" 0 "OK\n" ""
#
# Fail because ")" is missing
#
docommand test52 "$SHELL -c 'test \( \"-r\" || echo FAIL'" 0 "FAIL\n" IGNORE
#
# Fail because ")" is missing
#
docommand test53 "$SHELL -c 'test \( \"-r\" a || echo FAIL'" 0 "FAIL\n" IGNORE

#
# This case is unspecified by POSIX, so there may be an error message
#
docommand test54 "$SHELL -c 'test \( \) || echo FAIL'" 0 "FAIL\n" IGNORE
#
# The 3 and 4 argument POSIX case with ( )
#
docommand test55 "$SHELL -c 'test \( a \) && echo OK'" 0 "OK\n" IGNORE
docommand test56 "$SHELL -c 'test ! \( a \) || echo FAIL'" 0 "FAIL\n" IGNORE
#
# This is the "most complex" case defined by POSIX
#
docommand test57 "$SHELL -c 'test ! 2 -lt 1 && echo OK'" 0 "OK\n" ""

#
# Tests to check test builtin syntax error behavior
#
# POSIX hat test geändert: mit einem Argument ist immer strlen(a) > 0 gemeint
#
docommand test100 "$SHELL -c 'test -r; echo \$?'" 0 "0\n" ""

remove a
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
