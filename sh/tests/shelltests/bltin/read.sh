#! /bin/sh
#
# @(#)read.sh	1.1 16/07/25 2016 J. Schilling
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
docommand read03 "$SHELL -c 'IFS=:; read a b c d e < ii; echo \"\$a \$b \$c \$d \$e\"'" 0 "1 2  4 5\n" ""
docommand read04 "$SHELL -c 'read < i; echo \$REPLY'" 0 "1 2 3 4 5 6 7 8 9\n" ""

remove i ii
success
