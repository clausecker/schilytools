#! /bin/sh
#
# @(#)error.sh	1.1 16/06/18 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to verify the consequences of shell errors
#

#
# Test shell language syntax error
#
docommand error00 "$SHELL -c 'LC_ALL=C; while true; done; echo FAIL'" "!=0" "" NONEMPTY

#
# Test all special builtins with a "utility error".
# Expected is: exit != 0 before "FAIL" is printed.
#
docommand error01 "$SHELL -c 'LC_ALL=C; . xxzzy; echo FAIL'" "!=0" "" NONEMPTY
# : never fails
#docommand error02 "$SHELL -c 'LC_ALL=C; : xxzzy; echo FAIL'" "!=0" "" NONEMPTY
docommand error03 "$SHELL -c 'LC_ALL=C;for i in a; do break 0; done; echo FAIL'" "!=0" "" IGNORE
docommand error04 "$SHELL -c 'LC_ALL=C;for i in a; do continue 0; done; echo FAIL'" "!=0" "" IGNORE
docommand error05 "$SHELL -c 'LC_ALL=C; eval . xxzzy; echo FAIL'" "!=0" "" NONEMPTY
docommand error06 "$SHELL -c 'LC_ALL=C; exec xxzzy; echo FAIL'" "!=0" "" NONEMPTY
docommand error07 "$SHELL -c 'LC_ALL=C; exit xxzzy; echo FAIL'" "!=0" "" NONEMPTY
docommand error08 "$SHELL -c 'LC_ALL=C; export -z; echo FAIL'" "!=0" "" NONEMPTY
docommand error09 "$SHELL -c 'LC_ALL=C; readonly -z; echo FAIL'" "!=0" "" NONEMPTY
docommand error10 "$SHELL -c 'LC_ALL=C; return xxzzy; echo FAIL'" "!=0" "" NONEMPTY
docommand error11 "$SHELL -c 'LC_ALL=C; set -z; echo FAIL'" "!=0" "" NONEMPTY
docommand error12 "$SHELL -c 'LC_ALL=C; shift xxzzy; echo FAIL'" "!=0" "" NONEMPTY
# times never fails even with sh and ksh
#docommand error13 "$SHELL -c 'LC_ALL=C; times xxzzy; echo FAIL'" "!=0" "" NONEMPTY
# even sh and ksh do not fail with trap syntax errors
#docommand error14 "$SHELL -c 'LC_ALL=C; trap \"\" xxzzy; echo FAIL'" "!=0" "" NONEMPTY
docommand error15 "$SHELL -c 'LC_ALL=C; unset -z; echo FAIL'" "!=0" "" NONEMPTY

#
# Other utility (not a special builtin) shall not exit
#
docommand error16 "$SHELL -c 'LC_ALL=C; command -z; echo OK'" 0 "OK\n" NONEMPTY

#
# Redirection errors with special builtins shall exit
#
docommand error17 "$SHELL -c 'LC_ALL=C; . > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error18 "$SHELL -c 'LC_ALL=C; : > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error19 "$SHELL -c 'LC_ALL=C; break > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error20 "$SHELL -c 'LC_ALL=C; continue > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error21 "$SHELL -c 'LC_ALL=C; eval echo > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error22 "$SHELL -c 'LC_ALL=C; exec > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error23 "$SHELL -c 'LC_ALL=C; exit 0 > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error24 "$SHELL -c 'LC_ALL=C; export PATH > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error25 "$SHELL -c 'LC_ALL=C; readonly PATH > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error26 "$SHELL -c 'LC_ALL=C; return > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error27 "$SHELL -c 'LC_ALL=C; set > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error28 "$SHELL -c 'LC_ALL=C; set 1 2 3; shift > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error29 "$SHELL -c 'LC_ALL=C; times > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error30 "$SHELL -c 'LC_ALL=C; trap > /; echo FAIL'" "!=0" "" NONEMPTY
docommand error31 "$SHELL -c 'LC_ALL=C; unset LC_ALL > /; echo FAIL'" "!=0" "" NONEMPTY

docommand error32 "$SHELL -c 'LC_ALL=C; echo > /; echo OK'" 0 "OK\n" NONEMPTY
docommand error33 "$SHELL -c 'LC_ALL=C; readonly LC_ALL; LC_ALL=6; echo FAIL'" "!=0" "" NONEMPTY
docommand error34 "$SHELL -c 'LC_ALL=C; : \"\${x!y}\"; echo FAIL'" "!=0" "" NONEMPTY

