#! /bin/sh

# Read test core functions
. ../../common/test-common

#
# Basic tests to check whether the supported operators work
#
docommand a00 "$SHELL -c 'echo $(())'" 0 "0\n" ""
docommand a01 "$SHELL -c 'echo $((/))'" "!=0" "" IGNORE

docommand a02 "$SHELL -c 'echo $((1))'" 0 "1\n" ""
docommand a03 "$SHELL -c 'echo $((1+2))'" 0 "3\n" ""
docommand a04 "$SHELL -c 'echo $((1-2))'" 0 "-1\n" ""
docommand a05 "$SHELL -c 'echo $((2*3))'" 0 "6\n" ""
docommand a06 "$SHELL -c 'echo $((2*-3))'" 0 "-6\n" ""
docommand a07 "$SHELL -c 'echo $((10/2))'" 0 "5\n" ""

docommand a08 "$SHELL -c 'echo $((18%3))'" 0 "0\n" ""
docommand a09 "$SHELL -c 'echo $((17%3))'" 0 "2\n" ""
docommand a10 "$SHELL -c 'echo $((-17%3))'" 0 "-2\n" ""

docommand a11 "$SHELL -c 'echo $((1 << 1))'" 0 "2\n" ""
docommand a12 "$SHELL -c 'echo $((1 << 31))'" 0 "2147483648\n" ""
docommand a13 "$SHELL -c 'echo $((1 << 33))'" 0 "8589934592\n" ""
docommand a14 "$SHELL -c 'echo $((2 >> 1))'" 0 "1\n" ""
docommand a15 "$SHELL -c 'echo $((64 >> 5))'" 0 "2\n" ""
docommand a16 "$SHELL -c 'echo $((2147483648 >> 31))'" 0 "1\n" ""
docommand a17 "$SHELL -c 'echo $((8589934592 >> 33))'" 0 "1\n" ""

docommand a18 "$SHELL -c 'echo $((5 > 4))'" 0 "1\n" ""
docommand a19 "$SHELL -c 'echo $((4 > 4))'" 0 "0\n" ""
docommand a20 "$SHELL -c 'echo $((3 < 4))'" 0 "1\n" ""
docommand a21 "$SHELL -c 'echo $((4 < 4))'" 0 "0\n" ""
docommand a22 "$SHELL -c 'echo $((4 <= 4))'" 0 "1\n" ""
docommand a23 "$SHELL -c 'echo $((5 <= 4))'" 0 "0\n" ""
docommand a24 "$SHELL -c 'echo $((4 >= 4))'" 0 "1\n" ""
docommand a25 "$SHELL -c 'echo $((3 >= 4))'" 0 "0\n" ""
docommand a26 "$SHELL -c 'echo $((4 == 4))'" 0 "1\n" ""
docommand a27 "$SHELL -c 'echo $((3 == 4))'" 0 "0\n" ""
docommand a28 "$SHELL -c 'echo $((3 != 4))'" 0 "1\n" ""
docommand a29 "$SHELL -c 'echo $((4 != 4))'" 0 "0\n" ""

docommand a30 "$SHELL -c 'echo $((123 & 7))'" 0 "3\n" ""
docommand a31 "$SHELL -c 'echo $((123 ^ 7))'" 0 "124\n" ""
docommand a32 "$SHELL -c 'echo $((123 | 7))'" 0 "127\n" ""

docommand a33 "$SHELL -c 'echo $((1 && 2))'" 0 "1\n" ""
docommand a34 "$SHELL -c 'echo $((1 && 0))'" 0 "0\n" ""
docommand a35 "$SHELL -c 'echo $((0 && 2))'" 0 "0\n" ""
docommand a36 "$SHELL -c 'echo $((0 && 0))'" 0 "0\n" ""

docommand a37 "$SHELL -c 'echo $((1 || 2))'" 0 "1\n" ""
docommand a38 "$SHELL -c 'echo $((1 || 0))'" 0 "1\n" ""
docommand a39 "$SHELL -c 'echo $((0 || 2))'" 0 "1\n" ""
docommand a40 "$SHELL -c 'echo $((0 || 0))'" 0 "0\n" ""

docommand a41 "$SHELL -c 'echo $((+1))'" 0 "1\n" ""
docommand a42 "$SHELL -c 'echo $((+ 1))'" 0 "1\n" ""
docommand a43 "$SHELL -c 'echo $((-1))'" 0 "-1\n" ""
docommand a44 "$SHELL -c 'echo $((- 1))'" 0 "-1\n" ""

