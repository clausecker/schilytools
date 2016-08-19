h46873
s 00004/00000/00038
d D 1.3 16/08/18 22:20:29 joerg 3 2
c touch -t fuer gtouch
e
s 00006/00006/00032
d D 1.2 15/06/03 00:06:45 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00038/00000/00000
d D 1.1 11/05/29 20:19:37 joerg 1 0
c date and time created 11/05/29 20:19:37 by joerg
e
u
U
f e 0
f y 
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
cmd=delta		# for ../common/optv
ocmd=${delta}		# for ../common/optv
E 2
I 2
cmd=delta		# for ../../common/optv
ocmd=${delta}		# for ../../common/optv
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
docommand de1 "${get} -e $s" 0 IGNORE IGNORE
echo '%M%' > $g		|| miscarry "Could not create $g"
touch 0101000090 $g	|| miscarry "Could not touch $g"
I 3
if [ -f 0101000090 ]; then
	remove 0101000090
	touch -t 199001010000 $g	|| miscarry "Could not touch $g"
fi
E 3
expect_fail=true
docommand de2 "${delta} -o -ySomeComment $s" 0 IGNORE IGNORE
if  grep 90/01/01 $s > /dev/null 2> /dev/null
then
	echo "SCCS delta -o is supported"
else
	fail "SCCS delta -o unsupported"
fi

remove $z $s $p $g $output $error
success
E 1
