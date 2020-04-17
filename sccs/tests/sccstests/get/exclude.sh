#! /bin/sh

# Basic tests for the SCCS get exclude feature

# Read test core functions
. ../../common/test-common

g=foo
s=s.$g
p=p.$g
z=z.$g

remove $z $s $p $g

cp s.exclude $s
docommand ex1 "${get} -p -s $s" 0 "this is line one
this is line two
this is line three
this is line four
" ""

remove $z $s $p $g
success
