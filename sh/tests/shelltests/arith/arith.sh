#! /bin/sh
#
# @(#)arith.sh	1.19 16/06/07 Copyright 2016 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether the supported operators work
#
expect_fail_save=$expect_fail
expect_fail=true
#
# Unspecified by POSIX
#
docommand a00 "$SHELL -c 'echo \$(())'" 0 "0\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	echo
	echo "Test $cmd_label is unspecified behavior by POSIX."
	echo
fi

docommand a01 "$SHELL -c 'echo \$((/))'" "!=0" "" IGNORE

docommand a02 "$SHELL -c 'echo \$((1))'" 0 "1\n" ""
docommand a03 "$SHELL -c 'echo \$((1+2))'" 0 "3\n" ""
docommand a04 "$SHELL -c 'echo \$((1-2))'" 0 "-1\n" ""
docommand a05 "$SHELL -c 'echo \$((2*3))'" 0 "6\n" ""
docommand a06 "$SHELL -c 'echo \$((2*-3))'" 0 "-6\n" ""
docommand a07 "$SHELL -c 'echo \$((10/2))'" 0 "5\n" ""

docommand a08 "$SHELL -c 'echo \$((18%3))'" 0 "0\n" ""
docommand a09 "$SHELL -c 'echo \$((17%3))'" 0 "2\n" ""
docommand a10 "$SHELL -c 'echo \$((-17%3))'" 0 "-2\n" ""

docommand a11 "$SHELL -c 'echo \$((1 << 1))'" 0 "2\n" ""
expect_fail_save=$expect_fail
expect_fail=true
docommand a12 "$SHELL -c 'echo \$((1 << 31))'" 0 "2147483648\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	have_longlong=false
	echo
	echo "Test $cmd_label is unspecified behavior by POSIX."
	echo "Test $cmd_label needs more than 'signed long'."
	echo "Skipping long long tests."
	echo
else
	have_longlong=true
fi
if [ $have_longlong = true ]; then
docommand a13 "$SHELL -c 'echo \$((1 << 33))'" 0 "8589934592\n" ""
fi
docommand a14 "$SHELL -c 'echo \$((2 >> 1))'" 0 "1\n" ""
docommand a15 "$SHELL -c 'echo \$((64 >> 5))'" 0 "2\n" ""
if [ $have_longlong = true ]; then
docommand a16 "$SHELL -c 'echo \$((2147483648 >> 31))'" 0 "1\n" ""
docommand a17 "$SHELL -c 'echo \$((8589934592 >> 33))'" 0 "1\n" ""
fi

docommand a18 "$SHELL -c 'echo \$((5 > 4))'" 0 "1\n" ""
docommand a19 "$SHELL -c 'echo \$((4 > 4))'" 0 "0\n" ""
docommand a20 "$SHELL -c 'echo \$((3 < 4))'" 0 "1\n" ""
docommand a21 "$SHELL -c 'echo \$((4 < 4))'" 0 "0\n" ""
docommand a22 "$SHELL -c 'echo \$((4 <= 4))'" 0 "1\n" ""
docommand a23 "$SHELL -c 'echo \$((5 <= 4))'" 0 "0\n" ""
docommand a24 "$SHELL -c 'echo \$((4 >= 4))'" 0 "1\n" ""
docommand a25 "$SHELL -c 'echo \$((3 >= 4))'" 0 "0\n" ""
docommand a26 "$SHELL -c 'echo \$((4 == 4))'" 0 "1\n" ""
docommand a27 "$SHELL -c 'echo \$((3 == 4))'" 0 "0\n" ""
docommand a28 "$SHELL -c 'echo \$((3 != 4))'" 0 "1\n" ""
docommand a29 "$SHELL -c 'echo \$((4 != 4))'" 0 "0\n" ""

docommand a30 "$SHELL -c 'echo \$((123 & 7))'" 0 "3\n" ""
docommand a31 "$SHELL -c 'echo \$((123 ^ 7))'" 0 "124\n" ""
docommand a32 "$SHELL -c 'echo \$((123 | 7))'" 0 "127\n" ""

