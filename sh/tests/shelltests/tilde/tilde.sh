#! /bin/sh
#
# @(#)tilde.sh	1.1 19/03/25 Copyright 2019 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether tilde expansion works as expected.
#
docommand tilde00 "HOME=/123 $SHELL -c 'echo ~'" 0 "/123\n" ""
docommand tilde01 "HOME=/123 $SHELL -c 'PATH=~; echo \$PATH'" 0 "/123\n" ""
docommand tilde02 "HOME=/123 $SHELL -c 'export PATH=~; echo \$PATH'" 0 "/123\n" ""

#
# The next work with ksh but not with bosh, since the expansion for ~ from the
# content of $HOME is done in the parser
#
#docommand tilde03 "$SHELL -c 'HOME=/123; echo ~'" 0 "/123\n" ""
#docommand tilde04 "$SHELL -c 'HOME=/123; PATH=~; echo \$PATH'" 0 "/123\n" ""
#docommand tilde05 "$SHELL -c 'HOME=/123; export PATH=~; echo \$PATH'" 0 "/123\n" ""

success
