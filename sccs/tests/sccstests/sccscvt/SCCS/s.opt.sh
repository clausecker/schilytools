h44269
s 00025/00000/00000
d D 1.1 18/12/04 00:39:29 joerg 1 0
c date and time created 18/12/04 00:39:29 by joerg
e
u
U
f e 0
t
T
I 1
#! /bin/sh

# Basic tests for extended SCCS options

# Read test core functions
. ../../common/test-common

cmd=sccscvt		# for ../../common/optv
ocmd=${sccscvt}		# for ../../common/optv
g=foo
s=s.$g
p=p.$g
z=z.$g
output=got.output	# for ../../common/optv
error=got.error		# for ../../common/optv

remove $z $s $p $g

#
# Checking whether SCCS ${cmd} supports extended options
#
. ../../common/optv

remove $z $s $p $g $output $error
success
E 1
