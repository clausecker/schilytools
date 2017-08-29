#! /bin/sh
#
# @(#)read.sh	1.4 17/08/28 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether some builtin commands work
#
echo "1 2 3 4 5 6 7 8 9" > i
echo "1:2::4:5" > ii
docommand read01 "$SHELL -c 'read VAR < i; echo \$VAR'" 0 "1 2 3 4 5 6 7 8 9\n" ""
docommand read02 "$SHELL -c 'read a b c d e f < i; echo \$f \$e \$d \$c \$b \$a'" 0 "6 7 8 9 5 4 3 2 1\n" ""
#
# POSIX requires each non-space delimiter to form a field limit.
#
docommand read03 "$SHELL -c 'IFS=:; read a b c d e < ii; echo \"\$a \$b \$c \$d \$e\"'" 0 "1 2  4 5\n" ""
#
# As we support "select", we also support a default input variable name.
#
docommand read04 "$SHELL -c 'read < i; echo \$REPLY'" 0 "1 2 3 4 5 6 7 8 9\n" ""
#
# If the delimiter is a space, repeated delimiters are seen as a single
# field limit, the same way as it has been in the historic shell.
#
echo "1  2  3  4  5  6  7  8  9" > is
docommand read05 "$SHELL -c 'read a b c d e f < is; echo \$f \$e \$d \$c \$b \$a'" 0 "6 7 8 9 5 4 3 2 1\n" ""

#
# POSIX removes repeated "IFS white space"
#
echo "  ::  1::2: 3  :4  5  6  7  8  9" > iss
docommand read06 "$SHELL -c 'IFS=\": \";read a b c d e f < iss; echo \$f, \$e, \$d, \$c, \$b, \$a,'" 0 "3 4 5 6 7 8 9, 2, , 1, , ,\n" ""

remove i ii is iss

#
# Check whether read without newline exits with 1 and whether this
# causes an exit of the shell with -e
#
docommand read10 "$SHELL -c 'read LINE < /dev/null; echo \$?'" 0 "1\n" ""
docommand read11 "$SHELL -ce 'read LINE < /dev/null; echo \$?'" 1 "" ""

success
