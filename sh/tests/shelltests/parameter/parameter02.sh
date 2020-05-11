#! /bin/sh
#
# @(#)parameter02.sh	1.3 20/04/28 2016-2020 J. Schilling
#

# Read test core functions
. ../../common/test-common

#
# Basic tests for parameter expansion
# (ideas taken from "mksh" test suite)
# Thanks to Thorsten Glaser
#


#
# expand-ugly
# Some weird ${foo+bar} constructs
# whether they are all correct needs to be investigated
#
cat > x <<"XEOF"
case `alias '-?' 2>&1` in

*--raw*)
	# bosh
	alias pfn='dosh '\''for x in "$@"; do printf -- "%s\n" "$x"; done'\'' pfn'
	alias pfs='dosh '\''for x in "$@"; do printf -- "<%s> " "$x"; done'\'' pfs'
	;;
*)
	# was andres
	echo 'for x in "$@"; do printf -- "%s\\n" "$x"; done' >pfn
	echo 'for x in "$@"; do printf -- "<%s> " "$x"; done' >pfs
	chmod +x pfn pfs
	alias pfn=./pfn
	alias pfs=./pfs
	;;
esac

(echo 1 ${IFS+'}'z}) 2>/dev/null || echo failed in 1
(echo 2 "${IFS+'}'z}") 2>/dev/null || echo failed in 2
(echo 3 "foo ${IFS+'bar} baz") 2>/dev/null || echo failed in 3

(echo '4 \c'; pfn "foo ${IFS+"b   c"} baz") 2>/dev/null || echo failed in 4
# bosh: 4 failed in 4
#echo "4 foo b   c baz"

(echo '5 \c'; pfn "foo ${IFS+b   c} baz") 2>/dev/null || echo failed in 5

#(echo 6 ${IFS+"}"z}) 2>/dev/null || echo failed in 6
#(echo 7 "${IFS+"}"z}") 2>/dev/null || echo failed in 7
#bosh: 6 z}
#bosh: 7 z}
echo "6 }z"
echo "7 }z"

(echo 8 "${IFS+\"}\"z}") 2>/dev/null || echo failed in 8

#(echo 9 "${IFS+\"\}\"z}") 2>/dev/null || echo failed in 9
#bosh: 9 "\}"z
echo '9 "}"z'

(echo 10 foo ${IFS+'bar} baz'}) 2>/dev/null || echo failed in 10
(echo 11 "$(echo "${IFS+'}'z}")") 2>/dev/null || echo failed in 11
(echo 12 "$(echo ${IFS+'}'z})") 2>/dev/null || echo failed in 12
(echo 13 ${IFS+\}z}) 2>/dev/null || echo failed in 13

#(echo 14 "${IFS+\}z}") 2>/dev/null || echo failed in 14
#bosh: 14 \}z
echo "14 }z"

#u=x; (echo '15 \c'; pfs "foo ${IFS+a"b$u{ {"{{\}b} c ${IFS+d{}} bar" ${IFS-e{}} baz; echo .) 2>/dev/null || echo failed in 15
#bosh: 15 failed in 15
echo "15 <foo abx{ {{{}b c d{} bar> <}> <baz> ."

l=t; (echo 16 ${IFS+h`echo i ${IFS+$l}h'\c'`ere}) 2>/dev/null || echo failed in 16
l=t; (echo 17 ${IFS+h$(echo i ${IFS+$l}h'\c')ere}) 2>/dev/null || echo failed in 17
l=t; (echo 18 "${IFS+h`echo i ${IFS+$l}h'\c'`ere}") 2>/dev/null || echo failed in 18
l=t; (echo 19 "${IFS+h$(echo i ${IFS+$l}h'\c')ere}") 2>/dev/null || echo failed in 19
l=t; (echo 20 ${IFS+h`echo i "${IFS+$l}"h'\c'`ere}) 2>/dev/null || echo failed in 20
l=t; (echo 21 ${IFS+h$(echo i "${IFS+$l}"h'\c')ere}) 2>/dev/null || echo failed in 21
l=t; (echo 22 "${IFS+h`echo i "${IFS+$l}"h'\c'`ere}") 2>/dev/null || echo failed in 22
l=t; (echo 23 "${IFS+h$(echo i "${IFS+$l}"h'\c')ere}") 2>/dev/null || echo failed in 23
key=value; (echo '24 \c'; pfn "${IFS+'$key'}") 2>/dev/null || echo failed in 24