docommand a33 "$SHELL -c 'echo \$((1 && 2))'" 0 "1\n" ""
docommand a34 "$SHELL -c 'echo \$((1 && 0))'" 0 "0\n" ""
docommand a35 "$SHELL -c 'echo \$((0 && 2))'" 0 "0\n" ""
docommand a36 "$SHELL -c 'echo \$((0 && 0))'" 0 "0\n" ""

docommand a37 "$SHELL -c 'echo \$((1 || 2))'" 0 "1\n" ""
docommand a38 "$SHELL -c 'echo \$((1 || 0))'" 0 "1\n" ""
docommand a39 "$SHELL -c 'echo \$((0 || 2))'" 0 "1\n" ""
docommand a40 "$SHELL -c 'echo \$((0 || 0))'" 0 "0\n" ""

docommand a41 "$SHELL -c 'echo \$((+1))'" 0 "1\n" ""
docommand a42 "$SHELL -c 'echo \$((+ 1))'" 0 "1\n" ""
docommand a43 "$SHELL -c 'echo \$((-1))'" 0 "-1\n" ""
docommand a44 "$SHELL -c 'echo \$((- 1))'" 0 "-1\n" ""

docommand a45 "$SHELL -c 'echo \$((~1))'" 0 "-2\n" ""
docommand a46 "$SHELL -c 'echo \$((~63))'" 0 "-64\n" ""
docommand a47 "$SHELL -c 'echo \$((~1023))'" 0 "-1024\n" ""

docommand a48 "$SHELL -c 'echo \$((!0))'" 0 "1\n" ""
docommand a49 "$SHELL -c 'echo \$((!1))'" 0 "0\n" ""
docommand a50 "$SHELL -c 'echo \$((!2))'" 0 "0\n" ""
docommand a51 "$SHELL -c 'echo \$((!!999))'" 0 "1\n" ""

docommand a52 "$SHELL -c 'echo \$((+-~!1))'" 0 "1\n" ""
docommand a53 "$SHELL -c 'echo \$((+-~!(1)))'" 0 "1\n" ""

docommand a54 "$SHELL -c 'unset a; echo \$((a))'" 0 "0\n" ""

expect_fail_save=$expect_fail
expect_fail=true
docommand a55 "$SHELL -c 'unset a; echo \$((a++))'" 0 "0\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	var_incr=false
	echo
	echo "The shell $SHELL does not support variable increment like a++."
	echo "This is not required by POSIX."
	echo
else
	var_incr=true
	docommand a56 "$SHELL -c 'unset a; echo \$((a--))'" 0 "0\n" ""
	docommand a57 "$SHELL -c 'unset a; echo \$((++a))'" 0 "1\n" ""
	docommand a58 "$SHELL -c 'unset a; echo \$((--a))'" 0 "-1\n" ""

	docommand a59 "$SHELL -c 'unset a; echo \$((a++)); echo \$a'" 0 "0\n1\n" ""
	docommand a60 "$SHELL -c 'unset a; echo \$((a--)); echo \$a'" 0 "0\n-1\n" ""
	docommand a61 "$SHELL -c 'unset a; echo \$((++a)); echo \$a'" 0 "1\n1\n" ""
	docommand a62 "$SHELL -c 'unset a; echo \$((--a)); echo \$a'" 0 "-1\n-1\n" ""

	docommand a63 "$SHELL -c 'unset a; echo \$((++a++)); echo \$a'" "!=0" "" IGNORE
	docommand a64 "$SHELL -c 'unset a; echo \$((--a--)); echo \$a'" "!=0" "" IGNORE

	docommand a56 "$SHELL -c 'echo \$((\$((A=2))++--++--++--A))'" "!=0" "" IGNORE
	docommand a57 "$SHELL -c 'echo \$((++--++--++--A))'" "!=0" "" IGNORE
