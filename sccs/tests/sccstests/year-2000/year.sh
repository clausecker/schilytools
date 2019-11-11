#! /bin/sh

# Basic tests for SCCS extreme year numbers

# Read test core functions
. ../../common/test-common

IS_64=`file ${prs} | grep 'ELF 64'`

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
if [ "$IS_64" ]; then
	docommand y6 "${prs} -e '-d:I: :D: :T:' s.3000" 0 IGNORE IGNORE
else
	#
	# prs in 32 bit mode is expected to fail with y3000
	#
	docommand y6 "${prs} -e '-d:I: :D: :T:' s.3000" 1 IGNORE IGNORE
fi

remove $z $s $p $g $output
success
