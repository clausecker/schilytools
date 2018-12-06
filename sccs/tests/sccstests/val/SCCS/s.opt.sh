h04494
s 00062/00000/00025
d D 1.3 18/11/27 20:56:43 joerg 3 2
c Neue tests fuer -v und -T
e
s 00006/00006/00019
d D 1.2 15/06/03 00:06:45 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00025/00000/00000
d D 1.1 11/05/29 20:19:37 joerg 1 0
c date and time created 11/05/29 20:19:37 by joerg
e
u
U
f e 0
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
I 3
. ../../common/real-thing
E 3
E 2

D 2
cmd=val			# for ../common/optv
ocmd=${val}		# for ../common/optv
E 2
I 2
cmd=val			# for ../../common/optv
ocmd=${val}		# for ../../common/optv
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

I 3
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

E 3
remove $z $s $p $g $output $error
success
E 1