fi
docommand a58 "$SHELL -c 'echo \$((
1+2+3))'" 0 "6\n" ""
docommand a59 "$SHELL -c 'echo \$((1
+2+3))'" 0 "6\n" ""
docommand a60 "$SHELL -c 'echo \$((1+
2+3))'" 0 "6\n" ""
docommand a61 "$SHELL -c 'echo \$((1+2
+3))'" 0 "6\n" ""
docommand a62 "$SHELL -c 'echo \$((1+2+
3))'" 0 "6\n" ""
docommand a63 "$SHELL -c 'echo \$((1+2+3
))'" 0 "6\n" ""
docommand a64 "$SHELL -c 'echo \$((1+
(2+3)))'" 0 "6\n" ""
docommand a65 "$SHELL -c 'echo \$((1+(
2+3)))'" 0 "6\n" ""
docommand a66 "$SHELL -c 'echo \$((1+(2
+3)))'" 0 "6\n" ""
docommand a67 "$SHELL -c 'echo \$((1+(2+3
)))'" 0 "6\n" ""
docommand a68 "$SHELL -c 'echo \$((1+(2+3)
))'" 0 "6\n" ""

#
# Tests for number base
#
docommand a100 "$SHELL -c 'echo \$((0x1))'" 0 "1\n" ""
docommand a101 "$SHELL -c 'echo \$((0x01))'" 0 "1\n" ""
docommand a102 "$SHELL -c 'echo \$((01))'" 0 "1\n" ""
docommand a103 "$SHELL -c 'echo \$((0x10))'" 0 "16\n" ""

expect_fail_save=$expect_fail
expect_fail=true
docommand a104 "$SHELL -c 'echo \$((0100))'" 0 "64\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	octal_numbers=false
	echo
	echo "The shell $SHELL does not correctly convert octal numbers."
	echo "This is required by POSIX, so this shell is not POSIX compliant."
	echo "Skipping more octal tests."
	echo
else
	octal_numbers=true
fi
if [ $octal_numbers = true ] ; then
docommand a105 "$SHELL -c 'echo \$((010+10))'" 0 "18\n" ""
expect_fail_save=$expect_fail
expect_fail=true
docommand a106 "$SHELL -c 'echo \$((019))'" "!=0" "" IGNORE
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	echo
	echo "The shell $SHELL does not correctly flag illegal octal numbers."
	echo "This is required by POSIX, so this shell is not POSIX compliant."
	echo
fi
fi

#
# Tests for precedence and chained operators
#
docommand a120 "$SHELL -c 'echo \$((1+2*3))'" 0 "7\n" ""
docommand a121 "$SHELL -c 'echo \$((1+(2*3)))'" 0 "7\n" ""
docommand a122 "$SHELL -c 'echo \$((2*5+2*3))'" 0 "16\n" ""
docommand a123 "$SHELL -c 'echo \$((2*5+2*3+1))'" 0 "17\n" ""
docommand a124 "$SHELL -c 'echo \$((2*5+2*3+1+2+3))'" 0 "22\n" ""
docommand a125 "$SHELL -c 'echo \$(((1)+(2)))'" 0 "3\n" ""
docommand a126 "$SHELL -c 'echo \$(((2/3)!=(5/3)==(3/3)))'" 0 "1\n" ""
docommand a127 "$SHELL -c 'echo \$((0!=1==1))'" 0 "1\n" ""
docommand a128 "$SHELL -c 'echo \$((0!=3==3))'" 0 "0\n" ""
docommand a129 "$SHELL -c 'echo \$((2*5+2*3*2+1))'" 0 "23\n" ""
docommand a130 "$SHELL -c 'echo \$((20 / 2 / 2))'" 0 "5\n" ""

#
# Tests for the assignment operators
#
docommand a150 "$SHELL -c 'unset a; echo \$((a += 3))'" 0 "3\n" ""
docommand a151 "$SHELL -c 'a=0; echo \$((a += 3))'" 0 "3\n" ""
docommand a152 "$SHELL -c 'a=0; echo \$((a -= 3))'" 0 "-3\n" ""
docommand a153 "$SHELL -c 'a=2; echo \$((a *= 3))'" 0 "6\n" ""
docommand a154 "$SHELL -c 'a=2; echo \$((a /= 2))'" 0 "1\n" ""
docommand a155 "$SHELL -c 'a=17; echo \$((a %= 5))'" 0 "2\n" ""
docommand a156 "$SHELL -c 'a=1; echo \$((a <<= 3))'" 0 "8\n" ""
docommand a157 "$SHELL -c 'a=8; echo \$((a >>= 3))'" 0 "1\n" ""
docommand a158 "$SHELL -c 'a=123; echo \$((a &= 7))'" 0 "3\n" ""
docommand a159 "$SHELL -c 'a=123; echo \$((a ^= 7))'" 0 "124\n" ""
docommand a160 "$SHELL -c 'a=123; echo \$((a |= 7))'" 0 "127\n" ""
docommand a161 "$SHELL -c 'a=0;b=9; echo \$((a = b = 2)); echo \$a; echo \$b'" 0 "2\n2\n2\n" ""
docommand a162 "$SHELL -c 'a=0;b=9;c=6; echo \$((a = b = c = 2)); echo \$a; echo \$b; echo \$c'" 0 "2\n2\n2\n2\n" ""
docommand a162 "$SHELL -c 'i=1 j=2 k=3; echo \$((i += j += k)); echo \$i,\$j,\$k'" 0 "6\n6,5,3\n" ""