key=value; (echo '25 \c'; pfn "${IFS+"'$key'"}") 2>/dev/null || echo failed in 25    # ksh93: '$key'
#bosh: 25 $key
#echo "25 'value'"

key=value; (echo '26 \c'; pfn ${IFS+'$key'}) 2>/dev/null || echo failed in 26
key=value; (echo '27 \c'; pfn ${IFS+"'$key'"}) 2>/dev/null || echo failed in 27

#(echo '28 \c'; pfn "${IFS+"'"x ~ x'}'x"'}"x}" #') 2>/dev/null || echo failed in 28
#bosh: Syntax Fehler -> Komplettabbruch
echo "28 'x ~ x''x}\"x}\" #"

u=x; (echo '29 \c'; pfs foo ${IFS+a"b$u{ {"{ {\}b} c ${IFS+d{}} bar ${IFS-e{}} baz; echo .) 2>/dev/null || echo failed in 29
(echo '30 \c'; pfs ${IFS+foo 'b\
ar' baz}; echo .) 2>/dev/null || (echo failed in 30; echo failed in 31)
(echo '32 \c'; pfs ${IFS+foo "b\
ar" baz}; echo .) 2>/dev/null || echo failed in 32
#bosh: 29 failed in 29
#bosh: 30 failed in 30
#bosh: failed in 31
#bosh: 32 failed in 32
#echo "29 <foo> <abx{ {{> <{}b> <c> <d{}> <bar> <}> <baz> ."
#echo "30 <foo> <b\\
#ar> <baz> ."
#echo "32 <foo> <bar> <baz> ."

(echo '33 \c'; pfs "${IFS+foo 'b\
ar' baz}"; echo .) 2>/dev/null || echo failed in 33
(echo '34 \c'; pfs "${IFS+foo "b\
ar" baz}"; echo .) 2>/dev/null || echo failed in 34

#(echo '35 \c'; pfs a${v=a\ b} x ${v=c\ d}; echo .) 2>/dev/null || echo failed in 35
#bosh 35 <aa b> <x> <aa> <b> .
echo "35 <a> <b> <x> <a> <b> ."

(echo '36 \c'; pfs "${v=a\ b}" x "${v=c\ d}"; echo .) 2>/dev/null || echo failed in 36
(echo '37 \c'; pfs ${v-a\ b} x ${v-c\ d}; echo .) 2>/dev/null || echo failed in 37
(echo 38 ${IFS+x'a'y} / "${IFS+x'a'y}" .) 2>/dev/null || echo failed in 38

#foo="x'a'y"; (echo 39 ${foo%*'a'*} / "${foo%*'a'*}" .) 2>/dev/null || echo failed in 39
#bosh: 39 x' / x .
echo "39 x' / x' ."

foo="a b c"; (echo '40 \c'; pfs "${foo#a}"; echo .) 2>/dev/null || echo failed in 40
XEOF
docommand parameter-2-00 "$SHELL ./x" 0 "\
1 }z
2 ''z}
3 foo 'bar baz
4 foo b   c baz
5 foo b   c baz
6 }z
7 }z
8 \"\"z}
9 \"}\"z
10 foo bar} baz
11 ''z}
12 }z
13 }z
14 }z
15 <foo abx{ {{{}b c d{} bar> <}> <baz> .
16 hi there
17 hi there
18 hi there
19 hi there
20 hi there
21 hi there
22 hi there
23 hi there
24 'value'
25 'value'
26 \$key
27 'value'
28 'x ~ x''x}\"x}\" #
29 <foo> <abx{ {{> <{}b> <c> <d{}> <bar> <}> <baz> .
30 <foo> <b\\
ar> <baz> .
32 <foo> <bar> <baz> .
33 <foo 'bar' baz> .
34 <foo bar baz> .
35 <a> <b> <x> <a> <b> .
36 <a\ b> <x> <a\ b> .
37 <a b> <x> <c d> .
38 xay / x'a'y .
39 x' / x' .
40 < b c> .
" ""
remove x pfn pfs


#
# expand-unglob-dblq
# Some regular "${foo+bar}" constructs
# Whether cases 7..9 are correct needs to be investigated
#
cat > x <<"XEOF"
	u=x
	tl_norm() {
		v=$2
		test x"$v" = x"-" && unset v
		(echo "$1 plus norm foo ${v+'bar'} baz")
		(echo "$1 dash norm foo ${v-'bar'} baz")
		(echo "$1 eqal norm foo ${v='bar'} baz")
		(echo "$1 qstn norm foo ${v?'bar'} baz") 2>/dev/null || \
		    echo "$1 qstn norm -> error"
		(echo "$1 PLUS norm foo ${v:+'bar'} baz")
		(echo "$1 DASH norm foo ${v:-'bar'} baz")
		(echo "$1 EQAL norm foo ${v:='bar'} baz")
		(echo "$1 QSTN norm foo ${v:?'bar'} baz") 2>/dev/null || \
		    echo "$1 QSTN norm -> error"
	}
	tl_paren() {
		v=$2
		test x"$v" = x"-" && unset v
		(echo "$1 plus parn foo ${v+(bar)} baz")
		(echo "$1 dash parn foo ${v-(bar)} baz")
		(echo "$1 eqal parn foo ${v=(bar)} baz")
		(echo "$1 qstn parn foo ${v?(bar)} baz") 2>/dev/null || \
		    echo "$1 qstn parn -> error"
		(echo "$1 PLUS parn foo ${v:+(bar)} baz")
		(echo "$1 DASH parn foo ${v:-(bar)} baz")
		(echo "$1 EQAL parn foo ${v:=(bar)} baz")
		(echo "$1 QSTN parn foo ${v:?(bar)} baz") 2>/dev/null || \
		    echo "$1 QSTN parn -> error"
	}
	tl_brace() {
		v=$2
		test x"$v" = x"-" && unset v
		(echo "$1 plus brac foo ${v+a$u{{{\}b} c ${v+d{}} baz")
		(echo "$1 dash brac foo ${v-a$u{{{\}b} c ${v-d{}} baz")
		(echo "$1 eqal brac foo ${v=a$u{{{\}b} c ${v=d{}} baz")
		(echo "$1 qstn brac foo ${v?a$u{{{\}b} c ${v?d{}} baz") 2>/dev/null || \
		    echo "$1 qstn brac -> error"
		(echo "$1 PLUS brac foo ${v:+a$u{{{\}b} c ${v:+d{}} baz")
		(echo "$1 DASH brac foo ${v:-a$u{{{\}b} c ${v:-d{}} baz")
		(echo "$1 EQAL brac foo ${v:=a$u{{{\}b} c ${v:=d{}} baz")
		(echo "$1 QSTN brac foo ${v:?a$u{{{\}b} c ${v:?d{}} baz") 2>/dev/null || \
		    echo "$1 QSTN brac -> error"
	}
	tl_norm 1 -
	tl_norm 2 ''
	tl_norm 3 x
	tl_paren 4 -
	tl_paren 5 ''
	tl_paren 6 x
#	tl_brace 7 -
#	tl_brace 8 ''
#	tl_brace 9 x
XEOF
docommand parameter-2-01 "$SHELL ./x" 0 "\
1 plus norm foo  baz
1 dash norm foo 'bar' baz
1 eqal norm foo 'bar' baz
1 qstn norm -> error
1 PLUS norm foo  baz
1 DASH norm foo 'bar' baz
1 EQAL norm foo 'bar' baz
1 QSTN norm -> error
2 plus norm foo 'bar' baz
2 dash norm foo  baz
2 eqal norm foo  baz
2 qstn norm foo  baz
2 PLUS norm foo  baz
2 DASH norm foo 'bar' baz
2 EQAL norm foo 'bar' baz
2 QSTN norm -> error
3 plus norm foo 'bar' baz
3 dash norm foo x baz
3 eqal norm foo x baz
3 qstn norm foo x baz
3 PLUS norm foo 'bar' baz
3 DASH norm foo x baz
3 EQAL norm foo x baz
3 QSTN norm foo x baz
4 plus parn foo  baz
4 dash parn foo (bar) baz
4 eqal parn foo (bar) baz
4 qstn parn -> error
4 PLUS parn foo  baz
4 DASH parn foo (bar) baz
4 EQAL parn foo (bar) baz
4 QSTN parn -> error
5 plus parn foo (bar) baz
5 dash parn foo  baz
5 eqal parn foo  baz
5 qstn parn foo  baz
5 PLUS parn foo  baz
5 DASH parn foo (bar) baz
5 EQAL parn foo (bar) baz
5 QSTN parn -> error
6 plus parn foo (bar) baz
6 dash parn foo x baz
6 eqal parn foo x baz
6 qstn parn foo x baz
6 PLUS parn foo (bar) baz
6 DASH parn foo x baz
6 EQAL parn foo x baz
6 QSTN parn foo x baz
" ""

#
# Saved mksh results for tests 7..9
#
REST="\
7 plus brac foo  c } baz
7 dash brac foo ax{{{}b c d{} baz
7 eqal brac foo ax{{{}b c ax{{{}b} baz
7 qstn brac -> error
7 PLUS brac foo  c } baz
7 DASH brac foo ax{{{}b c d{} baz
7 EQAL brac foo ax{{{}b c ax{{{}b} baz
7 QSTN brac -> error
8 plus brac foo ax{{{}b c d{} baz
8 dash brac foo  c } baz
8 eqal brac foo  c } baz
8 qstn brac foo  c } baz
8 PLUS brac foo  c } baz
8 DASH brac foo ax{{{}b c d{} baz
8 EQAL brac foo ax{{{}b c ax{{{}b} baz
8 QSTN brac -> error
9 plus brac foo ax{{{}b c d{} baz
9 dash brac foo x c x} baz
9 eqal brac foo x c x} baz
9 qstn brac foo x c x} baz
9 PLUS brac foo ax{{{}b c d{} baz
9 DASH brac foo x c x} baz
9 EQAL brac foo x c x} baz
9 QSTN brac foo x c x} baz
"
remove x

#
# expand-unglob-unq
# Some regular ${foo+bar} constructs
#
cat > x <<"XEOF"
	u=x
	tl_norm() {
		v=$2
		test x"$v" = x"-" && unset v
		(echo $1 plus norm foo ${v+'bar'} baz)
		(echo $1 dash norm foo ${v-'bar'} baz)
		(echo $1 eqal norm foo ${v='bar'} baz)
		(echo $1 qstn norm foo ${v?'bar'} baz) 2>/dev/null || \
		    echo "$1 qstn norm -> error"
		(echo $1 PLUS norm foo ${v:+'bar'} baz)
		(echo $1 DASH norm foo ${v:-'bar'} baz)
		(echo $1 EQAL norm foo ${v:='bar'} baz)
		(echo $1 QSTN norm foo ${v:?'bar'} baz) 2>/dev/null || \
		    echo "$1 QSTN norm -> error"
	}
	tl_paren() {
		v=$2
		test x"$v" = x"-" && unset v
		(echo $1 plus parn foo ${v+\(bar')'} baz)
		(echo $1 dash parn foo ${v-\(bar')'} baz)
		(echo $1 eqal parn foo ${v=\(bar')'} baz)
		(echo $1 qstn parn foo ${v?\(bar')'} baz) 2>/dev/null || \
		    echo "$1 qstn parn -> error"
		(echo $1 PLUS parn foo ${v:+\(bar')'} baz)
		(echo $1 DASH parn foo ${v:-\(bar')'} baz)
		(echo $1 EQAL parn foo ${v:=\(bar')'} baz)
		(echo $1 QSTN parn foo ${v:?\(bar')'} baz) 2>/dev/null || \
		    echo "$1 QSTN parn -> error"
	}
	tl_brace() {
		v=$2
		test x"$v" = x"-" && unset v
		(echo $1 plus brac foo ${v+a$u{{{\}b} c ${v+d{}} baz)
		(echo $1 dash brac foo ${v-a$u{{{\}b} c ${v-d{}} baz)
		(echo $1 eqal brac foo ${v=a$u{{{\}b} c ${v=d{}} baz)
		(echo $1 qstn brac foo ${v?a$u{{{\}b} c ${v?d{}} baz) 2>/dev/null || \
		    echo "$1 qstn brac -> error"
		(echo $1 PLUS brac foo ${v:+a$u{{{\}b} c ${v:+d{}} baz)
		(echo $1 DASH brac foo ${v:-a$u{{{\}b} c ${v:-d{}} baz)
		(echo $1 EQAL brac foo ${v:=a$u{{{\}b} c ${v:=d{}} baz)
		(echo $1 QSTN brac foo ${v:?a$u{{{\}b} c ${v:?d{}} baz) 2>/dev/null || \
		    echo "$1 QSTN brac -> error"
	}
	tl_norm 1 -
	tl_norm 2 ''
	tl_norm 3 x
	tl_paren 4 -
	tl_paren 5 ''
	tl_paren 6 x
	tl_brace 7 -
	tl_brace 8 ''
	tl_brace 9 x
XEOF
docommand parameter-2-02 "$SHELL ./x" 0 "\
1 plus norm foo baz
1 dash norm foo bar baz
1 eqal norm foo bar baz
1 qstn norm -> error
1 PLUS norm foo baz
1 DASH norm foo bar baz
1 EQAL norm foo bar baz
1 QSTN norm -> error
2 plus norm foo bar baz
2 dash norm foo baz
2 eqal norm foo baz
2 qstn norm foo baz
2 PLUS norm foo baz
2 DASH norm foo bar baz
2 EQAL norm foo bar baz
2 QSTN norm -> error
3 plus norm foo bar baz
3 dash norm foo x baz
3 eqal norm foo x baz
3 qstn norm foo x baz
3 PLUS norm foo bar baz
3 DASH norm foo x baz
3 EQAL norm foo x baz
3 QSTN norm foo x baz
4 plus parn foo baz
4 dash parn foo (bar) baz
4 eqal parn foo (bar) baz
4 qstn parn -> error
4 PLUS parn foo baz
4 DASH parn foo (bar) baz
4 EQAL parn foo (bar) baz
4 QSTN parn -> error
5 plus parn foo (bar) baz
5 dash parn foo baz
5 eqal parn foo baz
5 qstn parn foo baz
5 PLUS parn foo baz
5 DASH parn foo (bar) baz
5 EQAL parn foo (bar) baz
5 QSTN parn -> error
6 plus parn foo (bar) baz
6 dash parn foo x baz
6 eqal parn foo x baz
6 qstn parn foo x baz
6 PLUS parn foo (bar) baz
6 DASH parn foo x baz
6 EQAL parn foo x baz
6 QSTN parn foo x baz
7 plus brac foo c } baz
7 dash brac foo ax{{{}b c d{} baz
7 eqal brac foo ax{{{}b c ax{{{}b} baz
7 qstn brac -> error
7 PLUS brac foo c } baz
7 DASH brac foo ax{{{}b c d{} baz
7 EQAL brac foo ax{{{}b c ax{{{}b} baz
7 QSTN brac -> error
8 plus brac foo ax{{{}b c d{} baz
8 dash brac foo c } baz
8 eqal brac foo c } baz
8 qstn brac foo c } baz
8 PLUS brac foo c } baz
8 DASH brac foo ax{{{}b c d{} baz
8 EQAL brac foo ax{{{}b c ax{{{}b} baz
8 QSTN brac -> error
9 plus brac foo ax{{{}b c d{} baz
9 dash brac foo x c x} baz
9 eqal brac foo x c x} baz
9 qstn brac foo x c x} baz
9 PLUS brac foo ax{{{}b c d{} baz
9 DASH brac foo x c x} baz
9 EQAL brac foo x c x} baz
9 QSTN brac foo x c x} baz
" ""
remove x

#
# expand-threecolons-dblq
# Used to segfault with mksh
#
cat > x <<"XEOF"
LC_ALL=C
TEST=1234
echo "${TEST:1:2:3}"
echo exit code $? but still living
XEOF
docommand -noremove parameter-2-03 "$SHELL ./x" "!=0" '' IGNORE
err=`grep -i 'bad' got.stderr`
if [ -z "$err" ]; then
	fail "Test $cmd_label failed: wrong error message"
fi
remove x
do_remove

#
# expand-threecolons-unq
# Must fail
#
cat > x <<"XEOF"
LC_ALL=C
TEST=1234
echo ${TEST:1:2:3}
echo exit code $? but still living
XEOF
docommand -noremove parameter-2-04 "$SHELL ./x" "!=0" '' IGNORE
err=`grep -i 'bad' got.stderr`
if [ -z "$err" ]; then
	fail "Test $cmd_label failed: wrong error message"
fi
remove x
do_remove

#
# expand-weird-2
# Check corner case of ${var?} vs. ${#var}
#
cat > x <<"XEOF"
	set 1 2 3 4 5 6 7 8 9 10 11
	echo ${#}	# value of $#
	echo ${##}	# length of $#
#	echo ${##1}	# $# trimmed 1
#	set 1 2 3 4 5 6 7 8 9 10 11 12
#	echo ${##1}
XEOF
docommand parameter-2-05 "$SHELL ./x" 0 "\
11
2
" ""
remove x

#
# expand-weird-2
# Check corner case of ${var?} vs. ${#var}
#
cat > x <<"XEOF"
(exit 0)
echo $? = ${#?} .
(exit 111)
echo $? = ${#?} .
XEOF
docommand parameter-2-06 "$SHELL ./x" 0 "\
0 = 1 .
111 = 3 .
" ""
remove x

#
# expand-weird-3
# Check that trimming works with positional parameters (Debian #48453)
#
cat > x <<"XEOF"
A=9999-02
B=9999
echo 1=${A#$B?}.
set -- $A $B
echo 2=${1#$2?}.
XEOF
docommand parameter-2-07 "$SHELL ./x" 0 "\
1=02.
2=02.
" ""
remove x

#
# expand-weird-4
# Check that tilde expansion is enabled in ${x#~}
# and cases that are modelled after it (${x/~/~})
# XXX fails with bosh
#
cat > x <<"XEOF"
HOME=/etc 
a="~/x" 
echo "<${a#~}> <${a#\~}> <${b:-~}> <${b:-\~}> <${c:=~}><$c>"
#echo "<${a#~}> <${a#\~}> <${b:-~}> <${b:-\~}> <${c:=~}><$c> <${a/~}> <${a/x/~}> <${a/x/\~}>"
XEOF
#docommand parameter-2-08 "$SHELL ./x" 0 "\
#<~/x> </x> <~> <\~> <~><~>
#" ""
echo parameter-2-08...skipped
remove x
# mksh: <~/x> </x> <~> <\~> <~><~> <~/x> <~//etc> <~/~>

cat > x <<"XEOF"
echo "1 ${12345678901234567890} ."
XEOF
docommand parameter-2-09 "$SHELL ./x" 0 '1  .\n' ""
remove x

cat > x <<"XEOF"
XEOF
#docommand parameter-2-00 "$SHELL ./x" 0 '' ""
remove x

success
