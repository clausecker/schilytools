#! /bin/sh
#
# @(#)printf.sh	1.9 18/09/01 Copyright 2016-2018 J. Schilling
#

# Read printf core functions
. ../../common/test-common

#
# Basic printfs to check the printf builtin
#
docommand printf01 "$SHELL -c 'printf \"%s\\\n\" \"abc\"'" 0 "abc\n" ""
docommand printf02 "$SHELL -c 'printf \"%s\" \"abc\"'" 0 "abc" ""
docommand printf03 "$SHELL -c 'printf \"%s\\\n\" \"abc\" 123'" 0 "abc\n123\n" ""
docommand printf04 "$SHELL -c 'printf \"%.2s\" \"abc\"'" 0 "ab" ""

docommand printf10 "$SHELL -c 'printf \"%d\\\n\" 123'" 0 "123\n" ""
docommand printf11 "$SHELL -c 'printf \"%10d\\\n\" 123'" 0 "       123\n" ""
docommand printf12 "$SHELL -c 'printf \"%-10d\\\n\" 123'" 0 "123       \n" ""
docommand printf13 "$SHELL -c 'printf \"%d\\\n\" 0xff'" 0 "255\n" ""
docommand printf14 "$SHELL -c 'printf \"%i\\\n\" 0xff'" 0 "255\n" ""
docommand printf15 "$SHELL -c 'printf \"%u\\\n\" 0xff'" 0 "255\n" ""
docommand printf16 "$SHELL -c 'printf \"%x\\\n\" 0xff'" 0 "ff\n" ""
docommand printf17 "$SHELL -c 'printf \"%X\\\n\" 0xff'" 0 "FF\n" ""
docommand printf18 "$SHELL -c 'printf \"%o\\\n\" 0xff'" 0 "377\n" ""

docommand printf20 "$SHELL -c 'printf \"%*d\\\n\" 10 123'" 0 "       123\n" ""
docommand printf21 "$SHELL -c 'printf \"%*d\\\n\" -10 123'" 0 "123       \n" ""

#
# %b cannot be based on printf() as it needs to support nul bytes.
# So we need to check the fieldwidth and significance features again
#
docommand printf22 "$SHELL -c 'printf \"%10b\\\n\" 123'" 0 "       123\n" ""
docommand printf23 "$SHELL -c 'printf \"%-10b\\\n\" 123'" 0 "123       \n" ""
docommand printf24 "$SHELL -c 'printf \"%*b\\\n\" 10 123'" 0 "       123\n" ""
docommand printf25 "$SHELL -c 'printf \"%*b\\\n\" -10 123'" 0 "123       \n" ""
docommand printf26 "$SHELL -c 'printf \"%.3b\\\n\" 1234567890'" 0 "123\n" ""
docommand printf27 "$SHELL -c 'printf \"%.*b\\\n\" 3 1234567890'" 0 "123\n" ""

#
# Check whether printf '\0123' behaves like the C-syntax and stops after
# the 3rd octal number even in case that the first number is a '0'
#
docommand printf30 "$SHELL -c 'printf \"\\1234\\\n\"'" 0 "S4\n" ""
docommand printf31 "$SHELL -c 'printf \"\\0123\\\n\"'" 0 "\n3\n" ""

docommand printf40 "$SHELL -c 'printf \"%d\\\n\" -1'" 0 "-1\n" ""
docommand printf41 "$SHELL -c 'printf \"%u\\\n\" -1'" 0 "18446744073709551615\n" ""
docommand printf42 "$SHELL -c 'printf \"%o\\\n\" -1'" 0 "1777777777777777777777\n" ""
docommand printf43 "$SHELL -c 'printf \"%x\\\n\" -1'" 0 "ffffffffffffffff\n" ""


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

cat > x <<"XEOF"
printf 'abc\0def'
XEOF
docommand printf116 "$SHELL ./x" 0 "abc\000def" ""

#
# Tests from Sven Maschek
#
docommand printf200 "$SHELL -c 'printf \"\\\a\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "007\n" ""
docommand printf201 "$SHELL -c 'printf \"\\\b\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "010\n" ""
docommand printf202 "$SHELL -c 'printf \"\\\t\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "011\n" ""
docommand printf203 "$SHELL -c 'printf \"\\\n\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "012\n" ""
docommand printf204 "$SHELL -c 'printf \"\\\v\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "013\n" ""
docommand printf205 "$SHELL -c 'printf \"\\\f\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "014\n" ""
docommand printf206 "$SHELL -c 'printf \"\\\r\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "015\n" ""

docommand printf207 "$SHELL -c 'printf \".\\\c.\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "056 134 143 056\n" ""
docommand printf208 "$SHELL -c 'printf \"%b\" \".\\\c.\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "056\n" ""

