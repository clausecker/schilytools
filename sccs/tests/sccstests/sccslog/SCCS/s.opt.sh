h05240
s 00006/00006/00019
d D 1.2 15/06/03 00:06:45 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00025/00000/00000
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

# Basic tests for extended SCCS options

# Read test core functions
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

D 2
cmd=sccslog		# for ../common/optv
ocmd=${sccslog}		# for ../common/optv
E 2
I 2
cmd=sccslog		# for ../../common/optv
ocmd=${sccslog}		# for ../../common/optv
E 2
g=foo
s=s.$g
p=p.$g
z=z.$g
D 2
output=got.output	# for ../common/optv
error=got.error		# for ../common/optv
E 2
I 2
output=got.output	# for ../../common/optv
error=got.error		# for ../../common/optv
E 2

remove $z $s $p $g

#
# Checking whether SCCS ${cmd} supports extended options
#
D 2
. ../common/optv
E 2
I 2
. ../../common/optv
E 2

remove $z $s $p $g $output $error
success
E 1
