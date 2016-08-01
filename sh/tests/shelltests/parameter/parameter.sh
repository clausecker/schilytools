#! /bin/sh
#
# @(#)parameter.sh	1.5 16/07/26 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

cmd="set . .; printf '%s ' \\\$*; printf '%s ' \\\$@; printf '%s ' \\\"\\\$*\\\"; printf '%s ' \\\"\\\$@\\\""
docommand ifs1 "$SHELL -c \"$cmd\"" 0 ". . . . . . . . " ""

docommand param1 "$SHELL -c 'parameter=param; echo \${parameter:-word}'" 0 "param\n" ""
docommand param2 "$SHELL -c 'parameter=\"\"; echo \${parameter:-word}'" 0 "word\n" ""
docommand param3 "$SHELL -c 'unset parameter; echo \${parameter:-word}'" 0 "word\n" ""

docommand param4 "$SHELL -c 'parameter=param; echo \${parameter-word}'" 0 "param\n" ""
docommand param5 "$SHELL -c 'parameter=\"\"; echo \${parameter-word}'" 0 "\n" ""
docommand param6 "$SHELL -c 'unset parameter; echo \${parameter-word}'" 0 "word\n" ""

docommand param7 "$SHELL -c 'parameter=param; echo \${parameter:=word}'" 0 "param\n" ""
docommand param8 "$SHELL -c 'parameter=\"\"; echo \${parameter:=word}'" 0 "word\n" ""
docommand param9 "$SHELL -c 'unset parameter; echo \${parameter:=word}'" 0 "word\n" ""

docommand param10 "$SHELL -c 'parameter=param; echo \${parameter=word}'" 0 "param\n" ""
docommand param11 "$SHELL -c 'parameter=\"\"; echo \${parameter=word}'" 0 "\n" ""
docommand param12 "$SHELL -c 'unset parameter; echo \${parameter=word}'" 0 "word\n" ""

docommand param13 "$SHELL -c 'parameter=param; echo \${parameter:?word}'" 0 "param\n" ""
docommand param14 "$SHELL -c 'parameter=\"\"; echo \${parameter:?word}'" "!=0" "" IGNORE
docommand param15 "$SHELL -c 'unset parameter; echo \${parameter:?word}'" "!=0" "" IGNORE

docommand param16 "$SHELL -c 'parameter=param; echo \${parameter?word}'" 0 "param\n" ""
docommand param17 "$SHELL -c 'parameter=\"\"; echo \${parameter?word}'" 0 "\n" ""
docommand param18 "$SHELL -c 'unset parameter; echo \${parameter?word}'" "!=0" "" IGNORE

docommand param19 "$SHELL -c 'parameter=param; echo \${parameter:+word}'" 0 "word\n" ""
docommand param20 "$SHELL -c 'parameter=\"\"; echo \${parameter:+word}'" 0 "\n" ""
docommand param21 "$SHELL -c 'unset parameter; echo \${parameter:+word}'" 0 "\n" ""

docommand param22 "$SHELL -c 'parameter=param; echo \${parameter+word}'" 0 "word\n" ""
docommand param23 "$SHELL -c 'parameter=\"\"; echo \${parameter+word}'" 0 "word\n" ""
docommand param24 "$SHELL -c 'unset parameter; echo \${parameter+word}'" 0 "\n" ""

docommand param25 "$SHELL -c 'echo \$#'" 0 "0\n" ""
docommand param26 "$SHELL -c 'echo \${#}'" 0 "0\n" ""
docommand param27 "$SHELL -c 'unset bla; echo \${#bla}'" 0 "0\n" ""
docommand param27 "$SHELL -c 'bla=ttt; echo \${#bla}'" 0 "3\n" ""
docommand param28 "$SHELL -c 'bla=ttt; echo \$#bla'" 0 "0bla\n" ""
docommand param29 "$SHELL -c 'bla=\"\"; echo \${#bla}'" 0 "0\n" ""

docommand param30 "$SHELL -c 'var=dlsfkjslfjalbla; echo \${var%*bla}'" 0 "dlsfkjslfjal\n" ""
docommand param31 "$SHELL -c 'var=dlsfkjslfjalbla; echo \${var%%*bla}'" 0 "\n" ""

