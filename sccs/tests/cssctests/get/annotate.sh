#! /bin/sh

# Tests for the -n and -m options of get.

# Import common functions & definitions.
. ../common/test-common


f=1test
s=s.$f
p=p.$f
remove $f $s $p

echo "line1" >  $f
echo "line2" >> $f

docommand A1 "$admin -n -i$f $s" 0 "" IGNORE
test -r $s         || fail admin could not create $s

remove $f 

# Test the -n (annotate module name) option
docommand N1 "${vg_get} -p -n $s" 0 "$f\tline1\n$f\tline2\n" IGNORE

# Test the -m (annotate SID) option
docommand N2 "${vg_get} -p -m $s" 0 "1.1\tline1\n1.1\tline2\n" IGNORE

# Test both options together.
docommand N3 "${vg_get} -p -n -m $s" 0 "$f\t1.1\tline1\n$f\t1.1\tline2\n" IGNORE

# Make a new delta to further test the -m option.
docommand G1 "${vg_get} -e $s" 0 "1.1\nnew delta 1.2\n2 lines\n" ""

echo "line3" >> $f
docommand D1 "$delta '-yAdded line three' $s" 0 \
    "1.2\n1 inserted\n0 deleted\n2 unchanged\n" \
    IGNORE

# Test the -m (annotate SID) option with several deltas...
docommand N4 "${vg_get} -p -m $s" 0 \
    "1.1\tline1\n1.1\tline2\n1.2\tline3\n" \
    IGNORE

# Now make a branch.
docommand G2 "${vg_get} -e -r1.1 $s" 0 "1.1\nnew delta 1.1.1.1\n2 lines\n" ""

echo "line4 %Z%" >> $f
docommand D2 "$delta '-yAdded line: a branch' $s" 0 \
    "1.1.1.1\n1 inserted\n0 deleted\n2 unchanged\n" \
    IGNORE

# Test both options together.
docommand N5 "${vg_get} -p -m -n -r1.1.1.1 $s" 0 \
    "$f\t1.1\tline1\n$f\t1.1\tline2\n$f\t1.1.1.1\tline4 @(#)\n" \
    IGNORE

remove command.log
remove $f $s $p
success
