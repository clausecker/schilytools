#! /bin/sh

# flags.sh:  Testing for setting /unsetting flags.

# Import common functions & definitions.
. ../../common/test-common

# Import function which tells us if we're testing CSSC, or something else.
. ../../common/real-thing

# $TESTING_SCCS_V5	Test SCCSv5 features from SunOS
# $TESTING_CSSC		Relict from CSSC tests, also applies to SCCS
# $TESTING_REAL_CSSC	Test real CSSC 4-digit year extensions
# $TESTING_REAL_SCCS	Test real Schily SCCS 4 digit year extensions
# $TESTING_SCCS_V6	Test SCCSv6 features

g=new.txt
s=s.$g
p=p.$g
remove foo $s $g $p [zx].$g

###
### Tests for the 'v' flag; see also init-mrs.sh.
###

# Create SCCS file with a substituted keyword.
echo '%M%' >foo
docommand v1 "${admin} -ifoo $s" 0 "" ""

# Check that the MR validation flag is OFF.
docommand v2 "${vg_prs} -d:MF: $s" 0 "no\n" ""

# Check that the MR validation program is unset.
docommand v3 "${vg_prs} -d:MP: $s" 0 "none\n" ""

## Create and specify MR numbers...

# Create with no MR
remove $s
docommand v4 "${vg_admin} -fv -m' ' -r2 -ifoo $s" 0 "" ""
remove $s

# Set MR flag -- should work.
remove $s
docommand v5 "${vg_admin} -fv -mI13 -ifoo $s" 0 "" IGNORE

# Install MR validating program (setting & getting the 
# name of the MR validator)
docommand v6 "${vg_admin} -fvtrue $s" 0 "" IGNORE

# Make sure validation checks can succeed, ever.
remove $s
docommand v7 "${vg_admin} -fvtrue -mI19 -ifoo $s" 0 "" ""

remove $s $g



###
### Tests for the 'b' flag
###
docommand b1 "${admin} -ifoo $s" 0 "" ""

# By default branch creation should fail, and we just get a delta
# further down the trunk -- the invocation does not  fail,
# we just don't get a branch.   In this situation, CSSC emits a warning
# to indicate to the user why their apparent intention has not been 
# carried out.
docommand b2 "${vg_get} -e -b -r1.1 $s" 0 "1.1\nnew delta 1.2\n1 lines\n" IGNORE

docommand b3 "${unget} $s" 0 "1.2\n" ""

# Turn on the enable-branches flag.
docommand b4 "${vg_admin} -fb $s" 0 "" ""

# Create a branch.
docommand b5 "${vg_get} -e -b -r1.1 $s" 0 "1.1\nnew delta 1.1.1.1\n1 lines\n" \
        IGNORE
docommand b6 "${unget} $s" 0 "1.1.1.1\n" ""

remove $s $g $p


###
### Tests for the n flag.
### 
# Create an SCCS file with the "n" flag turned on.
docommand b7 "${admin} -ifoo $s" 0 "" ""
docommand b8 "${vg_admin} -fn $s" 0 "" ""

# Skip a release (2)
docommand b9 "${vg_get} -e -r3 $s" 0 "1.1\nnew delta 3.1\n1 lines\n" \
            IGNORE
echo "hello" >> $g
docommand b10 "${vg_delta} -y $s" 0 "3.1\n1 inserted\n0 deleted\n1 unchanged\n" IGNORE
   
# Check that a null delta was made for release 2, at all.
docommand b11 "${prs} -r2.1 -d:I: $s" 0 "2.1\n" IGNORE

# Check some details about that release.  The comment is 
# "AUTO NULL DELTA", no deltas were included or excluded;
# one delta was ignored; the predecessor sequence number must be 1; 
# the sequence number of this delta must be 2, and the type must be 'D',
# that is, a normal delta.
if $TESTING_REAL_SCCS
then
docommand b12 "${vg_prs} -r2.1 '-d:C:|:DI:|:DP:|:DS:|:DT:' $s" 0 \
  'AUTO NULL DELTA\n|//|1|2|D\n' IGNORE
else
#
# Not POSIX compliant for -d:DI
# This applies to AT&T SCCS past 1984 and to CSSC
#
docommand b12-nonPOSIX "${vg_prs} -r2.1 '-d:C:|:DI:|:DP:|:DS:|:DT:' $s" 0 \
  'AUTO NULL DELTA\n||1|2|D\n' IGNORE
fi

###
### Tests for the d flag
###
remove $s
docommand d1 "${vg_admin} -n -fd1.1 $s" 0 "" ""
docommand d2 "${vg_prs} -d:FL: $s" 0 "default SID\t1.1\n\n" ""
remove $s



###
### Cleanup and exit.
###
rm -rf test 
remove foo $s $g $p [zx].$g command.log

success
