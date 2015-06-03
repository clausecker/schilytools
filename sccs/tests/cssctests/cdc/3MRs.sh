#! /bin/sh
# 3MRs.sh:  Testing for MR numbers with the cdc program..

# Import common functions & definitions.
. ../../common/test-common

g=testfile
s=s.$g
z=z.$g
x=x.$g
p=p.$g
files="$s $z $x $p"

remove command.log log log.stdout log.stderr base [sxzp].$g


# Create the input file.
cat > $g <<EOF
%M%: This is a test file containing nothing interesting.
EOF

# Create an SCCS file to work on.
docommand M1 "${admin} -i$g $s" 0 "" ""
remove $g

# Try to offer an MR, check this is rejected.
docommand M2 "${vg_cdc} -r1.1 -mAnMR -yThisShouldFail $s" 1 "" "IGNORE" 


# With stdin already used, make sure that not passing "-y" produces a
# fatal error.
docommand M3 "echo_nonl $s | ${vg_cdc} -r1.1 -" 1 "" IGNORE

# With stdin not used, make sure that comments are accepted via stdin.
docommand M4 "echo Hello | ${vg_cdc} -r1.1 $s" 0 "" ""
docommand M5 "${vg_prs} -d:C: -r1.1 $s | sed -n 1p" 0 "Hello\n" ""

# Make sure that the "-r" option is correctly mandatory.
docommand M6 "${vg_cdc} -yHiThereIfail2 $s" 1 "" "IGNORE"


# Set the MR flag, try cdc without an MR, and make sure this fails.
docommand M7 "${admin} -fvtrue $s" 0 "" ""

# With stdin already used, make sure that not passing "-m" produces a
# fatal error, even if we pass "-y".
# Have to use stdin else cdc will prompt for the MR.
docommand M8 "echo_nonl $s | ${vg_cdc} -r1.1 -yadayada -" 1 "" IGNORE

# With the MR flag still set, try cdc with an MR; make sure it passes.
docommand M9 "${vg_cdc} -r1.1 -yadayada -mMR1 $s" 0 "" "" 


# Check that the MR is added correctly
docommand M10 "${prs} -r1.1 -d:MR: $s" 0 "MR1\n\n" "" 

# Add another MR.   Check the order is correct.
docommand M11 "${vg_cdc} -r1.1 -yahoo '-mMR2 MR3' $s" 0 "" "" 
docommand M12 "${prs} -r1.1 -d:MR: $s" 0 "MR2\nMR3\nMR1\n\n" "" 

# Delete an MR; check the comments indicate this.  Check the MR list 
# no longer contains that MR.
docommand M13 "${vg_cdc} -r1.1 -y '-m!MR1' $s" 0 "" "" 
docommand M14 "${prs} -r1.1 -d:MR: $s" 0 "MR2\nMR3\n\n" "" 

# Make sure that the comments field now indicates that 
# that MR has been removed.
remove comment
${prs} -d:C: -r1.1 $s > comment || fail prs failed unexpectedly


docommand M15 "sed -n 1p <comment" 0 "*** LIST OF DELETED MRS ***\n" ""
docommand M16 "sed -n 2p <comment" 0 "MR1\n" ""
# Check that these removed-MR comments follow immediately
# from the previous comments.
docommand M17 "sed -n 3p <comment" 0 "ahoo\n" ""
remove comment



# Delete a non-existent MR.  Make sure that no error message is produced.
# Also make sure that the file is not changed.
cp $s s.saved || fail cp failed.
docommand M18 "${vg_cdc} -r1.1 -y '-m!MR7' $s" 0 "" "" 
docommand M19 "diff $s s.saved" 0 "" ""
remove s.saved


# With stdin already used, make sure that not passing "-y" produces a
# fatal error, even if we pass "-m".  The precise format of the error
# message is not important.
docommand M20 "echo_nonl $s | ${vg_cdc} -r1.1 -mMR4 -" 1 "" IGNORE


# Add and removee an MR in the same operation; make sure that the
# correct one takes precedence.
docommand M21 "${vg_cdc} -r1.1 -y '-m!MR5 MR5' $s" 0 "" IGNORE
docommand M22 "${prs} -r1.1 -d:MR: $s" 0 "MR5\nMR2\nMR3\n\n" "" 
# Same but in the other order; the result should not change.
docommand M23 "${vg_cdc} -r1.1 -y '-mMR5 !MR5' $s" 0 "" IGNORE
docommand M24 "${prs} -r1.1 -d:MR: $s" 0 "MR5\nMR2\nMR3\n\n" "" 


remove command.log passwd $s $p $g $z $x
success
