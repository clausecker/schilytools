#! /bin/sh
#
# @(#)set.sh	1.2 16/06/04 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether set works as expected.
#
docommand se00 "$SHELL -c 's=\"\$(set +o)\"; set -f; eval \"\$s\"; test \"\$s\" = \"\$(set +o)\" && echo OK'" 0 "OK\n" ""
docommand se01 "$SHELL -c 'echo \$#'" 0 "0\n" ""
docommand se02 "$SHELL -c 'set -- 1 2 3; echo \$#'" 0 "3\n" ""
docommand se03 "$SHELL -c 'set -- 1 2 3; set --; echo \$#'" 0 "0\n" ""

success