#
# Tests for command expansion inside arithmetic expansion
#
docommand a180 "$SHELL -c 'echo \$(($(true)==$(true)))'" "!=0" "" IGNORE
docommand a181 "$SHELL -c 'echo \$((`true`==`true`))'" "!=0" "" IGNORE
docommand a182 "$SHELL -c 'echo \$(($(echo 1)==$(echo 1)))'" 0 "1\n" ""
docommand a183 "$SHELL -c 'echo \$((`echo 1`==`echo 1`))'" 0 "1\n" ""

#
# Tests to check whether 0 && x or 1 || x correctly do not
# evaluate x
#
docommand a200 "$SHELL -c 'echo \$((0 && 1))'" 0 "0\n" ""
docommand a201 "$SHELL -c 'echo \$((0 && 1/0))'" 0 "0\n" ""
docommand a202 "$SHELL -c 'echo \$((0 && (1/0)))'" 0 "0\n" ""
docommand a203 "$SHELL -c 'echo \$((1 && (1/0)))'" "!=0" "" IGNORE

docommand a204 "$SHELL -c 'echo \$((1 || 1))'" 0 "1\n" ""
docommand a205 "$SHELL -c 'echo \$((1 || 1/0))'" 0 "1\n" ""
docommand a206 "$SHELL -c 'echo \$((1 || (1/0)))'" 0 "1\n" ""
docommand a207 "$SHELL -c 'echo \$((0 || (1/0)))'" "!=0" "" IGNORE

if [ "$var_incr" = true ]; then
docommand a208 "$SHELL -c 'unset a; echo \$((1 && a++)); echo \$a'" 0 "0\n1\n" ""
docommand a209 "$SHELL -c 'unset a; echo \$((0 && a++)); echo \$a'" 0 "0\n\n" ""
docommand a210 "$SHELL -c 'unset a; echo \$((0 && (a++))); echo \$a'" 0 "0\n\n" ""

docommand a211 "$SHELL -c 'unset a; echo \$((0 || a++)); echo \$a'" 0 "0\n1\n" ""
docommand a212 "$SHELL -c 'unset a; echo \$((1 || a++)); echo \$a'" 0 "1\n\n" ""
docommand a213 "$SHELL -c 'unset a; echo \$((1 || (a++))); echo \$a'" 0 "1\n\n" ""

docommand a214 "$SHELL -c 'unset a; echo \$((1 && --a)); echo \$a'" 0 "1\n-1\n" ""
docommand a215 "$SHELL -c 'unset a; echo \$((0 && --a)); echo \$a'" 0 "0\n\n" ""
docommand a216 "$SHELL -c 'unset a; echo \$((0 && (--a))); echo \$a'" 0 "0\n\n" ""

docommand a217 "$SHELL -c 'unset a; echo \$((0 || --a)); echo \$a'" 0 "1\n-1\n" ""
docommand a218 "$SHELL -c 'unset a; echo \$((1 || --a)); echo \$a'" 0 "1\n\n" ""
docommand a219 "$SHELL -c 'unset a; echo \$((1 || (--a))); echo \$a'" 0 "1\n\n" ""
else
	echo
	echo "Skipping more variable pre/post increment/decrement tests..."
	echo
fi

