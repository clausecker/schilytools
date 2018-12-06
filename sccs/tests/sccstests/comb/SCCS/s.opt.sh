h43575
s 00025/00000/00000
d D 1.1 18/12/03 22:49:04 joerg 1 0
c date and time created 18/12/03 22:49:04 by joerg
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

cmd=comb		# for ../../common/optv
ocmd=${comb}		# for ../../common/optv
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
