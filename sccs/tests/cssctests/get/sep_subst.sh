#! /bin/sh

## sep_subst.sh: 
#     Make sure that the delta information substituted for
#     each line is the information for the delta that we are
#     actually getting, not the delta information for the delta
#     which last touched that line.

# Import common functions & definitions.
. ../common/test-common


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
