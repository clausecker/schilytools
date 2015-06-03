h62580
s 00006/00006/00049
d D 1.2 15/06/03 00:06:45 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00055/00000/00000
d D 1.1 11/05/29 23:25:40 joerg 1 0
c date and time created 11/05/29 23:25:40 by joerg
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
cmd=admin		# for ../common/optv
ocmd=${admin}		# for ../common/optv
E 2
I 2
cmd=admin		# for ../../common/optv
ocmd=${admin}		# for ../../common/optv
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

echo '%M%' > $g		|| miscarry "Could not create $g"
touch 0101000090 $g	|| miscarry "Could not touch $g"
expect_fail=true
docommand go1 "${admin} -o -i$g -n $s" 0 IGNORE IGNORE
if  grep 90/01/01 $s > /dev/null 2> /dev/null
then
	echo "SCCS admin -o is supported"
	remove $s
	echo '%M%' > $g		|| miscarry "Could not create $g"
	touch -t 196001010000 $g
	ret=$?
	ls -l $g | grep 1960 > /dev/null
	ret2=$?
	if test $ret -eq 0 -a $ret2 -eq 0
	then
		docommand go2 "${admin} -o -i$g -n $s" 0 IGNORE IGNORE
		if grep 1960/01/01 $s
		then
			echo "SCCS admin -o works for file from 1960"
		else
			fail "SCCS admin -o failed for file from 1960"
		fi
	else
		fail "touch with date before 1970 not supported"
		echo "Skipping test go2"
	fi
else
	fail "SCCS admin -o unsupported"
fi

remove $z $s $p $g $output $error
success
E 1
