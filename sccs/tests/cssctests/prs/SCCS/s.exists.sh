h34841
s 00001/00001/00044
d D 1.2 15/06/03 00:06:44 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00045/00000/00000
d D 1.1 10/04/29 02:05:14 joerg 1 0
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

# exists.sh:  Testing for correct behaviour when a file isn't there.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2


# If we invert the order of the arguments to prs here, so that the
# nonexistent file is named first, then those systems which support
# exceptions will operate correctly, and those which don't, won't.

expands_to () {
    # $1 -- label
    # $2 -- format
    # $3 -- expansion
docommand $1 "${vg_prs} \"-d$2\" -r1.1 s.1 s.foobar" 1 "$3" "IGNORE"
}

remove s.1 p.1 1 z.1 s.foobar

# Create file
echo "Descriptive Text" > DESC
docommand e1 "${admin} -n -tDESC s.1" 0 "" ""
remove DESC

docommand e2a "${vg_prs} -d':M:\nX' s.1" 0 "1\nX\n" ""
docommand e2b "${vg_prs} -d':M:\n' s.1" 0 "1\n" ""
docommand e2c "${vg_prs} -d':M:
' s.1" 0 "1

" ""

docommand e3 "${get} -e s.1" 0 "1.1\nnew delta 1.2\n0 lines\n" IGNORE
echo "hello from %M%" >> 1
docommand e4 "${delta} -y s.1" 0 "1.2\n1 inserted\n0 deleted\n0 unchanged\n" ""



expands_to X1  :I:      "1.1\n"



remove s.1 p.1 z.1 1 command.log
success
E 1