#
# Tests for conditional expressions
#
docommand a250 "$SHELL -c 'echo \$((1 ? 15 : 12))'" 0 "15\n" ""
docommand a251 "$SHELL -c 'echo \$((0 ? 15 : 12))'" 0 "12\n" ""
docommand a252 "$SHELL -c 'echo \$((1 ? (15) : (12)))'" 0 "15\n" ""
docommand a253 "$SHELL -c 'echo \$((0 ? (15) : (12)))'" 0 "12\n" ""
docommand a254 "$SHELL -c 'echo \$((1 ? 1+2+3 : 4+5+6))'" 0 "6\n" ""
docommand a255 "$SHELL -c 'echo \$((0 ? 1+2+3 : 4+5+6))'" 0 "15\n" ""
docommand a256 "$SHELL -c 'echo \$((1 ? (1+2+3) : (4+5+6)))'" 0 "6\n" ""
docommand a257 "$SHELL -c 'echo \$((0 ? (1+2+3) : (4+5+6)))'" 0 "15\n" ""

if [ "$var_incr" = true ]; then
docommand a258 "$SHELL -c 'unset a; unset b; echo \$((1 ? a++ : b--)); echo \$a; echo \$b'" 0 "0\n1\n\n" ""
docommand a259 "$SHELL -c 'unset a; unset b; echo \$((0 ? a++ : b--)); echo \$a; echo \$b'" 0 "0\n\n-1\n" ""
docommand a260 "$SHELL -c 'unset a; b=-1; echo \$((0 ? a++ : b--)); echo \$a; echo \$b'" 0 "-1\n\n-2\n" ""
docommand a261 "$SHELL -c 'unset a; unset b; echo \$((1 ? (a++) : (b--))); echo \$a; echo \$b'" 0 "0\n1\n\n" ""
docommand a262 "$SHELL -c 'unset a; unset b; echo \$((0 ? (a++) : (b--))); echo \$a; echo \$b'" 0 "0\n\n-1\n" ""
docommand a263 "$SHELL -c 'unset a; b=-1; echo \$((0 ? (a++) : (b--))); echo \$a; echo \$b'" 0 "-1\n\n-2\n" ""
fi

docommand a264 "$SHELL -c 'echo \$((0?(9+4<1):12))'" 0 "12\n" ""
docommand a265 "$SHELL -c 'echo \$((1?(9+4<1):12))'" 0 "0\n" ""
docommand a266 "$SHELL -c 'echo \$((0?9+4<1:12))'" 0 "12\n" ""
docommand a267 "$SHELL -c 'echo \$((1?9+4<1:12))'" 0 "0\n" ""
docommand a268 "$SHELL -c 'echo \$((0?A=9+4<1:12))'" 0 "12\n" ""
docommand a269 "$SHELL -c 'echo \$((1?A=9+4<1:12))'" 0 "0\n" ""

docommand a270 "$SHELL -c 'echo \$((!!(-02<=-04)^1 ?+9+(+4):+3+(+12)))'" 0 "13\n" ""
docommand a271 "$SHELL -c 'echo \$((!(-02<=-04)^1 ?+9+(+4):+3+(+12)))'" 0 "15\n" ""
docommand a272 "$SHELL -c 'echo \$((!!(-02<=-04)^1 ?+9+(+4) <<1:+3+(+12)<<2))'" 0 "26\n" ""
docommand a273 "$SHELL -c 'echo \$((!(-02<=-04)^1 ?+9+(+4) <<1:+3+(+12)<<2))'" 0 "60\n" ""
docommand a274 "$SHELL -c 'echo \$((!(!(-02<=-04))^1 ?+9+(+4) <<1:+3+(+12)<<2))'" 0 "26\n" ""
docommand a275 "$SHELL -c 'echo \$(((!(-02<=-04))^1 ?+9+(+4) <<1:+3+(+12)<<2))'" 0 "60\n" ""

docommand a276 "$SHELL -c 'echo \$((1?0?-2:2:3))'" 0 "2\n" ""
docommand a277 "$SHELL -c 'echo \$((0?2?-2:2:3))'" 0 "3\n" ""
docommand a278 "$SHELL -c 'echo \$((1?2?-2:2:3))'" 0 "-2\n" ""
docommand a279 "$SHELL -c 'echo \$((0?0?-2:2:3))'" 0 "3\n" ""