docommand a45 "$SHELL -c 'echo $((~1))'" 0 "-2\n" ""
docommand a46 "$SHELL -c 'echo $((~63))'" 0 "-64\n" ""
docommand a47 "$SHELL -c 'echo $((~1023))'" 0 "-1024\n" ""

docommand a48 "$SHELL -c 'echo $((!0))'" 0 "1\n" ""
docommand a49 "$SHELL -c 'echo $((!1))'" 0 "0\n" ""
docommand a50 "$SHELL -c 'echo $((!2))'" 0 "0\n" ""
docommand a51 "$SHELL -c 'echo $((!!999))'" 0 "1\n" ""

docommand a52 "$SHELL -c 'echo $((+-~!1))'" 0 "1\n" ""
docommand a53 "$SHELL -c 'echo $((+-~!(1)))'" 0 "1\n" ""

#
# Tests for number base
#
docommand a100 "$SHELL -c 'echo $((0x1))'" 0 "1\n" ""
docommand a101 "$SHELL -c 'echo $((0x01))'" 0 "1\n" ""
docommand a102 "$SHELL -c 'echo $((01))'" 0 "1\n" ""
docommand a103 "$SHELL -c 'echo $((0x10))'" 0 "16\n" ""
docommand a104 "$SHELL -c 'echo $((0100))'" 0 "64\n" ""
docommand a105 "$SHELL -c 'echo $((010+10))'" 0 "18\n" ""
docommand a106 "$SHELL -c 'echo $((019))'" "!=0" "" IGNORE

#
# Tests for precedence and chained operators
#
docommand a120 "$SHELL -c 'echo $((1+2*3))'" 0 "7\n" ""
docommand a121 "$SHELL -c 'echo $((1+(2*3)))'" 0 "7\n" ""
docommand a122 "$SHELL -c 'echo $((2*5+2*3))'" 0 "16\n" ""
docommand a123 "$SHELL -c 'echo $((2*5+2*3+1))'" 0 "17\n" ""
docommand a124 "$SHELL -c 'echo $((2*5+2*3+1+2+3))'" 0 "22\n" ""
docommand a125 "$SHELL -c 'echo $(((1)+(2)))'" 0 "3\n" ""
docommand a126 "$SHELL -c 'echo $(((2/3)!=(5/3)==(3/3)))'" 0 "1\n" ""
docommand a127 "$SHELL -c 'echo $((0!=1==1))'" 0 "1\n" ""
docommand a128 "$SHELL -c 'echo $((0!=3==3))'" 0 "0\n" ""
docommand a129 "$SHELL -c 'echo $((2*5+2*3*2+1))'" 0 "23\n" ""

#
# Tests for the assignement operators
#
docommand a150 "$SHELL -c 'unset a; echo $((a += 3))'" 0 "3\n" ""
docommand a151 "$SHELL -c 'a=0; echo $((a += 3))'" 0 "3\n" ""
docommand a152 "$SHELL -c 'a=0; echo $((a -= 3))'" 0 "-3\n" ""
docommand a153 "$SHELL -c 'a=2; echo $((a *= 3))'" 0 "6\n" ""
docommand a154 "$SHELL -c 'a=2; echo $((a /= 2))'" 0 "1\n" ""
docommand a155 "$SHELL -c 'a=17; echo $((a %= 5))'" 0 "2\n" ""
docommand a156 "$SHELL -c 'a=1; echo $((a <<= 3))'" 0 "8\n" ""
docommand a157 "$SHELL -c 'a=8; echo $((a >>= 3))'" 0 "1\n" ""
docommand a158 "$SHELL -c 'a=123; echo $((a &= 7))'" 0 "3\n" ""
docommand a159 "$SHELL -c 'a=123; echo $((a ^= 7))'" 0 "124\n" ""
docommand a160 "$SHELL -c 'a=123; echo $((a |= 7))'" 0 "127\n" ""

#
# Tests for command expansion inside arithmetic expansion
#
docommand a180 "$SHELL -c 'echo $(($(true)==$(true)))'" "!=0" "" IGNORE
docommand a181 "$SHELL -c 'echo $((`true`==`true`))'" "!=0" "" IGNORE
docommand a182 "$SHELL -c 'echo $(($(echo 1)==$(echo 1)))'" 0 "1\n" ""
docommand a183 "$SHELL -c 'echo $((`echo 1`==`echo 1`))'" 0 "1\n" ""

