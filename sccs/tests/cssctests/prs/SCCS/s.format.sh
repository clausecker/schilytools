h26436
s 00001/00001/00052
d D 1.2 15/06/03 00:06:44 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00053/00000/00000
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

# format.sh:  Testing for correct interpretation of the format string.

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

# expands_to () {
#     # $1 -- label
#     # $2 -- format
#     # $3 -- expansion
# docommand $1 "${vg_prs} \"-d$2\" -r1.1 s.1 s.foobar" 1 "$3" "IGNORE"
# }

remove s.1 p.1 1 z.1 s.foobar

# Create file
echo "Descriptive Text" > DESC
docommand f1 "${admin} -n -tDESC s.1" 0 "" ""
remove DESC

docommand f2 "${vg_prs} -d':M:
X' s.1" 0 "1
X
" ""

docommand f3 "${vg_prs} -d'hello' s.1" 0 "hello
" ""

docommand f4a "${vg_prs} -d':M:
' s.1" 0 "1

" ""

docommand f4b "${vg_prs} -d':M:\n' s.1" 0 "1
" ""

docommand f5 "${vg_prs} -d':M:
' s.1 s.1" 0 "1

1

" ""



remove s.1 p.1 z.1 1 command.log
success
E 1
