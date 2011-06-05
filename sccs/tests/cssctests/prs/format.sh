#! /bin/sh

# format.sh:  Testing for correct interpretation of the format string.

# Import common functions & definitions.
. ../common/test-common


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
