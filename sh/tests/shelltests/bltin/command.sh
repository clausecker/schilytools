#! /bin/sh
#
# @(#)command.sh	1.4 17/09/02 2017 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether the builtin "command" work
#
docommand command01 "$SHELL -c 'command'" 0 "" ""
docommand command02 "$SHELL -c 'command -v ls | grep /bin/ls'" 0 NONEMPTY ""
docommand command03 "$SHELL -c 'command -V ls | grep \"ls.*/bin/ls\"'" 0 NONEMPTY ""
docommand command04 "$SHELL -c 'command -p ls | grep command.sh'" 0 NONEMPTY ""
docommand command05 "$SHELL -c 'command env | grep ZzZzZ'" 1 "" ""
docommand command06 "$SHELL -c 'ZzZzZ=bla command env | grep ZzZzZ=bla'" 0 NONEMPTY ""

docommand command10 "$SHELL -c 'ls() { :; }; ls'" 0 "" ""
docommand command11 "$SHELL -c 'ls() { :; }; command ls'" 0 NONEMPTY ""

docommand command20 "$SHELL -c 'command expr 1 \< 2'" 0 "1\n" ""

#
# Check whether command -p switches "echo" into POSIX mode.
#
docommand command50 "$SHELL -c 'command echo -n bla'" 0 "-n bla\n" ""
docommand command51 "$SHELL -c 'PATH=/usr/ucb:$PATH; command echo -n bla'" 0 "bla" ""
docommand command52 "$SHELL -c 'PATH=/usr/ucb:$PATH; command -p echo -n bla'" 0 "-n bla\n" ""
docommand command53 "$SHELL -c 'SYSV3=true; export SYSV3; command echo -n \"bla\\\\t\"'" 0 "bla\t" ""
docommand command54 "$SHELL -c 'SYSV3=true; export SYSV3; command -p echo -n \"bla\\\\t\"'" 0 "-n bla\t\n" ""


docommand command80 "$SHELL -c 'echo() { ls com*; }; command echo OK'" 0 "OK\n" ""
docommand command81 "$SHELL -c 'echo() { ls com*; }; command -p echo OK'" 0 "OK\n" ""

success
