#! /bin/sh

# Basic tests for extended SCCS options

# Read test core functions
. ../common/test-common

cmd=what		# for ../common/optv
ocmd=${what}		# for ../common/optv
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

remove $z $s $p $g $output $error
success