docommand printf209 "$SHELL -c 'printf \"\\\d\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 144\n" ""
docommand printf210 "$SHELL -c 'printf \"\\\e\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 145\n" ""
docommand printf211 "$SHELL -c 'printf \"\\\g\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 147\n" ""
docommand printf212 "$SHELL -c 'printf \"\\\h\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 150\n" ""
docommand printf213 "$SHELL -c 'printf \"\\\i\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 151\n" ""
docommand printf214 "$SHELL -c 'printf \"\\\j\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 152\n" ""
docommand printf215 "$SHELL -c 'printf \"\\\k\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 153\n" ""
docommand printf216 "$SHELL -c 'printf \"\\\l\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 154\n" ""
docommand printf217 "$SHELL -c 'printf \"\\\m\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 155\n" ""
docommand printf218 "$SHELL -c 'printf \"\\\o\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 157\n" ""
docommand printf219 "$SHELL -c 'printf \"\\\p\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 160\n" ""
docommand printf220 "$SHELL -c 'printf \"\\\q\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 161\n" ""
docommand printf221 "$SHELL -c 'printf \"\\\s\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 163\n" ""
docommand printf222 "$SHELL -c 'printf \"\\\u\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 165\n" ""
docommand printf223 "$SHELL -c 'printf \"\\\w\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 167\n" ""
docommand printf224 "$SHELL -c 'printf \"\\\x\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 170\n" ""
docommand printf225 "$SHELL -c 'printf \"\\\y\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 171\n" ""
docommand printf226 "$SHELL -c 'printf \"\\\z\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 172\n" ""


docommand printf227 "$SHELL -c 'printf \"%b\" \"\\\33\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "134 063 063\n" ""
docommand printf228 "$SHELL -c 'printf \"%b\" \"\\\033\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "033\n" ""
docommand printf229 "$SHELL -c 'printf \"%b\" \"\\\0033\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "033\n" ""

docommand printf230 "$SHELL -c 'printf \"\\\33\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "033\n" ""
docommand printf231 "$SHELL -c 'printf \"\\\033\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "033\n" ""
docommand printf232 "$SHELL -c 'printf \"\\\0033\" | od -b -A n|sed -e 2d -e \"s/^[ 	]*//\"'" 0 "003 063\n" ""

docommand printf233 "$SHELL -c 'printf \"%d\\\n\" \"\\\"a\"'" 0 "97\n" ""


#
# Tests for floating point enhancements.
#
expect_fail_save=$expect_fail
expect_fail=true
docommand -silent -esilent printf400 "$SHELL -c 'LC_ALL=C;export LC_ALL; printf \"%f\\\n\" 1.234567'" 0 "1.234567\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	echo
	echo "Test $cmd_label is a Solaris printf enhancement."
	echo "Skipping printf400..printf406."
	echo
else
docommand printf400 "$SHELL -c 'LC_ALL=C;export LC_ALL; printf \"%f\\\n\" 1.234567'" 0 "1.234567\n" ""
docommand printf401 "$SHELL -c 'LC_ALL=C;export LC_ALL; printf \"%F\\\n\" 1.234567'" 0 "1.234567\n" ""
docommand printf402 "$SHELL -c 'LC_ALL=C;export LC_ALL; printf \"%e\\\n\" 1.234567'" 0 "1.234567e+00\n" ""
docommand printf403 "$SHELL -c 'LC_ALL=C;export LC_ALL; printf \"%E\\\n\" 1.234567'" 0 "1.234567E+00\n" ""
docommand printf404 "$SHELL -c 'LC_ALL=C;export LC_ALL; printf \"%g\\\n\" 1.234567'" 0 "1.23457\n" ""
docommand printf405 "$SHELL -c 'LC_ALL=C;export LC_ALL; printf \"%G\\\n\" 1.234567'" 0 "1.23457\n" ""

docommand printf406 "$SHELL -c 'LC_ALL=C;export LC_ALL; printf \"%*.*f\\\n\" 6 3 1.234567'" 0 " 1.235\n" ""
fi

#
# Tests for the Solaris enhancements %n$
# First test %s only as the closed source Solaris /usr/bin/printf does
# not support %n$i.
#
expect_fail_save=$expect_fail
expect_fail=true
docommand -silent -esilent printf500 "$SHELL -c 'printf \"%2\\\$s %1\\\$s\\\n\" 1 2'" 0 "2 1\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	echo
	echo "Test $cmd_label is a Solaris printf enhancement."
	echo "Skipping printf500..printf501."
	echo
else
docommand printf500 "$SHELL -c 'printf \"%2\\\$s %1\\\$s\\\n\" 1 2'" 0 "2 1\n" ""

cat > x <<"XEOF"
printf '%3$*2$.*1$s\n' 3 10 abcdefghijk
XEOF
docommand printf501 "$SHELL ./x" 0 "       abc\n" ""
fi

expect_fail_save=$expect_fail
expect_fail=true
docommand -silent -esilent printf600 "$SHELL -c 'printf \"%2\\\$i %1\\\$i\\\n\" 1 2'" 0 "2 1\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	echo
	echo "Test $cmd_label is a Schily/ksh93 printf enhancement."
	echo "Skipping printf600..printf604."
	echo
else
docommand printf600 "$SHELL -c 'printf \"%2\\\$i %1\\\$i\\\n\" 1 2'" 0 "2 1\n" ""

cat > x <<"XEOF"
printf '%3$*2$.*1$i\n' 3 10 1234567
XEOF
docommand printf601 "$SHELL ./x" 0 "   1234567\n" ""

cat > x <<"XEOF"
printf '%3$*2$.*1$i\n' 3 10 1
XEOF
docommand printf602 "$SHELL ./x" 0 "       001\n" ""

cat > x <<"XEOF"
printf '%3$*2$.*1$i\n' 3 10 123
XEOF
docommand printf603 "$SHELL ./x" 0 "       123\n" ""

cat > x <<"XEOF"
printf '%3$*2$.*1$i\n' 3 10 123 2 15 456
XEOF
docommand printf604 "$SHELL ./x" 0 "       123\n            456\n" ""
fi

#exit

remove x
success
