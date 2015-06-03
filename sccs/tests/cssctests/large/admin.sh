#! /bin/sh
# admin.sh:  The creation of very large (>99999 lines) SCCS files.

# Import common functions & definitions.
. ../../common/test-common

g=bigfile.txt
s=s.$g
z=z.$g
x=x.$g
p=p.$g

lines=100002

remove command.log log log.stdout log.stderr $g $s $z $x $p

( ${SRCROOT}/tests/testutils/yes '%C%' | head -${lines} > $g )  || miscarry Cannot create large input file.

docommand A1 "${vg_admin} -i${g} ${s}" 0 "" ""
mv ${g} old.${g} || miscarry "Rename failed"

# Make sure we can retrieve the file.
docommand A2 "${vg_get} -k $s" 0 "1.1\n100002 lines\n" ""
diff old.${g} ${g} || fail A2 "cannot correctly retrieve stored file"
remove old.${g} ${g}
docommand A3 "${get} $s" 0 "1.1\n100002 lines\n" ""

# Make sure the number of lines is correct.
docommand A4 "${prs} -d:Li: $s" 0 "99999\n" ""


remove command.log log log.stdout log.stderr $g $s $z $x $p
success
