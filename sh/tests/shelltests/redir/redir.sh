#! /bin/sh
#
# @(#)redir.sh	1.2 17/06/28 Copyright 2017 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether I/O redirection works as expected.
#
docommand redir01 "$SHELL -c 'echo foo > /dev/null'" 0 "" ""
docommand redir02 "$SHELL -c '> /dev/null echo foo'" 0 "" ""
docommand redir03 "$SHELL -c 'echo foo < /dev/null'" 0 "foo\n" ""
docommand redir04 "$SHELL -c '< /dev/null echo foo'" 0 "foo\n" ""

docommand redir10 "$SHELL -c 'if true; then echo foo; else echo bar; fi'" 0 "foo\n" ""
docommand redir11 "$SHELL -c 'if false; then echo foo; else echo bar; fi'" 0 "bar\n" ""
docommand redir12 "$SHELL -c 'if true; then echo foo; else echo bar; fi > /dev/null'" 0 "" ""
docommand redir13 "$SHELL -c 'if false; then echo foo; else echo bar; fi > /dev/null'" 0 "" ""
docommand redir14 "$SHELL -c 'if true; then echo true; elif true; then echo elif; fi > /dev/null'" 0 "" ""

#
# Errors from absolute path names are printed in the redirected child.
# Errors from PATH name search are printed in the parent and have not
# been redirected in former times.
#
docommand redir20 "$SHELL -c '/does/not/exist 2>/dev/null'" 127 "" ""
docommand redir21 "$SHELL -c 'does-not-exist 2>/dev/null'" 127 "" ""

success
