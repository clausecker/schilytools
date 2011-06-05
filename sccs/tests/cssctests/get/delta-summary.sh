#! /bin/sh

# Import common functions & definitions.
. ../common/test-common

g=keys.txt
s=s.$g
l=l.$g
p=p.$g
remove $s $g $l $p
../../testutils/uu_decode --decode < keys.uue || miscarry could not extract test file.

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