#
# Test shell language syntax error interactive may not exit
#
docommand error100 "$SHELL -ci 'LC_ALL=C; while true; done
echo OK'" 0 "OK\n" NONEMPTY

#
# Test all special builtins with a "utility error".
# Expected is no exit in interactive mode
#
docommand error101 "$SHELL -ci 'LC_ALL=C; . xxzzy
echo OK'" 0 "OK\n" NONEMPTY
docommand error102 "$SHELL -ci 'LC_ALL=C; : xxzzy
echo OK'" 0 "OK\n" NONEMPTY
docommand error103 "$SHELL -ci 'LC_ALL=C;for i in a; do break 0; done
echo OK'" 0 "OK\n" IGNORE
docommand error104 "$SHELL -ci 'LC_ALL=C;for i in a; do continue 0; done
echo OK'" 0 "OK\n" IGNORE
docommand error105 "$SHELL -ci 'LC_ALL=C; eval . xxzzy
echo OK'" 0 "OK\n" NONEMPTY
# exec always exits even with sh and ksh
#docommand error106 "$SHELL -ci 'LC_ALL=C; exec xxzzy
#echo OK'" 0 "OK\n" NONEMPTY
# exit alays exits
#docommand error107 "$SHELL -ci 'LC_ALL=C; exit xxzzy
#echo OK'" 0 "" NONEMPTY
docommand error108 "$SHELL -ci 'LC_ALL=C; export -z
echo OK'" 0 "OK\n" NONEMPTY
docommand error109 "$SHELL -ci 'LC_ALL=C; readonly -z
echo OK'" 0 "OK\n" NONEMPTY
docommand error110 "$SHELL -ci 'LC_ALL=C; return xxzzy
echo OK'" 0 "OK\n" NONEMPTY
docommand error111 "$SHELL -ci 'LC_ALL=C; set -z
echo OK'" 0 "OK\n" NONEMPTY
docommand error112 "$SHELL -ci 'LC_ALL=C; shift xxzzy
echo OK'" 0 "OK\n" NONEMPTY
# times never fails even with sh and ksh
#docommand error113 "$SHELL -c 'LC_ALL=C; times xxzzy; echo FAIL'" "!=0" "" NONEMPTY
docommand error114 "$SHELL -ci 'LC_ALL=C; trap \"\" xxzzy
echo OK'" 0 "OK\n" NONEMPTY
docommand error115 "$SHELL -ci 'LC_ALL=C; unset -z
echo OK'" 0 "OK\n" NONEMPTY

#
# Other utility (not a special builtin) shall not exit
#
docommand error116 "$SHELL -ci 'LC_ALL=C; command -z
echo OK'" 0 "OK\n" NONEMPTY

#
# Redirection errors with special builtins in interactive mode shall not exit
#
docommand error117 "$SHELL -ci 'LC_ALL=C; . > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error118 "$SHELL -ci 'LC_ALL=C; : > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error119 "$SHELL -ci 'LC_ALL=C; break > /
echo OK'" O "OK\n" NONEMPTY
docommand error120 "$SHELL -ci 'LC_ALL=C; continue > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error121 "$SHELL -ci 'LC_ALL=C; eval echo > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error122 "$SHELL -ci 'LC_ALL=C; exec > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error123 "$SHELL -ci 'LC_ALL=C; exit 0 > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error124 "$SHELL -ci 'LC_ALL=C; export PATH > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error125 "$SHELL -ci 'LC_ALL=C; readonly PATH > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error126 "$SHELL -ci 'LC_ALL=C; return > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error127 "$SHELL -ci 'LC_ALL=C; set > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error128 "$SHELL -ci 'LC_ALL=C; set 1 2 3; shift > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error129 "$SHELL -ci 'LC_ALL=C; times > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error130 "$SHELL -ci 'LC_ALL=C; trap > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error131 "$SHELL -ci 'LC_ALL=C; unset LC_ALL > /
echo OK'" 0 "OK\n" NONEMPTY

docommand error132 "$SHELL -ci 'LC_ALL=C; echo > /
echo OK'" 0 "OK\n" NONEMPTY
docommand error133 "$SHELL -ci 'LC_ALL=C; readonly LC_ALL; LC_ALL=6
echo OK'" 0 "OK\n" NONEMPTY
docommand error134 "$SHELL -ci 'LC_ALL=C; : \"\${x!y}\"
echo OK'" 0 "OK\n" NONEMPTY

success
