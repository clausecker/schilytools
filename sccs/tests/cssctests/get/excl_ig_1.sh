#! /bin/sh
# excl_ig_1.sh:  Tests for exclusions and ignores.

# Import common functions & definitions.
. ../common/test-common


g=incl_excl_1
s=s.$g
x=x.$g 
z=z.$g
p=p.$g
remove $g $s $x $z $p

cp s.incl_excl_1.input $s

docommand xg1 "${vg_get} -r1.1 -p $s"      0 "" IGNORE

docommand xg2 "${vg_get} -r1.2 -p $s"      0 \
    "This line was added in version 1.2
" IGNORE

docommand xg3 "${vg_get} -r1.3 -p $s"      0 \
    "This line was added in version 1.2
This line was added in version 1.3
" IGNORE

docommand xg4 "${vg_get} -r1.4 -p $s"      0 \
    "This line was added in version 1.2
This line was added in version 1.3
This line was added in version 1.4
" IGNORE

docommand xg5 "${vg_get} -r1.5 -p $s"      0 \
    "This line was added in version 1.2
This line was added in version 1.3
This line was added in version 1.4
This line was added in version 1.5
" IGNORE

# Revision 1.6 ignores SID 1.2.
docommand xg6 "${vg_get} -r1.6 -p $s"      0 \
"This line was added in version 1.3
This line was added in version 1.4
This line was added in version 1.5
This line was added in version 1.6; that version also ignores 1.2\n" \
IGNORE

remove $g $s $x $z $p
success
