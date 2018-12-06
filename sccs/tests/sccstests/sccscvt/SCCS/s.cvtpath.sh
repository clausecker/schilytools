h04668
s 00059/00000/00000
d D 1.1 18/12/05 00:34:57 joerg 1 0
c date and time created 18/12/05 00:34:57 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

# Basic tests for "initial path" adding with sccscvt

# Read test core functions
. ../../common/test-common

g=foo
s=s.$g
p=p.$g
z=z.$g
g2=foo2
s2=s.$g2
p2=p.$g2
z2=z.$g2

#
# SCCS_V6 is unset with the normal SCCS v4 tests and set to empty value
# in case we run the same tests in a way that let's all new history files
# to be created as SCCS v6 files.
#
sccs_v6=${SCCS_V6-false} 
: ${sccs_v6:=true}			# true/false depending on state

if $sccs_v6; then
	exit				# Only test conversion to V6
fi


remove $z $s $p $g XXXX a .sccs

echo '%M%' > $g
mkdir .sccs				# Needed to compute Initial Path

docommand initpath1 "${admin} -N-XXXX -i. $g" 0 "" ""
docommand initpath2 "${sccscvt} -NXXXX -V6 $g" 0 "" ""
docommand initpath3 "${prs} -NXXXX -d:Gp: $g" 0 "foo\n" ""

docommand initpath11 "${admin} -NXXXX -fy -n a/b/c/$g" 0 "" ""
docommand initpath12 "${sccscvt} -NXXXX -V6 a/b/c/$g" 0 "" ""
docommand initpath13 "${prs} -NXXXX -d:Gp: a/b/c/$g" 0 "a/b/c/foo\n" ""
remove a

docommand initpath21 "${admin} -N -fy -n a/b/c/$g" 0 "" ""
docommand initpath22 "${sccscvt} -N -V6 a/b/c/$g" 0 "" ""
docommand initpath23 "${prs} -N -d:Gp: a/b/c/$g" 0 "a/b/c/foo\n" ""
remove a

docommand initpath31 "${admin} -Ns. -fy -n a/b/c/$s" 0 "" ""
docommand initpath32 "${sccscvt} -Ns. -V6 a/b/c/$s" 0 "" ""
docommand initpath33 "${prs} -Ns. -d:Gp: a/b/c/$s" 0 "a/b/c/foo\n" ""
remove a

docommand initpath41 "${admin} -NXXXX/s. -fy -n a/b/c/XXXX/$s" 0 "" ""
docommand initpath42 "${sccscvt} -NXXXX/s. -V6 a/b/c/XXXX/$s" 0 "" ""
docommand initpath43 "${prs} -NXXXX/s. -d:Gp: a/b/c/XXXX/$s" 0 "a/b/c/foo\n" ""

remove $z $s $p $g XXXX a .sccs
success
E 1
