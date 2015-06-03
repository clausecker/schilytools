#! /bin/sh

# sactbasic.sh:  Basic tests for sact

# Import common functions & definitions.
. ../../common/test-common

g=foo
s=s.$g
p=p.$g

remove $s $p $g

# It is invalid to supply no arguments to sact.
docommand sb1 "${vg_sact}" 1 IGNORE IGNORE 

remove $s $p $g
success
