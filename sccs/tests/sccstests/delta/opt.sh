#! /bin/sh

# Basic tests for extended SCCS options

# Read test core functions
. ../../common/test-common

cmd=delta		# for ../../common/optv
ocmd=${delta}		# for ../../common/optv
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

cp s.origd $s
docommand de1 "${get} -e $s" 0 IGNORE IGNORE
echo '%M%' > $g		|| miscarry "Could not create $g"
touch 0101000090 $g	|| miscarry "Could not touch $g"
if [ -f 0101000090 ]; then
	remove 0101000090
	touch -t 199001010000 $g	|| miscarry "Could not touch $g"
fi
expect_fail=true
docommand de2 "${delta} -o -ySomeComment $s" 0 IGNORE IGNORE
if  grep 90/01/01 $s > /dev/null 2> /dev/null
then
	echo "SCCS delta -o is supported"
else
	fail "SCCS delta -o unsupported"
fi

rm $s
${admin} -n $s
${get} -e $s 2>/dev/null > /dev/null
echo "123 %M%" > $g
docommand de10 "${delta} -Xprepend -ySomeComment $s" 0 IGNORE IGNORE
docommand de11 "${get} -p $s" 0 "123 foo\n" IGNORE
${get} -e $s 2>/dev/null > /dev/null
echo "000" > $g
docommand de12 "${delta} -Xprepend -ySomeComment $s" 0 IGNORE IGNORE
docommand de13 "${get} -p $s" 0 "000\n123 foo\n" IGNORE

remove $z $s $p $g $output $error
success
