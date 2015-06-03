#! /bin/sh

# Basic tests for extended SCCS keywords

# Read test core functions
. ../../common/test-common

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
