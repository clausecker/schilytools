hV6,sum=26073
s 00006/00006/00029
d D 1.2 2015/06/03 00:06:45+0200 joerg 2 1
S s 51647
c ../common/test-common -> ../../common/test-common
e
s 00035/00000/00000
d D 1.1 2011/05/29 20:19:37+0200 joerg 1 0
S s 50813
c date and time created 11/05/29 20:19:37 by joerg
e
u
U
f e 0
G r 0e46e8b60f0e5
G p sccs/tests/sccstests/get/opt.sh
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
cmd=get			# for ../common/optv
ocmd=${get}		# for ../common/optv
E 2
I 2
cmd=get			# for ../../common/optv
ocmd=${get}		# for ../../common/optv
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

cp s.origd $s
expect_fail=true
docommand go1 "${get} -o $s" 0 IGNORE IGNORE
if ls -l $g 2> /dev/null | grep 1990 > /dev/null
then
	echo "SCCS get -o is supported"
else
	fail "SCCS get -o unsupported"
fi

remove $z $s $p $g $output $error
success
E 1
