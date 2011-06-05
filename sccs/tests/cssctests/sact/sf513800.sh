#! /bin/sh

# sf513800.sh:  Tests relating to SOurceForge bug 513800

# Import common functions & definitions.
. ../common/test-common

g=foo
s=s.$g
p=p.$g

remove $s $p $g

# Extract the test input files. 
for n in 1 2 
do
    filename=sf513800_${n}.uue
    ../../testutils/uu_decode --decode < $filename || miscarry could not uudecode $filename
done

docommand s1 "${vg_sact} $s" 0 IGNORE IGNORE 



remove $s $p $g
success
