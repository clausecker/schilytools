h14596
s 00001/00001/00064
d D 1.2 15/06/03 00:06:44 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00065/00000/00000
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
# branch.sh:  Tests for making branches.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

remove command.log 

g=brtest
s=s.$g
z=z.$g
x=x.$g
p=p.$g
remove [zxsp].$g $g

# Create the s. file and make sure it exists.
remove $g
echo "%M%" > $g
docommand B1 "$admin -n -i$g $s" 0 "" IGNORE
remove $g

# Try making a branch when the "b" flag is not set.
# This should suceed (really!), but a branch should not be 
# made.
docommand B2 "${vg_get} -e -b $s" 0 "1.1\nnew delta 1.2\n1 lines\n" IGNORE
# When the -b flag is ignored, CSSC emits a warning but SCCS is silent.

# I *think* it's possible that not all versions of SCCS emit anything 
# to stdout for "unget".   At least, that's what I gather from an email
# from Maurice O' Donnell <mod@tfn.com>.
docommand B3 "$unget $s" 0 "1.2\n" ""

# Set the branch flag and make sure everything still works,
# but make sure that the file IS branched this time.
docommand B4 "$admin -fb $s" 0 "" ""
docommand B5 "${vg_get} -e -b $s" 0 "1.1\nnew delta 1.1.1.1\n1 lines\n" ""

# Check the file in to leave the branch in place.
docommand B6 "$delta -yNoComment $s" 0 "1.1.1.1\n0 inserted\n0 deleted\n1 unchanged\n" ""

# Make a branch at the same place, and check the resulting SID.
docommand B7 "${vg_get} -e -b -r1.1 $s" 0 "1.1\nnew delta 1.1.2.1\n1 lines\n" ""
docommand B8 "$delta -yNoComment $s" 0 "1.1.2.1\n0 inserted\n0 deleted\n1 unchanged\n" ""


# Check out a revision on the branch for editing, but don't make a branch there.
docommand B9 "${vg_get} -e -r1.1.1.1 $s" 0 "1.1.1.1\nnew delta 1.1.1.2\n1 lines\n" ""
docommand B10 "$delta -yNoComment $s" 0 "1.1.1.2\n0 inserted\n0 deleted\n1 unchanged\n" ""


# Making a second branch at a location where one already exists will 
# create a second branch; the release number stays the same, but the 
# branch number chosen is one greater than the last time.
docommand B11 "${vg_get} -e -b -r1.1.1.1 $s" 0 "1.1.1.1\nnew delta 1.1.3.1\n1 lines\n" ""
docommand B12 "$delta -yNoComment $s" 0 "1.1.3.1\n0 inserted\n0 deleted\n1 unchanged\n" ""

# Make another branch at 1.1.  We continue just to increase 
# the branch number.   I know this seems a bit silly, but 
# it really is the way SCCS works, and we must be compatible with it.
docommand B13 "${vg_get} -e -b $s" 0 "1.1\nnew delta 1.1.4.1\n1 lines\n" ""


remove [zxsp].$g $g
remove command.log
success
E 1
