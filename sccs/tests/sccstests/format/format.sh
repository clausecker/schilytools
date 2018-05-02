#! /bin/sh

# Basic tests for SCCS time stamps

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

docommand format01 "${admin} -V6 -n $s" 0 IGNORE IGNORE

if grep -s 'hV6' $s > /dev/null; then
	format=v6
else
	format=v4
fi

logname=`id -un`
if test -z "$logname"
then
	logname=`logname`
fi
logname=`echo $logname | sed -e 's/ /_/g'`

echo test | tail +2 > /dev/null 2>/dev/null
if [ "$?" -eq 0 ]; then
	plustwo=+2
else
	plustwo='-n +2'
fi

if test "$format" = v6
then
	tail $plustwo $s | sed \
		-e 's^ [0-9]*/../.. ..:..:..^ yy/mm/dd hh:mm:ss^' \
		-e 's/ss\.[0-9]*/ss.fffffffff/' \
		-e 's/[+-][0-9][0-9][0-9][0-9] /+zzzz /' \
		-e "s: $logname: uuu:" \
		-e 's/G r .............*/G r xxxxxxxxxxxxx/' | \
		grep -v 'G p ' > format
	diffs=`diff format reference-v6`
	if test ! -z "$diffs"
	then
		fail "Deviations from SCCSv6 format: $diffs"
	fi
else
	tail $plustwo $s | sed \
		-e 's^ [0-9]*/../.. ..:..:..^ yy/mm/dd hh:mm:ss^' \
		-e "s: $logname: uuu:" > format
	diffs=`diff format reference-v4`
	if test ! -z "$diffs"
	then
		fail "Deviations from SCCSv4 format: $diffs"
	fi
fi
echo "format$format...passed"

remove $z $s $p $g $output $error format
success
