hV6,sum=22997
s 00001/00001/00099
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 63213
c ../common/test-common -> ../../common/test-common
e
s 00100/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 63074
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ef214e4
G p sccs/tests/cssctests/unget/output.sh
t
T
I 1
#! /bin/sh

# output.sh:  Testing for output format of unget.
#

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g1=new1.txt
g2=new2.txt
s1=s.$g1
s2=s.$g2
p1=p.$g1
p2=p.$g2
all="$s1 $g1 $p1 $s2 $g2 $p2 xxx1 xxx2 old.$g1 old.$g2"
remove $all


setup_an_edit () {
# $1 is the label.
echo_nonl $1"1-6:"
m=mfile
remove $all $m
echo "%M%" >$m || miscarry could not create $m.
docommand --silent ${1}1 "${admin}   -i $s1" 0 "" "" <mfile
docommand --silent ${1}2 "${admin} -i $s2" 0 "" ""   <mfile
remove $m
docommand --silent ${1}3 "${admin} -fb -fj $s1 $s2" 0 "" ""
docommand --silent ${1}4 "${get} -e -b -r1.1 $s1 $s2" 0 \
 "\ns.new1.txt:\n1.1\nnew delta 1.1.1.1\n1 lines\
\n\ns.new2.txt:\n1.1\nnew delta 1.1.1.1\n1 lines\n" \
	IGNORE
docommand              --silent ${1}5 "${get} -Gxxx1 -e -r1.1 $s1" 0 \
"1.1\nnew delta 1.2\n1 lines\n" \
	IGNORE
docommand              --silent ${1}6 "${get} -Gxxx2 -e -r1.1 $s2" 0 \
"1.1\nnew delta 1.2\n1 lines\n" \
	IGNORE
remove old.$g1 old.$g2
echo "passed "
}

# unget of only one file....
setup_an_edit a
docommand a7 "${vg_unget} -r1.1.1.1 $s1" 0 "1.1.1.1\n" ""
test -f $g1 && fail a7 unget failed to remove $g1
docommand a8 "${vg_unget} $s1" 0 "1.2\n" ""

setup_an_edit b
docommand b7 "${vg_unget} -r1.1.1.1 -s $s1" 0 "" ""
test -f $g1 && fail b7 unget -s failed to remove $g1
docommand b8 "${vg_unget} -s $s1" 0 "" ""

setup_an_edit c
docommand c7 "${vg_unget} -n -r1.1.1.1 $s1" 0 "1.1.1.1\n" ""
test -f $g1 || fail c7 unget -n removed $g1
docommand c8 "${vg_unget} -n $s1" 0 "1.2\n" ""

setup_an_edit d
docommand d7 "${vg_unget} -r1.1.1.1 -n -s $s1" 0 "" ""
test -f $g1 || fail d7 unget -n removed $g1
docommand d8 "${vg_unget} -n -s $s1" 0 "" ""


setup_an_edit e
docommand e7 "${vg_unget} -r1.1.1.1 $s1 $s2" 0 "\
\ns.new1.txt:\
\n1.1.1.1\
\n\
\ns.new2.txt:\
\n1.1.1.1\
\n\
" ""
test -f $g1 && fail e7 unget failed to remove $g1
test -f $g2 && fail e7 unget failed to remove $g2

docommand e8 "${vg_unget} $s1 $s2" 0 \
 "\ns.new1.txt:\n1.2\n\ns.new2.txt:\n1.2\n" ""



setup_an_edit f
docommand f7 "${vg_unget} -r1.1.1.1 -s $s1 $s2" 0 "" ""
test -f $g1 && fail f7 unget -s failed to remove $g1
test -f $g2 && fail f7 unget -s failed to remove $g2
docommand f8 "${vg_unget} -s $s1 $s2" 0 "" ""

setup_an_edit g
docommand g7 "${vg_unget} -r1.1.1.1 -n $s1 $s2" 0 \
 "\ns.new1.txt:\n1.1.1.1\n\ns.new2.txt:\n1.1.1.1\n" ""
test -f $g1 || fail g7 unget -n removed $g1
test -f $g2 || fail g7 unget -n removed $g2
docommand g8 "${vg_unget} -s $s1 $s2" 0 "" ""


###
### Cleanup and exit.
###
remove $all
success
E 1
