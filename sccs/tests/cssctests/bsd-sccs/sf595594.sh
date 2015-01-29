#! /bin/sh
#
# This is a test for SourceForge Bug ID 595594, reported by Joel Young.
# This is where "sccs get SCCS" where there are three files (a, b, c) in the
# SCCS difrectory stops processing at b, because a writable version of 
# b exists.  In fact it should carry on a check out a copy of c.

. ../common/test-common
. ../common/not-root


# If LANG is defined but the system is misconfigured, we will produce
# the error message "Error setting locale: No such file or directory".
# If that happens, the test suite will fail.  For this reason, we
# unset the LANG environment variable.  Of course, things being
# printed out in the wrong language would also mess up the results of
# the test suite.
# We want to prevent setlocale(LC_ALL, "") failing:
unset LANG

# We assume that all the files we want to work on are in the 
# current directory.
unset PROJECTDIR

echo "Using the driver program ${sccs}"

files="a b c"
sfiles=" SCCS/s.a SCCS/s.b SCCS/s.c"


setup() {
	if test -f SCCS/s.sf595594.sh
	then
		if test -d XSCCS
		then
			miscarry "Cannot rename SCCS to XSCCS, XSCCS exists"
		else
			mv SCCS XSCCS
		fi
	fi
}

restore() {
	if test -f XSCCS/s.sf595594.sh
	then
		if test -d SCCS
		then
			miscarry "Cannot rename XSCCS back to XSCCS, SCCS exists"
		else
			mv XSCCS SCCS
		fi
	fi
}

cleanup () {
    if [ -d SCCS ] 
    then
	( cd SCCS && for i in $files; do rm -f [spzd].$i; done )
	rm -f $files
	rmdir SCCS
    fi
    rm -f $files
}

setup
cleanup
remove command.log log log.stdout log.stderr 
mkdir SCCS

echo "Creating the input files..."
for i in $files
do
    echo "This is file $i" > $i
    ${sccs} enter $i
    rm ,$i
done


docommand e1 "${sccs} edit b" 0 IGNORE IGNORE
docommand e2 "test -w b" 0 "" ""
docommand e3 "${vg_sccs} get SCCS" 1 IGNORE IGNORE

# At this point, a read-only copy of a and c should exist.
# b should still be writable. 

for i in a c 
do
    docommand e4${i}1 "test -f $i" 0 "" ""
    docommand e4${i}2 "test -w $i" 1 "" ""
done

docommand e5 "test -w b" 0 "" ""

cleanup
restore
success


    
