#! /bin/sh
#
# @(#)here03.sh	1.1 17/03/17 Copyright 2017 J. Schilling
#

# Read test core functions
. ../../common/test-common
. ${SRCROOT}/tests/bin/echo_nonl

#
# Basic tests for here documents in interactive mode.
# This is needed because the POSIX standard expects the shell to expand
# PS1, PS2 and PS4 and this may interfere with the space used for reading
# the here codument.
#

#
# heredoc-101
# Basic Check whether here documents work in interactive mode
#
cat > x <<"XEOF"
cat << EOF
1
2
3
EOF
XEOF
docommand here101 "PS1='PS1 
' PS2='PS2 
' ENV=/./ $SHELL -si <./x" 0 "1\n2\n3\n" IGNORE
remove x

#
# heredoc-102
# Check whether here documents work in interactive mode while PS1 and PS2
# are subject to macro expansion.
#
cat > x <<"XEOF"
cat << EOF
1
2
3
EOF
XEOF
docommand here102 "PS1='$SHELL 
' PS2='$PATH 
' ENV=/./ $SHELL -si <./x" 0 "1\n2\n3\n" IGNORE
remove x

success