docommand a280 "$SHELL -c 'echo \$((1?3:0?-2:2))'" 0 "3\n" ""
docommand a281 "$SHELL -c 'echo \$((0?3:2?-2:2))'" 0 "-2\n" ""
docommand a282 "$SHELL -c 'echo \$((1?3:2?-2:2))'" 0 "3\n" ""
docommand a283 "$SHELL -c 'echo \$((0?3:0?-2:2))'" 0 "2\n" ""

docommand a284 "$SHELL -c 'echo \$((1?(0?-2:2):3))'" 0 "2\n" ""
docommand a285 "$SHELL -c 'echo \$((0?(2?-2:2):3))'" 0 "3\n" ""
docommand a286 "$SHELL -c 'echo \$((1?(2?-2:2):3))'" 0 "-2\n" ""
docommand a287 "$SHELL -c 'echo \$((0?(0?-2:2):3))'" 0 "3\n" ""

docommand a288 "$SHELL -c 'echo \$((1?3:(0?-2:2)))'" 0 "3\n" ""
docommand a289 "$SHELL -c 'echo \$((0?3:(2?-2:2)))'" 0 "-2\n" ""
docommand a290 "$SHELL -c 'echo \$((1?3:(2?-2:2)))'" 0 "3\n" ""
docommand a291 "$SHELL -c 'echo \$((0?3:(0?-2:2)))'" 0 "2\n" ""

docommand a292 "$SHELL -c 'echo $((1?0^0:1?-2:3))'" 0 "0\n" ""
docommand a293 "$SHELL -c 'echo $((0?0^0:1?-2:3))'" 0 "-2\n" ""
docommand a294 "$SHELL -c 'echo $((1?0^0:0?-2:3))'" 0 "0\n" ""
docommand a295 "$SHELL -c 'echo $((0?0^0:0?-2:3))'" 0 "3\n" ""

if [ "$var_incr" = true ]; then
docommand a300 "$SHELL -c 'echo \$((QQ_*=XCd<<=Y++^O++?Y>>=H++:T++!=C++))'" 0 "0\n" ""
docommand a301 "$SHELL -c 'echo \$((YHz-=M++|Z++<<X++>N++==(M|=P_+=L+++T++)))'" 0 "0\n" ""
#
# The next may fail nor not, C does not permit it as a parenthesis is missing.
# So we do not run it.
#
#docommand a301b "$SHELL -c 'echo \$((YHz-=M++|Z++<<X++>N++==M|=P_+=L+++T++))'" "!=0" "" IGNORE
else
	echo
	echo "Skipping more variable pre/post increment/decrement tests..."
	echo
fi

docommand a302 "$SHELL -c 'echo \$((N>>=U+=F&=J!=(H*=S-P<=K<<I-X!=G<<V>=(Z|=R<I<<O))))'" 0 "0\n" ""
#
# The next may fail nor not, C does not permit it as a parenthesis is missing.
# So we do not run it.
#
#docommand a302b "$SHELL -c 'echo \$((N>>=U+=F&=J!=H*=S-P<=K<<I-X!=G<<V>=Z|=R<I<<O))'" "!=0" "" IGNORE

#
# lvalue and division by zero - so this must fail
#
docommand a310 "$SHELL -c 'echo \$((F-=B>>=D|D<=E*=Q|S!=W!=R|B&=M+O?Y?A:Q<G>>P:M*=N%=Q>>Q))'" "!=0" "" IGNORE
docommand a312 "$SHELL -c 'echo \$((t%=r&=U-O*U+=J))'" "!=0" "" IGNORE

#
# Tests for the comma operator
#
expect_fail_save=$expect_fail
expect_fail=true
docommand a400 "$SHELL -c 'echo \$((1 , 15))'" 0 "15\n" ""
expect_fail=$expect_fail_save
if [ "$failed" = true ]; then
	comma_operator=false
	echo
	echo "Test $cmd_label checks requires POSIX syntax (comma operator)."
	echo
else
	comma_operator=true
