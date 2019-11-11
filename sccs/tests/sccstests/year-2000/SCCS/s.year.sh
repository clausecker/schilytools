h38436
s 00010/00001/00039
d D 1.3 19/11/08 01:34:41 joerg 3 2
c Mit y3000 erwarten wir nun um 32 Bit Modus exit code 1
e
s 00001/00001/00039
d D 1.2 15/06/03 00:06:45 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00040/00000/00000
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

# Basic tests for SCCS extreme year numbers

# Read test core functions
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

I 3
IS_64=`file ${prs} | grep 'ELF 64'`

E 3
g=foo
s=s.$g
p=p.$g
z=z.$g
output=get.output

remove $z $s $p $g

expect_fail=true
docommand y1 "${prs} -e '-d:I: :D: :T:' s.dates" 0 IGNORE IGNORE
docommand y2 "${prs} -e '-d:I: :D: :T:' s.dates" 0 "1.6 68/12/31 23:59:59
1.5 38/01/01 00:00:00
1.4 37/12/31 23:59:59
1.3 00/01/01 00:00:00
1.2 99/12/31 23:59:59
1.1 70/01/01 00:00:00
" IGNORE

docommand y3 "${prs} -e '-d:I: :D: :T:' s.2069" 0 IGNORE IGNORE
docommand y4 "${prs} -e '-d:I: :D: :T:' s.2069" 0 "1.7 69/01/01 00:00:00
1.6 68/12/31 23:59:59
1.5 38/01/01 00:00:00
1.4 37/12/31 23:59:59
1.3 00/01/01 00:00:00
1.2 99/12/31 23:59:59
1.1 70/01/01 00:00:00
" IGNORE

docommand y5 "${prs} -e '-d:I: :D: :T:' s.1960" 0 IGNORE IGNORE
D 3
docommand y6 "${prs} -e '-d:I: :D: :T:' s.3000" 0 IGNORE IGNORE
E 3
I 3
if [ "$IS_64" ]; then
	docommand y6 "${prs} -e '-d:I: :D: :T:' s.3000" 0 IGNORE IGNORE
else
	#
	# prs in 32 bit mode is expected to fail with y3000
	#
	docommand y6 "${prs} -e '-d:I: :D: :T:' s.3000" 1 IGNORE IGNORE
fi
E 3

remove $z $s $p $g $output
success
E 1
