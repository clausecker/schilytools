h33573
s 00023/00000/00000
d D 1.1 20/04/16 00:23:42 joerg 1 0
c date and time created 20/04/16 00:23:42 by joerg
e
u
U
f e 0
t
T
I 1
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
E 1