#
# Tests to check whether 0 && x or 1 || x correctly do not
# evaluate x
#
docommand a200 "$SHELL -c 'echo $((0 && 1))'" 0 "0\n" ""
docommand a201 "$SHELL -c 'echo $((0 && 1/0))'" 0 "0\n" ""
docommand a202 "$SHELL -c 'echo $((0 && (1/0)))'" 0 "0\n" ""
docommand a203 "$SHELL -c 'echo $((1 && (1/0)))'" "!=0" "" IGNORE

docommand a204 "$SHELL -c 'echo $((1 || 1))'" 0 "1\n" ""
docommand a205 "$SHELL -c 'echo $((1 || 1/0))'" 0 "1\n" ""
docommand a206 "$SHELL -c 'echo $((1 || (1/0)))'" 0 "1\n" ""
docommand a207 "$SHELL -c 'echo $((0 || (1/0)))'" "!=0" "" IGNORE

docommand a208 "$SHELL -c 'unset a; echo $((1 && a++)); echo \$a'" 0 "0\n1\n" ""
docommand a209 "$SHELL -c 'unset a; echo $((0 && a++)); echo \$a'" 0 "0\n\n" ""
docommand a210 "$SHELL -c 'unset a; echo $((0 && (a++))); echo \$a'" 0 "0\n\n" ""

docommand a211 "$SHELL -c 'unset a; echo $((0 || a++)); echo \$a'" 0 "0\n1\n" ""
docommand a212 "$SHELL -c 'unset a; echo $((1 || a++)); echo \$a'" 0 "1\n\n" ""
docommand a213 "$SHELL -c 'unset a; echo $((1 || (a++))); echo \$a'" 0 "1\n\n" ""

docommand a214 "$SHELL -c 'unset a; echo $((1 && --a)); echo \$a'" 0 "1\n-1\n" ""
docommand a215 "$SHELL -c 'unset a; echo $((0 && --a)); echo \$a'" 0 "0\n\n" ""
docommand a216 "$SHELL -c 'unset a; echo $((0 && (--a))); echo \$a'" 0 "0\n\n" ""

docommand a217 "$SHELL -c 'unset a; echo $((0 || --a)); echo \$a'" 0 "1\n-1\n" ""
docommand a218 "$SHELL -c 'unset a; echo $((1 || --a)); echo \$a'" 0 "1\n\n" ""
docommand a219 "$SHELL -c 'unset a; echo $((1 || (--a))); echo \$a'" 0 "1\n\n" ""

#
# Tests for conditional expressions
#
docommand a250 "$SHELL -c 'echo $((1 ? 15 : 12))'" 0 "15\n" ""
docommand a251 "$SHELL -c 'echo $((0 ? 15 : 12))'" 0 "12\n" ""
docommand a252 "$SHELL -c 'echo $((1 ? (15) : (12)))'" 0 "15\n" ""
docommand a253 "$SHELL -c 'echo $((0 ? (15) : (12)))'" 0 "12\n" ""
docommand a254 "$SHELL -c 'echo $((1 ? 1+2+3 : 4+5+6))'" 0 "6\n" ""
docommand a255 "$SHELL -c 'echo $((0 ? 1+2+3 : 4+5+6))'" 0 "15\n" ""
docommand a256 "$SHELL -c 'echo $((1 ? (1+2+3) : (4+5+6)))'" 0 "6\n" ""
docommand a257 "$SHELL -c 'echo $((0 ? (1+2+3) : (4+5+6)))'" 0 "15\n" ""
docommand a258 "$SHELL -c 'unset a; unset b; echo $((1 ? a++ : b--)); echo \$a; echo \$b'" 0 "0\n1\n\n" ""
docommand a259 "$SHELL -c 'unset a; unset b; echo $((0 ? a++ : b--)); echo \$a; echo \$b'" 0 "0\n\n-1\n" ""
docommand a260 "$SHELL -c 'unset a; b=-1; echo $((0 ? a++ : b--)); echo \$a; echo \$b'" 0 "-1\n\n-2\n" ""
docommand a261 "$SHELL -c 'unset a; unset b; echo $((1 ? (a++) : (b--))); echo \$a; echo \$b'" 0 "0\n1\n\n" ""
docommand a262 "$SHELL -c 'unset a; unset b; echo $((0 ? (a++) : (b--))); echo \$a; echo \$b'" 0 "0\n\n-1\n" ""
docommand a263 "$SHELL -c 'unset a; b=-1; echo $((0 ? (a++) : (b--))); echo \$a; echo \$b'" 0 "-1\n\n-2\n" ""

