#! /bin/sh

# Basic tests for SCCS flags

# Read test core functions
. ../../common/test-common

g=foo
s=s.$g
p=p.$g
z=z.$g

remove $z $s $p $g

#
# Checking whether SCCS supports the 'i' flag with parameter
#
# This test will not be passed by SCCS version 4
#
expect_fail=true
echo '%M%' > $g		|| miscarry "Could not create $g"
docommand fl1 "${admin} -n '-fi%M%' -i$g $s" 0 "" ""
remove $z $s $p $g
echo '%M%' > $g		|| miscarry "Could not create $g"
docommand fl2 "${admin} -n '-fi%M%%I%' -i$g $s" 1 "" IGNORE

#
# Checking whether admin -fy will switch off the "No id keywords" warning
#
remove $z $s $p $g
echo 'bla' > $g		|| miscarry "Could not create $g"
docommand fl3 "${admin} -n -fy -i$g $s" 0 "" ""
if test -r $s
then
	remove $g
	docommand fl4 "${get} $s" 0 IGNORE ""
else
	echo "Test fl4 skipped"
fi

remove $z $s $p $g
echo 'bla' > $g		|| miscarry "Could not create $g"
echo '%M%' >> $g	|| miscarry "Could not append a line to $g"
docommand fl5 "${admin} -n -fs1 -i$g $s" 0 "" ""
if test -r $s
then
	remove $g
	docommand fl6 "${get} -p $s" 0 "bla
%M%\n" IGNORE
	docommand fl7 "${admin} -fs2 $s" 0 "" ""
	docommand fl8 "${get} -p $s" 0 "bla
foo\n" IGNORE
	#
	# Check whether the line count is reset for multiple files
	#
	docommand fl9 "${get} -p $s $s" 0 "bla
foo\nbla\nfoo\n" IGNORE
else
	echo "Test fl6..fl9 skipped"
fi

remove $z $s $p $g
success
