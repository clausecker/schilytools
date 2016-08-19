#! /bin/sh

# Basic tests for extended SCCS options

# Read test core functions
. ../../common/test-common

cmd=admin		# for ../../common/optv
ocmd=${admin}		# for ../../common/optv
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

echo '%M%' > $g		|| miscarry "Could not create $g"
touch 0101000090 $g	|| miscarry "Could not touch $g"
if [ -f 0101000090 ]; then
	remove 0101000090
	touch -t 199001010000 $g	|| miscarry "Could not touch $g"
fi
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
