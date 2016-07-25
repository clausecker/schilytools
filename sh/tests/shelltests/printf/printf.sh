#! /bin/sh
#
# @(#)printf.sh	1.2 16/07/24 Copyright 2016 J. Schilling
#

# Read printf core functions
. ../../common/test-common

#
# Basic printfs to check the printf builtin
#
docommand printf01 "$SHELL -c 'printf \"%s\n\" \"abc\"'" 0 "abc\n" ""
docommand printf02 "$SHELL -c 'printf \"%s\" \"abc\"'" 0 "abc" ""
docommand printf03 "$SHELL -c 'printf \"%s\n\" \"abc\" 123'" 0 "abc\n123\n" ""
docommand printf04 "$SHELL -c 'printf \"%.2s\" \"abc\"'" 0 "ab" ""

docommand printf10 "$SHELL -c 'printf \"%d\n\" 123'" 0 "123\n" ""
docommand printf11 "$SHELL -c 'printf \"%10d\n\" 123'" 0 "       123\n" ""
docommand printf12 "$SHELL -c 'printf \"%-10d\n\" 123'" 0 "123       \n" ""
docommand printf13 "$SHELL -c 'printf \"%d\n\" 0xff'" 0 "255\n" ""
docommand printf14 "$SHELL -c 'printf \"%i\n\" 0xff'" 0 "255\n" ""
docommand printf15 "$SHELL -c 'printf \"%u\n\" 0xff'" 0 "255\n" ""
docommand printf16 "$SHELL -c 'printf \"%x\n\" 0xff'" 0 "ff\n" ""
docommand printf17 "$SHELL -c 'printf \"%X\n\" 0xff'" 0 "FF\n" ""
docommand printf18 "$SHELL -c 'printf \"%o\n\" 0xff'" 0 "377\n" ""

docommand printf20 "$SHELL -c 'printf \"%*d\n\" 10 123'" 0 "       123\n" ""
docommand printf21 "$SHELL -c 'printf \"%*d\n\" -10 123'" 0 "123       \n" ""

#
# %b cannot be based on printf() as it needs to support nul bytes.
# So we need to check the fieldwidth and significance features again
#
docommand printf22 "$SHELL -c 'printf \"%10b\n\" 123'" 0 "       123\n" ""
docommand printf23 "$SHELL -c 'printf \"%-10b\n\" 123'" 0 "123       \n" ""
docommand printf24 "$SHELL -c 'printf \"%*b\n\" 10 123'" 0 "       123\n" ""
docommand printf25 "$SHELL -c 'printf \"%*b\n\" -10 123'" 0 "123       \n" ""
docommand printf26 "$SHELL -c 'printf \"%.3b\n\" 1234567890'" 0 "123\n" ""
docommand printf27 "$SHELL -c 'printf \"%.*b\n\" 3 1234567890'" 0 "123\n" ""

cat > x <<"XEOF"
printf '%b' 'abc'
XEOF
docommand printf110 "$SHELL ./x" 0 "abc" ""

cat > x <<"XEOF"
printf '%b' 'abc\cdef'
XEOF
docommand printf111 "$SHELL ./x" 0 "abc" ""

cat > x <<"XEOF"
printf '%b123' 'abc\cdef'
XEOF
docommand printf112 "$SHELL ./x" 0 "abc" ""

cat > x <<"XEOF"
printf '%b' 'abc\01'
XEOF
docommand printf113 "$SHELL ./x" 0 "abc\001" ""

cat > x <<"XEOF"
printf '%b' 'abc\0'
XEOF
docommand printf114 "$SHELL ./x" 0 "abc\000" ""

cat > x <<"XEOF"
printf '%b' 'abc\0def'
XEOF
docommand printf115 "$SHELL ./x" 0 "abc\000def" ""

remove x
success
