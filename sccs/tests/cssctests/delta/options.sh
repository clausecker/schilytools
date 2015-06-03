#! /bin/sh
# options.sh:  Testing for the various command-line options of "delta".

# Import common functions & definitions.
. ../../common/test-common

remove command.log log log.stdout log.stderr
mkdir test 2>/dev/null

g=foo
s=s.$g
p=p.$g
z=z.$g
files="$g $s $p $z ${g}_1 ${g}_2"

remove $files

append() {
   f="$1"
   shift
   echo  "$@" >> "$f" || miscarry "Could not append a line to $1" 
}


test -d test || mkdir test || miscarry "Could not create subdirectory 'test'" >&2

exec </dev/null

# Do some setup.
docommand o1 "${admin} -n $s" 0 IGNORE IGNORE

docommand o2 "${get} -e $s" 0 IGNORE IGNORE
append $g "hello, world"
docommand o3 "${vg_delta} -r1.1 -yFirst $s" 0 \
 "1.2\n1 inserted\n0 deleted\n0 unchanged\n" IGNORE

docommand o4 "${get} -e $s" 0 IGNORE IGNORE
append $g "hello, world"


# The -s option should suppress output to stdout.
docommand o5 "${vg_delta} -s -r1.2 -ySecond $s" 0 "" IGNORE

# Now we have deltas 1.1, 1.2 and 1.3.

# Now edit the same file twice (but different SIDs)
docommand o6 "${admin} -dj $s" 0 IGNORE IGNORE
docommand o7 "${get} -r1.1 -e -p $s > ${g}_1" 0 IGNORE IGNORE
append ${g}_1 "this is appended to file 1"

docommand o8 "${get} -r1.2 -e -p $s > ${g}_2" 0 IGNORE IGNORE
append ${g}_2 "this is appended to file 2"



mv ${g}_1 ${g} || miscarry "Could not rename ${g}_1 to ${g}"

# Failure to specify an SCCS file name is an error.
docommand o9 "${vg_delta} -r1.2.1.1 -yBranch1" 1 IGNORE IGNORE

# Unknown command line option is an error (we pick an unlikely one)
docommand o10 "${vg_delta}"' -! -yBranch1' 1 IGNORE IGNORE

# Ambiguity in SID selection is an error
docommand o11 "${vg_delta} -r1 -yBranch1 $s" 1 IGNORE IGNORE
docommand o12 "${vg_delta} -yBranch1 $s" 1 IGNORE IGNORE

# Invalid branch should be detected (note return value 2 not 1)
docommand o13 "${vg_delta} -r1.2.1.1a -yBranch2 $s" 1 IGNORE IGNORE

docommand o14 "${vg_delta} -r1.1.1.1 -yBranch1 $s" 0 IGNORE IGNORE

# the p-file should still exist
docommand o15 "test -r $p" 0 "" ""

mv ${g}_2 ${g} || miscarry "Could not rename ${g}_2 to ${g}"
docommand o16 "${vg_delta} -r1.2.1.1 -p -yBranch2 $s" 0 \
"1.2.1.1\n1a2\n> this is appended to file 2\n1 inserted\n0 deleted\n1 unchanged\n" IGNORE

# the p-file should now be gone
docommand o17 "test -r $p" 1 "" ""

remove $files 
success
