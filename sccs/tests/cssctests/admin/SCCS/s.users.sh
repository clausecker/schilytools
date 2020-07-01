hV6,sum=40025
s 00001/00001/00057
d D 1.2 2015/06/03 00:06:43+0200 joerg 2 1
S s 14811
c ../common/test-common -> ../../common/test-common
e
s 00058/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 14672
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8eb503db
G p sccs/tests/cssctests/admin/users.sh
t
T
I 1
#! /bin/sh

# users.sh:  Testing for -a and -e options of admin.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g=bar
s=s.${g}
z=z.${g}

remove $s $g $z foo command.log last.command core 
remove expected.stderr got.stderr expected.stdout got.stdout

remove foo
echo '%M%' > foo
test `cat foo` = '%M%' || miscarry cannot create file foo.

docommand A1 "${vg_admin} -ifoo ${s}" 0 "" IGNORE
remove foo

#no_users="\n"
no_users="none\n\n"

# We may not have a "prt".
# docommand A2 "${prt} -u $s" 0 "\ns.bar:\n\n Users allowed to make deltas -- \n\teveryone\n" ""

# If the authorised user list is empty everyone is allowed to make deltas.
docommand A2 "${prs} -d:UN: $s" 0 "${no_users}" ""

# Check the adding of users.
docommand A3 "${vg_admin} -abashful ${s}" 0 "" ""
docommand A4 "${vg_admin} -agrumpy  ${s}" 0 "" ""
docommand A5 "${vg_admin} -asleepy  ${s}" 0 "" ""

# docommand A6 "${prt} -u $s" 0 "\ns.bar:\n\n Users allowed to make deltas -- \n\tsleepy\n\tgrumpy\n\tbashful\n" ""
docommand A6 "${vg_prs} -d:UN: $s" 0 "sleepy\ngrumpy\nbashful\n\n" ""


# Check the removal of users.
docommand A7 "${vg_admin} -esleepy  ${s}" 0 "" ""
docommand A8 "${vg_prs} -d:UN: $s" 0 "grumpy\nbashful\n\n" ""
docommand A9 "${vg_admin} -ebashful  ${s}" 0 "" ""
docommand A10 "${prs} -d:UN: $s" 0 "grumpy\n\n" ""
docommand A11 "${vg_admin} -egrumpy  ${s}" 0 "" ""
docommand A12 "${prs} -d:UN: $s" 0 "${no_users}" ""

# Adding and removing a user in the same command should still
# result in the user being added.
docommand A13 "${vg_admin} -asleepy -esleepy ${s}" 0 "" ""
docommand A14 "${prs} -d:UN: $s" 0 "sleepy\n\n" ""


remove $s $g $z foo command.log last.command core 
remove expected.stderr got.stderr expected.stdout got.stdout


success
E 1