docommand param32 "$SHELL -c 'var=bladlsfkblajslfjal; echo \${var#*bla}'" 0 "dlsfkblajslfjal\n" ""
docommand param33 "$SHELL -c 'var=bladlsfkblajslfjal; echo \${var##*bla}'" 0 "jslfjal\n" ""

docommand param34 "$SHELL -c 'var=/home/joerg; echo \${var%*rg}'" 0 "/home/joe\n" ""
docommand param35 "$SHELL -c 'var=/home/joerg; echo \${var%%*rg}'" 0 "\n" ""

docommand param36 "$SHELL -c 'var=/home/joerg; echo \${var#/h*}'" 0 "ome/joerg\n" ""
docommand param37 "$SHELL -c 'var=/home/joerg; echo \${var##/h*}'" 0 "\n" ""

docommand param38 "$SHELL -c 'echo \${#}'" 0 "0\n" ""
docommand param39 "$SHELL -c 'echo \${#:}'" "!=0" "" IGNORE
docommand param40 "$SHELL -c 'echo \${xxx:}'" 0 "\n" ""
docommand param41 "$SHELL -c 'echo \${xxx:a}'" "!=0" "" IGNORE
docommand param42 "$SHELL -c 'xxx=/home/joerg; echo \${xxx:}'" 0 "/home/joerg\n" ""

#
# Test from Robert Elz <kre@munnari.oz.au>
# $ set -- "abab*cbb" 
# $ echo "${1} ${1#a} ${1%b} ${1##ab} ${1%%b} ${1#*\*} ${1%\**}"
#
cmd='set -- "abab*cbb"; echo "${1} ${1#a}"'
docommand param43 "$SHELL -c '$cmd'" 0 "abab*cbb bab*cbb\n" ""
cmd='set -- "abab*cbb"; echo ${1} ${1#a}'
docommand param44 "$SHELL -c '$cmd'" 0 "abab*cbb bab*cbb\n" ""

cmd='set -- "abab*cbb"; echo "${1} ${1%b}"'
docommand param45 "$SHELL -c '$cmd'" 0 "abab*cbb abab*cb\n" ""
cmd='set -- "abab*cbb"; echo ${1} ${1%b}'
docommand param46 "$SHELL -c '$cmd'" 0 "abab*cbb abab*cb\n" ""

cmd='set -- "abab*cbb"; echo "${1} ${1##ab}"'
docommand param47 "$SHELL -c '$cmd'" 0 "abab*cbb ab*cbb\n" ""
cmd='set -- "abab*cbb"; echo ${1} ${1##ab}'
docommand param48 "$SHELL -c '$cmd'" 0 "abab*cbb ab*cbb\n" ""

cmd='set -- "abab*cbb"; echo "${1} ${1%%b}"'
docommand param49 "$SHELL -c '$cmd'" 0 "abab*cbb abab*cb\n" ""
cmd='set -- "abab*cbb"; echo ${1} ${1%%b}'
docommand param50 "$SHELL -c '$cmd'" 0 "abab*cbb abab*cb\n" ""

cmd='set -- "abab*cbb"; echo "${1} ${1#*\*}"'
docommand param51 "$SHELL -c '$cmd'" 0 "abab*cbb cbb\n" ""
cmd='set -- "abab*cbb"; echo ${1} ${1#*\*}'
docommand param52 "$SHELL -c '$cmd'" 0 "abab*cbb cbb\n" ""

cmd='set -- "abab*cbb"; echo "${1} ${1%\**}"'
docommand param53 "$SHELL -c '$cmd'" 0 "abab*cbb abab\n" ""
cmd='set -- "abab*cbb"; echo ${1} ${1%\**}'
docommand param54 "$SHELL -c '$cmd'" 0 "abab*cbb abab\n" ""


#
# Test that set -u does not cause "$@" to fail
#
docommand param100 "$SHELL -cu 'echo \"\$@\"'" 0 "\n" ""

success