#
# Tests for the comma operator
#
docommand a300 "$SHELL -c 'echo $((1 , 15))'" 0 "15\n" ""
docommand a301 "$SHELL -c 'echo $((a=2 , 15)); echo \$a'" 0 "15\n2\n" ""
docommand a302 "$SHELL -c 'echo $((a=2*3 , 15)); echo \$a'" 0 "15\n6\n" ""
docommand a303 "$SHELL -c 'echo $((a=2*3*4 , 15)); echo \$a'" 0 "15\n24\n" ""
docommand a304 "$SHELL -c 'echo $(((1) , (15)))'" 0 "15\n" ""
docommand a305 "$SHELL -c 'echo $(((a=2) , (15))); echo \$a'" 0 "15\n2\n" ""
docommand a306 "$SHELL -c 'echo $(((a=2*3) , (15))); echo \$a'" 0 "15\n6\n" ""
docommand a307 "$SHELL -c 'echo $((a=(2*3) , (15))); echo \$a'" 0 "15\n6\n" ""
docommand a308 "$SHELL -c 'echo $(((a=2*3*4) , (15))); echo \$a'" 0 "15\n24\n" ""
docommand a309 "$SHELL -c 'unset a; echo $((a++ , 15)); echo \$a'" 0 "15\n1\n" ""
docommand a310 "$SHELL -c 'unset a; echo $((a++ , ++a)); echo \$a'" 0 "2\n2\n" ""
docommand a311 "$SHELL -c 'unset a; echo $((a++ , a++)); echo \$a'" 0 "1\n2\n" ""

#
# Test cases that are similar to the tests from ksh93
#
docommand ak01a "$SHELL -c 'x=1 y=2 z=3; echo $((2+2))'" 0 "4\n" ""
docommand ak01b "$SHELL -c 'x=1 y=2 z=3; echo $((2+2 != 4))'" 0 "0\n" ""
docommand ak02a "$SHELL -c 'x=1 y=2 z=3; echo $((x+y))'" 0 "3\n" ""
docommand ak02b "$SHELL -c 'x=1 y=2 z=3; echo $((x+y!=z))'" 0 "0\n" ""
docommand ak03a "$SHELL -c 'x=1 y=2 z=3; echo $((\$x+\$y))'" 0 "3\n" ""
docommand ak03b "$SHELL -c 'x=1 y=2 z=3; echo $((\$x+\$y!=\$z))'" 0 "0\n" ""
docommand ak04a "$SHELL -c 'x=1 y=2 z=3; echo $(((x|y)))'" 0 "3\n" ""
docommand ak04b "$SHELL -c 'x=1 y=2 z=3; echo $(((x|y)!=z))'" 0 "0\n" ""
docommand ak05  "$SHELL -c 'x=1 y=2 z=3; echo $((x >= z))'" 0 "0\n" ""
docommand ak06  "$SHELL -c 'x=1 y=2 z=3; echo $((y+3 != z+2))'" 0 "0\n" ""
docommand ak07  "$SHELL -c 'x=1 y=2 z=3; echo $((y<<2 != 1<<3))'" 0 "0\n" ""
docommand ak08  "$SHELL -c 'x=1 y=2 z=3; echo $((133%10 != 3))'" 0 "0\n" ""
docommand ak09  "$SHELL -c 'd=0; echo $((d || 1))'" 0 "1\n" ""
docommand ak10  "$SHELL -c 'd=0; echo $((d++ != 0))'" 0 "0\n" ""
docommand ak11  "$SHELL -c 'd=1; echo $((--d != 0))'" 0 "0\n" ""
docommand ak12  "$SHELL -c 'd=0; echo $(( (d++,6)!=6 && d!=1))'" 0 "0\n" ""
docommand ak13  "$SHELL -c 'd=0; echo $(( (1?2+1:3*4+d++)!=3 || d!=0))'" 0 "0\n" ""
docommand ak14  "$SHELL -c 'i=1; echo $(( (i?0:1)))'" 0 "0\n" ""
docommand ak15  "$SHELL -c 'i=1; echo $(( (1 || 1 && 0) != 1))'" 0 "0\n" ""
docommand ak16  "$SHELL -c 'x=1; echo $(( (x=-x) != -1 ))'" 0 "0\n" ""
docommand ak17  "$SHELL -c 'x=2; echo $(( 1$((\$x))3 != 123 ))'" 0 "0\n" ""


success
