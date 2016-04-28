#! /bin/sh

# Read test core functions
. ../../common/test-common

#
# This causes bash-3.x to fail as bash-3.x does not handle sh -e correctly.
#
docommand ef0 "$SHELL -ce 'for i in 1 2 3; do  ( echo $i; if test -d . ; then (false; echo 4);  fi ) ; done' | grep 2 " 1 "" ""

#
# POSIX: Check whether a I/O redirection error with a builtin or function stops the whole shell
#
docommand ef1 "$SHELL -c 'echo < nonexistant| echo OK'" 0 "OK\n" IGNORE
docommand ef2 "$SHELL -c 'x() { ls; }; x < nonexistant| echo OK'" 0 "OK\n" IGNORE

success
