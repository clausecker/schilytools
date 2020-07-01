hV6,sum=18029
s 00001/00001/00032
d D 1.2 2015/06/03 00:06:45+0200 joerg 2 1
S s 58629
c ../common/test-common -> ../../common/test-common
e
s 00033/00000/00000
d D 1.1 2011/05/29 20:19:37+0200 joerg 1 0
S s 58490
c date and time created 11/05/29 20:19:37 by joerg
e
u
U
f e 0
G r 0e46e8b6408a6
G p sccs/tests/sccstests/prs/prskeyw.sh
t
T
I 1
#! /bin/sh

# Basic tests for extended SCCS keywords

# Read test core functions
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

g=foo
s=s.$g
p=p.$g
z=z.$g
output=got.output
error=got.error

remove $z $s $p $g

docommand pkw1 "${prs} -e '-d:I: :D: :T:' s.dates" 0 "1.2 99/12/31 23:59:59
1.1 70/01/01 00:00:00\n" ""
expect_fail=true
docommand pkw2 "${prs} -e '-d:I: :d: :T:' s.dates" 0 "1.2 1999/12/31 23:59:59
1.1 1970/01/01 00:00:00\n" ""
docommand pkw3 "${prs} -e '-d:I: :DY: :T:' s.dates" 0 "1.2 1999 23:59:59
1.1 1970 00:00:00\n" ""

docommand pkw4 "${prs} -e '-d:I: :D: :T:' s.1960" 0 "1.2 99/12/31 23:59:59
1.1 60/01/01 00:00:00\n" ""
docommand pkw5 "${prs} -e '-d:I: :d: :T:' s.1960" 0 "1.2 1999/12/31 23:59:59
1.1 1960/01/01 00:00:00\n" ""
docommand pkw6 "${prs} -e '-d:I: :DY: :T:' s.1960" 0 "1.2 1999 23:59:59
1.1 1960 00:00:00\n" ""

remove $z $s $p $g $output $error
success
E 1
