hV6,sum=35089
s 00001/00001/00043
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 09663
c ../common/test-common -> ../../common/test-common
e
s 00044/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 09524
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ed58b33
G p sccs/tests/cssctests/get/sep_subst.sh
t
T
I 1
#! /bin/sh

## sep_subst.sh: 
#     Make sure that the delta information substituted for
#     each line is the information for the delta that we are
#     actually getting, not the delta information for the delta
#     which last touched that line.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2


f=1test
s=s.$f
p=p.$f
remove $f $s $p

echo "line1 %I%" >  $f
echo "line2" >> $f

docommand prep1 "$admin -n -i$f $s" 0 "" IGNORE
test -r $s         || fail admin could not create $s

remove $f 

# Make a new delta 
docommand prep2 "$get -e $s" 0 "1.1\nnew delta 1.2\n2 lines\n" ""

echo "line3 %I%" >> $f
docommand prep3 "$delta '-yAdded line three' $s" 0 \
    "1.2\n1 inserted\n0 deleted\n2 unchanged\n" \
    IGNORE


docommand G1 "${vg_get} -p -r1.1 $s" 0 "line1 1.1\nline2\n" \
	    "1.1\n2 lines\n"
docommand G2 "${vg_get} -p -r1.2 $s" 0 "line1 1.2\nline2\nline3 1.2\n" \
	    "1.2\n3 lines\n"



remove command.log
remove $f $s $p
success
E 1
