#! /bin/sh

# Read test core functions
. ../../common/test-common

docommand e0 "$SHELL -c 'set -o fullexitcode; (exit 1234567890); echo \$?; exit 0'" 0 "1234567890\n" ""
docommand e1 "$SHELL -c 'if $SHELL -c \"exit 256\" ; then echo TRUE; else echo FALSE; fi'" 0 "TRUE\n" ""
docommand e2 "$SHELL -c 'set -o fullexitcode; if $SHELL -c \"exit 256\" ; then echo TRUE; else echo FALSE; fi'" 0 "FALSE\n" ""
docommand e3 "$SHELL -c '$SHELL -c \"exit 256\" ; echo \$?; exit 0'" 0 "0\n" ""

success
