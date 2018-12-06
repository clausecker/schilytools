#! /bin/sh

# Basic tests for extended SCCS options

# Read test core functions
. ../../common/test-common
. ../../common/real-thing

cmd=val			# for ../../common/optv
ocmd=${val}		# for ../../common/optv
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

#
# SCCS_V6 is unset with the normal SCCS v4 tests and set to empty value
# in case we run the same tests in a way that let's all new history files
# to be created as SCCS v6 files.
#
sccs_v6=${SCCS_V6-false} 
: ${sccs_v6:=true}			# true/false depending on state

#
# TESTING_SCCS_V6 is set to true/false depending on wheher SCCS_V6=
# results in "admin -n s.file" to create a SCCS V6 history file.
#

if  $TESTING_REAL_SCCS; then

#
# Test the new SCCS val flags -v and -T
#

docommand val1 "${vg_val} -v s.hist-v4" 0 "SCCS V4 s.hist-v4\n" ""
docommand val2 "${vg_val} -v s.hist-v6" 0 "SCCS V6 s.hist-v6\n" ""

docommand -noremove val3 "${vg_val} -T s.time" 0 NONEMPTY ""
if grep "warning.*date" < got.stdout > /dev/null; then
	:
else
	fail "val3: Non-monotic time stamps not detected"
fi
do_remove

docommand -noremove val4 "${vg_val} -T s.time-v6-1" 0 NONEMPTY ""
if grep "warning.*date" < got.stdout > /dev/null; then
	:
else
	fail "val4: Non-monotic time stamps not detected"
fi
do_remove

docommand -noremove val5 "${vg_val} -T s.time-v6-2" 0 NONEMPTY ""
if grep "warning.*date" < got.stdout > /dev/null; then
	:
else
	fail "val5: Non-monotic time stamps not detected"
fi
do_remove

docommand -noremove val6 "${vg_val} -T s.time-v6-3" 0 NONEMPTY ""
if grep "warning.*date" < got.stdout > /dev/null; then
	:
else
	fail "val6: Non-monotic time stamps not detected"
fi
do_remove

#
# Time backwards, but different timezone, so val must be silent.
#
docommand val6 "${vg_val} -T s.time-v6-4" 0 "" ""

fi

remove $z $s $p $g $output $error
success
