#! /bin/sh

# Read test core functions
. ../../common/test-common

docommand here3 "$SHELL here3" 0 "1 2 3 EOF) a\n" ""

docommand here-dash "$SHELL here-dash" 0 "A:echo line 3\nB:echo line 1\nline 2\nDASH_CODE:echo line 4)\"\nDASH_CODE:echo line 5\n" ""

success
