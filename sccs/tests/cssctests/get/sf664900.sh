#! /bin/sh
# sf664900.sh: tests for SourceForge bug number 664900 

# Import common functions & definitions.
. ../../common/test-common


g=foo.txt
s=s.$g
x=x.$g 
z=z.$g
p=p.$g


remove $g $s $x $z $p
touch $g

docommand t1 "${admin} -i$g -r1.1.1.1 -n $s" 0 IGNORE IGNORE 
remove foo 

docommand t2 "${vg_get} $s" 1 IGNORE IGNORE

remove $g $s $x $z $p
success
