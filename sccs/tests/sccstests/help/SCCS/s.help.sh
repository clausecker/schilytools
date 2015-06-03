h43600
s 00001/00001/00023
d D 1.2 15/06/03 00:06:45 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00024/00000/00000
d D 1.1 11/05/29 20:19:37 joerg 1 0
c date and time created 11/05/29 20:19:37 by joerg
e
u
U
f e 0
t
T
I 1
#! /bin/sh

# Basic tests for SCCS help

# Read test core functions
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g=foo
s=s.$g
p=p.$g
z=z.$g
output=get.output

remove $z $s $p $g

if test ! -x ${help}
then
	echo "XFAIL SCCS "help" command not found"
else
	docommand he1 "${help} stuck" 0 IGNORE ""
fi

remove $z $s $p $g $output
success
E 1
