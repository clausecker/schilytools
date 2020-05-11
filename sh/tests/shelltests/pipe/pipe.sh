#! /bin/sh
#
# @(#)pipe.sh	1.2 20/04/26 2016-2020 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether pipe commands work
# This tests are needed as our new optimized pipe did cause some
# problems in the past.
# Caution: this test may hang if a bug similar to the bug present
# between December 2015 and July 2016 is present.
#
docommand pipe01 "$SHELL -c 'echo 1 |  while read a; do :; done | cat'" 0 "" ""
docommand pipe02 "$SHELL -c 'echo 1 |  while read a; do echo OK; done | cat'" 0 "OK\n" ""
docommand pipe03 "$SHELL -c 'a() { echo 1 |  while read a; do :; done | cat; }; a'" 0 "" ""
docommand pipe04 "$SHELL -c 'a() { echo 1 |  while read a; do :; done | cat; }; a=\`a\`'" 0 "" ""
docommand pipe05 "$SHELL -c 'a() { echo 1 | b; }; b() { while read a; do :; done | cat; }; a=\`a\`'" 0 "" ""
docommand pipe06 "$SHELL -c 'a() { echo 1 | b; }; b() { while read b; do echo \$b; done | cat; }; c=\`a\`; echo \$c'" 0 "1\n" ""

#
# This checks for a bug reported in 2020 that caused the loop to reset it's
# input to stdin after /bin/echo has been called.
# In order to be able to finish the test in error case, we set stdin to "xfile"
# and get extra input instead of a hang because the command tried to read from
# the tty.
#
echo bla > xfile
docommand pipe10 "$SHELL -c 'exec 0< xfile; echo test | cat | while read b; do /bin/echo \$b; done'" 0 "test\n" ""

remove xfile
success
