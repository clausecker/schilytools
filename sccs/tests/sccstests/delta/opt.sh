#! /bin/sh

# Basic tests for extended SCCS options

# Read test core functions
. ../common/test-common

cmd=delta		# for ../common/optv
ocmd=${delta}		# for ../common/optv
g=foo
s=s.$g
p=p.$g
z=z.$g
output=got.output	# for ../common/optv
error=got.error		# for ../common/optv

remove $z $s $p $g

#
# Checking whether SCCS ${cmd} supports extended options
#
. ../common/optv

cp s.origd $s
docommand de1 "${get} -e $s" 0 IGNORE IGNORE
echo '%M%' > $g		|| miscarry "Could not create $g"
touch 0101000090 $g	|| miscarry "Could not touch $g"
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
