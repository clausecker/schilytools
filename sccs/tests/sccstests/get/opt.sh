#! /bin/sh

# Basic tests for extended SCCS options

# Read test core functions
. ../common/test-common

cmd=get			# for ../common/optv
ocmd=${get}		# for ../common/optv
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
