h29583
s 00001/00001/00099
d D 1.2 15/06/03 00:06:44 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00100/00000/00000
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
# MRs.sh:  Testing for MR numbers.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

rm -rf test
remove command.log log log.stdout log.stderr base
remove passwd 
mkdir test 2>/dev/null

# Create the input files.
cat > base <<EOF
%M%: This is a test file containing nothing interesting.
EOF
for i in 1 2 3 4 5 6
do 
    cat base                       > test/passwd.$i
    echo "This is file number" $i >> test/passwd.$i
done 
remove base test/[xz].* passwd test/[spx].passwd


# Create an SCCS file to work on.
docommand M1 "${admin} -itest/passwd.1 test/s.passwd" 0 "" ""

# Check the file out.
docommand M2 "${get} -e test/s.passwd" 0 "1.1\nnew delta 1.2\n2 lines\n" ""

# Check that the MR validation flag is OFF.
docommand M3 "${prs} -d:MF: test/s.passwd" 0 "no\n" ""

# Check that the MR validation program is unset.
docommand M4 "${prs} -d:MP: test/s.passwd" 0 "none\n" ""

# Set the MR validator to "true".
docommand M5 "${admin} -fvtrue test/s.passwd" 0 "" ""

# Check that the MR validation flag is ON.
docommand M6 "${prs} -d:MF: test/s.passwd" 0 "yes\n" ""

# Check that the MR validation program is set.
docommand M7 "${prs} -d:MP: test/s.passwd" 0 "true\n" ""

# Check in a file, giving an MR.
docommand M8 "${delta} -mmr.XYZZY -ycomment.XYZZY test/s.passwd" 0 \
    "1.2\n0 inserted\n0 deleted\n2 unchanged\n" ""

# Check that the MR for the delta we just did appears in the sccs-prs listing.
docommand M9 "${prs} -r1.2 test/s.passwd |  \
            sed -ne '/^MRs:$/,/^COMMENTS:$/ p'" 0 \
            "MRs:\nmr.XYZZY\nCOMMENTS:\n" ""

# Check the file out again.
docommand M10 "${get} -e test/s.passwd" 0 "1.2\nnew delta 1.3\n2 lines\n" ""

# Set the MR validator to "false".
docommand M11 "${admin} -fv/bin/false test/s.passwd" 0 "" ""

# Make sure that the MR validation fails, and delta fails.
docommand M12 "${delta} -mmr.M12 -ycomment.M12 test/s.passwd" 1 "1.3\n" IGNORE

# Unset the validation flag and make sure we can delta afterward.
docommand M13 "${admin} -dv test/s.passwd" 0 "" ""

# Make sure that the delta now succeeds after we have removed the MR flag.
docommand M14 "${delta} -ycomment.M14 test/s.passwd" 0 \
    "1.3\n0 inserted\n0 deleted\n2 unchanged\n" ""

# Check the file out again.
docommand M15 "${get} -e test/s.passwd" 0 "1.3\nnew delta 1.4\n2 lines\n" ""

# Now that MR validation is turned off, we should not be able to specify MRs.
docommand M16 "${delta} -mmr.M16 -ycomment.M16 test/s.passwd" 1 "1.4\n" IGNORE

# If the MR flag is on but has no value ...
docommand M17 "${admin} -fv test/s.passwd" 0 "" ""

# Check that the MR validation flag is ON.
docommand M18 "${prs} -d:MF: test/s.passwd" 0 "yes\n" ""

# Check that the MR validation program is blank.
docommand M19 "${prs} -d:MP: test/s.passwd" 0 "\n" ""

# Hence MRs should be accepted without checking.
docommand M20 "${delta} -mmr.M20 -ycomment.M20 test/s.passwd" 0 \
        "1.4\n0 inserted\n0 deleted\n2 unchanged\n"  ""

# Check the file out again.
docommand M21 "${get} -e test/s.passwd" 0 "1.4\nnew delta 1.5\n2 lines\n" ""

# Check the file out again.  Require MRs.  Try to check in without MRs
docommand M22 "${admin} -fv'test -n' test/s.passwd" 0 "" ""
docommand M23 "${delta} -ycomment.M23 test/s.passwd </dev/null" 1 IGNORE  IGNORE
docommand M24 "test -f test/p.passwd" 0 "" ""

rm -rf test
remove command.log passwd 

success
E 1
