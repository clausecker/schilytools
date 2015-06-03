h26596
s 00001/00001/00057
d D 1.3 15/06/03 00:06:43 joerg 3 2
c ../common/test-common -> ../../common/test-common
e
s 00003/00001/00055
d D 1.2 14/08/26 20:06:34 joerg 2 1
c Zerstoerung der Checksumme hatte auch "V6" -> "V3" gewandelt
e
s 00056/00000/00000
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

# admin-hz.sh:  Tests for the -h and -z options of "admin".

# Import common functions & definitions.
D 3
. ../common/test-common
E 3
I 3
. ../../common/test-common
E 3

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
D 2
docommand c3 " (sed -e '1y/0123456789/9876453210/' <$s >$s2) " 0 "" ""
E 2
I 2
# If we are testing SCCS v6, we need to repair the V6 header
# as the first sed command replaces V6 by V3.
docommand c3 " (sed -e '1y/0123456789/9876453210/' <$s | sed -e '1s/V3/V6/' >$s2) " 0 "" ""
E 2

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
E 1
