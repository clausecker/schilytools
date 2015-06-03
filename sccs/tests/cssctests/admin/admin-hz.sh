#! /bin/sh

# admin-hz.sh:  Tests for the -h and -z options of "admin".

# Import common functions & definitions.
. ../../common/test-common

g=new.txt
s=s.$g
p=p.$g
s2=s.spare
remove foo $s $g $p [zx].$g $s2

# Create SCCS file
echo 'hello from %M%' >foo

docommand c1 "${vg_admin} -ifoo $s" 0 "" ""
remove foo

# Make sure the checksum is checked as correct.
docommand c2 "${vg_admin} -h $s" 0 "" ""

# Now, create a copy with a changed checksum, but no other 
# differences.
# If we are testing SCCS v6, we need to repair the V6 header
# as the first sed command replaces V6 by V3.
docommand c3 " (sed -e '1y/0123456789/9876453210/' <$s | sed -e '1s/V3/V6/' >$s2) " 0 "" ""

# Check that we think that the checksum of the file is wrong.
docommand c4 "${vg_admin} -h $s2" 1 "" "IGNORE"

# Make sure that specifying "-h -z" does not cause the checksum 
# to be fixed (this is why we do it twice).
docommand c5 "${vg_admin} -h -z $s2" 1 "" "IGNORE"
docommand c6 "${vg_admin} -h -z $s2" 1 "" "IGNORE"

# Check that we still think it is wrong if we pass both files to 
# admin, no matter what the order.
docommand c7 "${vg_admin} -h $s $s2" 1 "" "IGNORE"
docommand c8 "${vg_admin} -h $s2 $s" 1 "" "IGNORE"


# Fix the checksum.
docommand c9 "${vg_admin} -z $s2" 0 "" ""

# Check that we are happy again.
docommand c10 "${vg_admin}  -h $s2" 0 "" ""
docommand c11 "${vg_admin}  -h $s $s2" 0 "" ""
docommand c12 "${vg_admin} -h $s2 $s" 0 "" ""

# Make sure the files are again identical.
docommand c13 "diff $s $s2" 0 "" "IGNORE"


### Cleanup and exit.
rm -rf test 
remove foo $s $g $p [zx].$g command.log $s2
success