fi
if [ "$comma_operator" = true ]; then
docommand a401 "$SHELL -c 'echo \$((a=2 , 15)); echo \$a'" 0 "15\n2\n" ""
docommand a402 "$SHELL -c 'echo \$((a=2*3 , 15)); echo \$a'" 0 "15\n6\n" ""
docommand a403 "$SHELL -c 'echo \$((a=2*3*4 , 15)); echo \$a'" 0 "15\n24\n" ""
docommand a404 "$SHELL -c 'echo \$(((1) , (15)))'" 0 "15\n" ""
docommand a405 "$SHELL -c 'echo \$(((a=2) , (15))); echo \$a'" 0 "15\n2\n" ""
docommand a406 "$SHELL -c 'echo \$(((a=2*3) , (15))); echo \$a'" 0 "15\n6\n" ""
docommand a407 "$SHELL -c 'echo \$((a=(2*3) , (15))); echo \$a'" 0 "15\n6\n" ""
docommand a408 "$SHELL -c 'echo \$(((a=2*3*4) , (15))); echo \$a'" 0 "15\n24\n" ""
docommand a409 "$SHELL -c 'unset a; echo \$((a++ , 15)); echo \$a'" 0 "15\n1\n" ""
docommand a410 "$SHELL -c 'unset a; echo \$((a++ , ++a)); echo \$a'" 0 "2\n2\n" ""
docommand a411 "$SHELL -c 'unset a; echo \$((a++ , a++)); echo \$a'" 0 "1\n2\n" ""
else
	echo
	echo "Skipping comma operator tests."
	echo
fi

#
# Test cases that are similar to the tests from ksh93
#
docommand ak01a "$SHELL -c 'x=1 y=2 z=3; echo \$((2+2))'" 0 "4\n" ""
docommand ak01b "$SHELL -c 'x=1 y=2 z=3; echo \$((2+2 != 4))'" 0 "0\n" ""
docommand ak02a "$SHELL -c 'x=1 y=2 z=3; echo \$((x+y))'" 0 "3\n" ""
docommand ak02b "$SHELL -c 'x=1 y=2 z=3; echo \$((x+y!=z))'" 0 "0\n" ""
docommand ak03a "$SHELL -c 'x=1 y=2 z=3; echo \$((\$x+\$y))'" 0 "3\n" ""
docommand ak03b "$SHELL -c 'x=1 y=2 z=3; echo \$((\$x+\$y!=\$z))'" 0 "0\n" ""
docommand ak04a "$SHELL -c 'x=1 y=2 z=3; echo \$(((x|y)))'" 0 "3\n" ""
docommand ak04b "$SHELL -c 'x=1 y=2 z=3; echo \$(((x|y)!=z))'" 0 "0\n" ""
docommand ak05  "$SHELL -c 'x=1 y=2 z=3; echo \$((x >= z))'" 0 "0\n" ""
docommand ak06  "$SHELL -c 'x=1 y=2 z=3; echo \$((y+3 != z+2))'" 0 "0\n" ""
docommand ak07  "$SHELL -c 'x=1 y=2 z=3; echo \$((y<<2 != 1<<3))'" 0 "0\n" ""
docommand ak08  "$SHELL -c 'x=1 y=2 z=3; echo \$((133%10 != 3))'" 0 "0\n" ""
docommand ak09  "$SHELL -c 'd=0; echo \$((d || 1))'" 0 "1\n" ""
if [ "$var_incr" = true ]; then
docommand ak10  "$SHELL -c 'd=0; echo \$((d++ != 0))'" 0 "0\n" ""
docommand ak11  "$SHELL -c 'd=1; echo \$((--d != 0))'" 0 "0\n" ""
fi
if [ "$comma_operator" = true ]; then
docommand ak12  "$SHELL -c 'd=0; echo \$(( (d++,6)!=6 && d!=1))'" 0 "0\n" ""
fi
if [ "$var_incr" = true ]; then
docommand ak13  "$SHELL -c 'd=0; echo \$(( (1?2+1:3*4+d++)!=3 || d!=0))'" 0 "0\n" ""
fi
docommand ak14  "$SHELL -c 'i=1; echo \$(( (i?0:1)))'" 0 "0\n" ""
docommand ak15  "$SHELL -c 'i=1; echo \$(( (1 || 1 && 0) != 1))'" 0 "0\n" ""
docommand ak16  "$SHELL -c 'x=1; echo \$(( (x=-x) != -1 ))'" 0 "0\n" ""
docommand ak17  "$SHELL -c 'x=2; echo \$(( 1\$((\$x))3 != 123 ))'" 0 "0\n" ""


success
