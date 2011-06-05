#! /bin/sh
# sf479916.sh:  Tests for SourceForge bug 479916,
#               which relates to correct selection of
#               a delta from a branch when the -t flag is 
#               used.

# Import common functions & definitions.
. ../common/test-common


g=X
s=s.$g
x=x.$g 
z=z.$g
p=p.$g

remove $g $s $x $z $p

echo "%Z%" > $g
echo "%Z%" >> $g

docommand prep1 "${admin} -fb -i$g $s" 0 IGNORE IGNORE
remove $g


docommand prep2 "${get} -e -b -t $s" 0 "1.1
new delta 1.1.1.1
2 lines
" IGNORE
docommand prep3 "${delta} -yNoComment $s" 0 IGNORE IGNORE

docommand prep4 "${get} -e -b -t $s" 0 "1.1.1.1
new delta 1.1.2.1
2 lines
" IGNORE

docommand prep5 "${delta} -yNoComment $s" 0 IGNORE IGNORE


# Now for the actual test - the "-t" option should pich the 
# most recent delata, which is 1.1.2.1, not 1.1.1.1.

docommand T1 "${vg_get} -t $s" 0 "1.1.2.1
2 lines
" IGNORE


# Create another trunk delta
docommand prep6 "${get} -e $s" 0 "1.1
new delta 1.2
2 lines
" IGNORE
docommand prep7 "${delta} -yNoComment $s" 0 IGNORE IGNORE


docommand T2 "${vg_get} -t $s" 0 "1.2
2 lines
" IGNORE

docommand T3 "${vg_get}  $s" 0 "1.2
2 lines
" IGNORE


# Add another release.
docommand prep8 "${get} -r2 -e $s" 0 "1.2
new delta 2.1
2 lines
" IGNORE
docommand prep9 "${delta} -yNoComment $s" 0 IGNORE IGNORE

# ... and another branch off 1.2.
docommand prep10 "${get} -r1.2 -e $s" 0 "1.2
new delta 1.2.1.1
2 lines
" IGNORE
docommand prep11 "${delta} -yNoComment $s" 0 IGNORE IGNORE


docommand T4 "${vg_get} -t -r2 $s" 0 "2.1
2 lines
" IGNORE

docommand T5 "${vg_get} -t -r2.1 $s" 0 "2.1
2 lines
" IGNORE

docommand T6 "${vg_get} -r2.1 $s" 0 "2.1
2 lines
" IGNORE

docommand T7 "${vg_get} -t $s" 0 "2.1
2 lines
" IGNORE

docommand T8 "${vg_get} -t -r1 $s" 0 "1.2.1.1
2 lines
" IGNORE

docommand T9 "${vg_get} -t -r1.1 $s" 0 "1.1.2.1
2 lines
" IGNORE

docommand T10 "${vg_get} -t -r1.1.1 $s" 0 "1.1.1.1
2 lines
" IGNORE

docommand T11 "${vg_get} -t -r1.1.2 $s" 0 "1.1.2.1
2 lines
" IGNORE

docommand T12 "${vg_get} -t -r1.1.1.1 $s" 0 "1.1.1.1
2 lines
" IGNORE

docommand T13 "${vg_get} -t -r1.1.2.1 $s" 0 "1.1.2.1
2 lines
" IGNORE


remove $g $s $x $z $p
success
