hV6,sum=63355
s 00001/00001/00047
d D 1.2 2015/06/03 00:06:45+0200 joerg 2 1
S s 38139
c ../common/test-common -> ../../common/test-common
e
s 00048/00000/00000
d D 1.1 2011/05/29 21:15:25+0200 joerg 1 0
S s 38000
c date and time created 11/05/29 21:15:25 by joerg
e
u
U
f e 0
f y 
G r 0e46e8b606db4
G p sccs/tests/sccstests/get/getkeyw.sh
t
T
I 1
#! /bin/sh

# Basic tests for SCCS keyword extensions %d% %e% %g% %h%

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
output=get.output

remove $z $s $p $g

#
# Checking whether SCCS supports extended keywords
#
echo '%d%' > $g		|| miscarry "Could not create $g"
echo '%e%' >> $g	|| miscarry "Could not append a line to $g"
echo '%g%' >> $g	|| miscarry "Could not append a line to $g"
echo '%h%' >> $g	|| miscarry "Could not append a line to $g"
docommand keyw1 "${admin} -n -i$g $s" 0 "" IGNORE
#
# The keywords %d% %e% %g% %h% should not be expanded by default
#
docommand keyw2 "${get} -p $s" 0 "%d%\n%e%\n%g%\n%h%\n" IGNORE
#
# If all keywords are expanded, no '%' is expected in the output
# 1) set 'y' flag to enable %d% %e% %g% %h% expansion
#
expect_fail=true
docommand keyw3 "${admin} -fyd,e,g,h $s" 0 "" IGNORE
docommand keyw4a "${get} -p $s > $output" 0 "" IGNORE
docommand keyw4b "grep '%' $output" IGNORE "" IGNORE
expect_fail=false
#
# 2) set 'x' flag to enable expansion for all additional keywords
#
expect_fail=true
docommand keyw5 "${admin} -dy -fx $s" 0 "" IGNORE
docommand keyw6a "${get} -p $s > $output" 0 "" IGNORE
docommand keyw6b "grep '%' $output" IGNORE "" IGNORE
expect_fail=false

#remove $z $s $p $g 
remove $z $s $p $g $output
success
E 1
