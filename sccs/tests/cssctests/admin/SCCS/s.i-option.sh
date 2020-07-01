hV6,sum=44877
s 00001/00001/00084
d D 1.2 2015/06/03 00:06:43+0200 joerg 2 1
S s 19377
c ../common/test-common -> ../../common/test-common
e
s 00085/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 19238
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8eb95d9e
G p sccs/tests/cssctests/admin/i-option.sh
t
T
I 1
#! /bin/sh

# i-option.sh:  Testing for correct operation of admin -i.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g=bar
s=s.$g
remove $s foo $g

remove $g
echo '%M%' > foo
test x`cat foo` = x'%M%' || miscarry cannot create file foo.

docommand I1 "${vg_admin} -ifoo $s" 0 "" IGNORE
docommand I2 "${get} -r1.1 -p $s"      0 "$g\n" IGNORE
remove foo s.bar

# -i on its own means read from stdin.

echo baz | \
docommand I3 "${vg_admin} -i $s" 0 "" IGNORE
docommand I4 "${get} -r1.1 -p $s"      0 "baz\n" IGNORE


remove $s $g foo

# If the file specified by -i does not exist, make sure that not
# only is there a fatal exit, but neither the s-file or the x-file is 
# left behind.

g=foo
s=s.$g
x=x.$g

remove $g $s $x
docommand I5 "${vg_admin} -i$g $s" 1 "" IGNORE

echo_nonl "I6..."
if test -f $s; then
    fail I6: The file $s should not have been created.
fi
echo 'passed'

echo_nonl "I7..."
if test -f $x; then
    fail I7: The temporary file $x should have been deleted.
fi
remove $g $s $x
echo 'passed'



# Now check that we get the number of inserted lines correct for the
# first delta.

remove $g $s $x
echo_nonl "" > $g
docommand I8 "${vg_admin} -i$g $s" 0 "" IGNORE
docommand I9 "${prs}  -d:Li: $s" 0 "00000\n" IGNORE

# Check that the deleted and unchanged lines are also zero.
docommand I10 "${prs} -d:Ld: $s" 0 "00000\n" IGNORE
docommand I11 "${prs} -d:Lu: $s" 0 "00000\n" IGNORE

remove $g $s $x
echo_nonl "\n" > $g
docommand I12 "${vg_admin} -i$g $s" 0 "" IGNORE
docommand I13 "${prs}  -d:Li: $s" 0 "00001\n" IGNORE

# Check that the deleted and unchanged lines are also zero.
docommand I14 "${prs} -d:Ld: $s" 0 "00000\n" IGNORE
docommand I15 "${prs} -d:Lu: $s" 0 "00000\n" IGNORE


remove $g $s $x
echo_nonl "hello\nworld\n" > $g
docommand I16 "${vg_admin} -i$g $s" 0 "" IGNORE
docommand I17 "${prs}  -d:Li: $s" 0 "00002\n" IGNORE



remove $x s.bar s.foo foo bar command.log 
success
E 1
