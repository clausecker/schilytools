#! /bin/sh

# hardlinks.sh:  Testing for behaviour when there are hard links to
#                the s-file.  


# Import common functions & definitions.
. ../../common/test-common

g=foo
s=s.$g
s2=s.bar
files="$s $g $s2"

remove $files

docommand L1 "${admin} -n ${s}" 0 "" IGNORE
docommand L2 "test -r ${s}" 0 "" IGNORE

# Make a hard link.
if ln $s $s2
then
    # OS and filessystem support hard links - we can do the test

    # SCCS commands should fail on the SCCS file now, since the link 
    # count is not 1.  We try a selection. 
    docommand L3 "${vg_prs} ${s}"           1 IGNORE IGNORE
    docommand L4 "${vg_admin} -anobby ${s}" 1 IGNORE IGNORE
    docommand L5 "${vg_get} -g ${s}"        1 IGNORE IGNORE

    # For some reason the SCCS version of "val" does not make this check.
    # We do, but I don't think this functional difference is very 
    # important. 
    # expect_fail=true
    # docommand L6 "${val} ${s}"           1 IGNORE IGNORE
    # expect_fail=false
else
    echo "Your OS or your the file system do not support hard links - some tests skipped"
fi



remove s.bar 
success
