#! /bin/sh
#
# @(#)commamd.sh	1.2 17/08/29 2017 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether the builtin "command" work
#
docommand command01 "$SHELL -c 'command'" 0 "" ""
docommand command02 "$SHELL -c 'command -v ls | grep /bin/ls'" 0 NONEMPTY ""
docommand command03 "$SHELL -c 'command -V ls | grep \"ls.*/bin/ls\"'" 0 NONEMPTY ""
docommand command04 "$SHELL -c 'command -p ls | grep commamd.sh'" 0 NONEMPTY ""
docommand command05 "$SHELL -c 'command env | grep ZzZzZ'" 1 "" ""
docommand command06 "$SHELL -c 'ZzZzZ=bla command env | grep ZzZzZ=bla'" 0 NONEMPTY ""

docommand command10 "$SHELL -c 'ls() { :; }; ls'" 0 "" ""
docommand command11 "$SHELL -c 'ls() { :; }; command ls'" 0 NONEMPTY ""

docommand command20 "$SHELL -c 'command expr 1 \< 2'" 0 "1\n" ""

success
