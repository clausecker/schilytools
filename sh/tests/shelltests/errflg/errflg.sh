#! /bin/sh
#
# @(#)errflg.sh	1.5 18/12/06 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# This causes bash-3.x to fail as bash-3.x does not handle sh -e correctly.
#
docommand ef0 "$SHELL -ce 'for i in 1 2 3; do  ( echo \$i; if test -d . ; then (false; echo 4);  fi ) ; done'" "!=0" "1\n" ""

#
# POSIX: Check whether a I/O redirection error with a builtin or function stops the whole shell
#
docommand ef1 "$SHELL -c 'echo < nonexistant| echo OK'" 0 "OK\n" IGNORE
docommand ef2 "$SHELL -c 'x() { ls; }; x < nonexistant| echo OK'" 0 "OK\n" IGNORE

#
# false is now a builtin, check whether builtins work as expected
#
docommand ef3  "$SHELL -ce 'false && false; echo OK'" 0 "OK\n" ""
docommand ef4  "$SHELL -ce 'false || false; echo OK'" "!=0" "" ""
docommand ef5  "$SHELL -ce 'false; echo OK'" "!=0" "" ""

docommand ef6  "$SHELL -ce 'f() { false; }; while f; do :; done; echo OK'" 0 "OK\n" ""
docommand ef7  "$SHELL -ce 'f() { return \$1; }; f 42 || echo OK'" 0 "OK\n" ""
docommand ef8  "$SHELL -ce 'f() { return \$1; }; if f 42; then : ; fi; echo OK'" 0 "OK\n" ""
docommand ef9  "$SHELL -ce 'f() { return \$1; }; while f 42; do : ; done; echo OK'" 0 "OK\n" ""
docommand ef10 "$SHELL -ce 'f() { return \$1; }; { f 42; echo OK1; } && echo OK2' " 0 "OK1\nOK2\n" ""
docommand ef11 "$SHELL -ce 'f() { return \$1; }; { f 42; echo OK1; f 42; } && : ; echo OK2' " 0 "OK1\nOK2\n" ""
docommand ef12 "$SHELL -ce 'f() { echo OK1; return \$1; }; f 42 || echo OK2'" 0 "OK1\nOK2\n" ""
docommand ef13 "$SHELL -ce 'f() { return \$1; }; { f 42; echo OK1; f 42; } && : ; echo OK2'" 0 "OK1\nOK2\n" ""

#
# eval makes builtins a bit more complex
#
docommand ef14 "$SHELL -ce 'f() { return \$1; }; eval f 42 || echo OK'" 0 "OK\n" ""

#
# The exit code of a command substitution becomes the total exit code
# if there is no following command. So errexit is propagated (see ef22).
#
docommand ef20 "$SHELL -c 'a=\`false; echo 1\`; echo x \$a'" 0 "x 1\n" ""
docommand ef21 "$SHELL -c 'a=\`false; echo 1\` : ; echo x \$a'" 0 "x 1\n" ""
docommand ef22 "$SHELL -ce 'a=\`false; echo 1\`; echo x \$a'" "!=0" "" ""
docommand ef23 "$SHELL -ce 'a=\`false; echo 1\` : ; echo x \$a'" 0 "x\n" ""

success
