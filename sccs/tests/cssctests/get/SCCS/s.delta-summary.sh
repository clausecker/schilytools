h15235
s 00001/00001/00063
d D 1.3 15/06/03 00:06:44 joerg 3 2
c ../common/test-common -> ../../common/test-common
e
s 00001/00001/00063
d D 1.2 15/06/01 23:55:23 joerg 2 1
c ../../testutils/ -> ${SRCROOT}/tests/testutils/
e
s 00064/00000/00000
d D 1.1 11/04/30 19:50:22 joerg 1 0
c date and time created 11/04/30 19:50:22 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

# Import common functions & definitions.
D 3
. ../common/test-common
E 3
I 3
. ../../common/test-common
E 3

g=keys.txt
s=s.$g
l=l.$g
p=p.$g
remove $s $g $l $p
D 2
../../testutils/uu_decode --decode < keys.uue || miscarry could not extract test file.
E 2
I 2
${SRCROOT}/tests/testutils/uu_decode --decode < keys.uue || miscarry could not extract test file.
E 2

summary="\
    1.2	97/10/25 23:04:58 james
	changed the one and only line

    1.1	97/10/25 23:04:20 james
	date and time created 97/10/25 23:04:20 by james
"


# Check the basic function of the -L option.
docommand L1 "${vg_get} -L s.keys.txt" 0 "$summary\n" "1.2\n1 lines\n"
remove $g

# Check that -lp does the same thing.
docommand L1p "${vg_get} -lp s.keys.txt" 0 "$summary\n" "1.2\n1 lines\n" 
remove $g

docommand L2 "${vg_get} -L s.keys.txt s.keys.txt" 0 "$summary\n$summary\n" "
s.keys.txt:
1.2
1 lines

s.keys.txt:
1.2
1 lines
"
remove $g

# Check that the "new delta" message also goes to stderr when we are using -L.
docommand L2e "${vg_get} -e -L s.keys.txt" 0 "$summary\n" "1.2
new delta 1.3
1 lines
"
# Reverse the effect of the edit.
remove $g $p

# Check that the delta summary is sent to stderr if -p is given (and that 
# the body goes to stdout)..
docommand L3 "${vg_get} -p -k -L s.keys.txt" 0 "$summary\n1.2 %I%\n" "1.2\n1 lines\n"
remove $g


# Generate an l-file ...
docommand o1a "${vg_get} -l s.keys.txt" 0 "1.2\n1 lines\n" ""

# ... and check its contents.
docommand o1b "cat $l" 0 "$summary\n" ""
remove $g $l


remove $s $g $l $p
success
E 1
