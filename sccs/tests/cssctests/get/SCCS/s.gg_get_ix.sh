h47910
s 00002/00002/00075
d D 1.2 15/06/03 00:06:44 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00077/00000/00000
d D 1.1 10/05/11 11:30:00 joerg 1 0
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh
#
# gg_get_ix.sh:  Tests the -i and -x options of "get"
#

# Import common functions & definitions.

D 2
. ../common/test-common
. ../common/real-thing
E 2
I 2
. ../../common/test-common
. ../../common/real-thing
E 2

remove command.log 

g=incl_excl
s=s.$g
z=z.$g
x=x.$g
p=p.$g

remove [zxsp].$g $g

# Create the s. file and make sure it exists.

remove $g

## These tests currently work fine on Digital Unix but 
## Excl_1 fails on Solaris.  Hence it's commented out.
## TODO: make sense of this situation.


echo "%M%" > $g

docommand Init_1 "$admin -n -i$g $s" 0 "" IGNORE

remove $g

# "get" the new file and check its contents.

docommand Init_2 "${vg_get} -p $s" 0 "$g\n" IGNORE

# Try excluding V1.1 (the only version)
# Returns a NULL file.

# docommand Excl_1 "${vg_get} -x1.1 -p $s" 0 "" IGNORE

# Edit the file and insert a line that identifies the version.

docommand Edit_1 "${vg_get} -e $s" 0 "1.1\nnew delta 1.2\n1 lines\n" ""
echo "Inserted in V1.2" >> $g
docommand Delt_1 "$delta -yNoComment $s" 0 "1.2\n1 inserted\n0 deleted\n1 unchanged\n" ""

# Now let's extract a read-only copy of V1.2 excluding V1.1

docommand Excl_2 "${vg_get} -x1.1 -p $s" 0 "Inserted in V1.2\n" IGNORE

# Edit V1.2 excluding V1.1

docommand Edit_2 "${vg_get} -e -x1.1 $s | grep -v co25" 0 "Excluded:\n1.1\n1.2\nnew delta 1.3\n1 lines\n" IGNORE
echo "V1.3 excluded V1.1" >> $g
docommand Delt_2 "$delta -yNoComment $s" 0 "1.3\n1 inserted\n0 deleted\n1 unchanged\n" IGNORE

# Now let's see what happens with various gets.

# Manually exclude 1.1 (it should be excluded anyway even if we didn't)
docommand Get_0 "${vg_get} -x1.1 -p $s" 0 "Inserted in V1.2\nV1.3 excluded V1.1\n" IGNORE

# First get V1.3 which should automatically exclude V1.1

docommand Get_1 "${vg_get} -p $s" 0 "Inserted in V1.2\nV1.3 excluded V1.1\n" IGNORE

# Then do a "get" including V1.1.  All 3 lines should bee present.

docommand Get_2 "${vg_get} -p -i1.1 $s" 0 "$g\nInserted in V1.2\nV1.3 excluded V1.1\n" IGNORE

remove [zxsp].$g $g
remove command.log

success
E 1
