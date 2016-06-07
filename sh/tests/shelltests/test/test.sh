#! /bin/sh
#
# @(#)test.sh	1.2 16/06/04 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check the test builtin
#
docommand test00 "$SHELL -c 'test || echo FAIL'" 0 "FAIL\n" ""
docommand test01 "$SHELL -c 'test \"\" || echo FAIL'" 0 "FAIL\n" ""
docommand test02 "$SHELL -c 'test \"a\" && echo OK'" 0 "OK\n" ""

touch a
docommand test03 "$SHELL -c 'test a -nt b && echo nt OK || echo nt BAD'" 0 "nt OK\n" ""
docommand test04 "$SHELL -c 'test b -ot a && echo ot OK || echo ot BAD'" 0 "ot OK\n" ""
remove a

#
# Tests to check test builtin syntax error behavior
#
# POSIX hat test geändert: mit einem Argument ist immer strlen(a) > 0 gemeint
#
#docommand test100 "$SHELL -c 'test -e; echo \$?'" 0 "1\n" ""

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
