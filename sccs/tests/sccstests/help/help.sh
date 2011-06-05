#! /bin/sh

# Basic tests for SCCS help

# Read test core functions
. ../common/test-common

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
