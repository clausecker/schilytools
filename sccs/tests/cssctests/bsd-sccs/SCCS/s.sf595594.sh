h10090
s 00001/00001/00108
d D 1.6 20/04/16 01:57:21 joerg 6 5
c %Z% Neu um "No id keywords" Warnung zu vermeiden
e
s 00002/00002/00107
d D 1.5 15/06/03 00:06:43 joerg 5 4
c ../common/test-common -> ../../common/test-common
e
s 00013/00003/00096
d D 1.4 15/04/25 18:43:53 joerg 4 3
c test -w -> wtest -w ... wtest ist eine Funktion mit ls -l | grep
e
s 00001/00001/00098
d D 1.3 15/01/28 20:07:29 joerg 3 2
c setup vor cleanup rufen, damit rmdir: directory "SCCS": Directory not empty nicht kommt
e
s 00026/00000/00073
d D 1.2 11/05/30 19:30:32 joerg 2 1
c setup()/restore() neu um SCCS nach XSCCS und zurueck zu wandeln
e
s 00073/00000/00000
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
#
# This is a test for SourceForge Bug ID 595594, reported by Joel Young.
# This is where "sccs get SCCS" where there are three files (a, b, c) in the
# SCCS difrectory stops processing at b, because a writable version of 
# b exists.  In fact it should carry on a check out a copy of c.

D 5
. ../common/test-common
E 5
I 5
. ../../common/test-common
E 5
I 4

# The test suite fails if you run it as root, particularly because
# "test -w foo" returns 0 if you are root, even if foo is a readonly
# file. We try to avoid this by calling the "wtest" function instead
# of just "test".
# Please don't run the test suite as root, because it may spuriously
# fail.
E 4
D 5
. ../common/not-root
E 5
I 5
. ../../common/not-root
E 5


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


I 2
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

E 2
cleanup () {
    if [ -d SCCS ] 
    then
	( cd SCCS && for i in $files; do rm -f [spzd].$i; done )
	rm -f $files
	rmdir SCCS
    fi
    rm -f $files
}

I 3
setup
E 3
cleanup
remove command.log log log.stdout log.stderr 
I 2
D 3
setup
E 3
E 2
mkdir SCCS

echo "Creating the input files..."
for i in $files
do
D 6
    echo "This is file $i" > $i
E 6
I 6
    echo "This is file $i %Z%" > $i	# Avoid "No id keywords" warning
E 6
    ${sccs} enter $i
    rm ,$i
done


docommand e1 "${sccs} edit b" 0 IGNORE IGNORE
D 4
docommand e2 "test -w b" 0 "" ""
E 4
I 4
#docommand e2 "test -w b" 0 "" ""
docommand e2 "wtest -w b" 0 "" ""
E 4
docommand e3 "${vg_sccs} get SCCS" 1 IGNORE IGNORE

# At this point, a read-only copy of a and c should exist.
# b should still be writable. 

for i in a c 
do
    docommand e4${i}1 "test -f $i" 0 "" ""
D 4
    docommand e4${i}2 "test -w $i" 1 "" ""
E 4
I 4
#    docommand e4${i}2 "test -w $i" 1 "" ""
    docommand e4${i}2 "wtest -w $i" 1 "" ""
E 4
done

D 4
docommand e5 "test -w b" 0 "" ""
E 4
I 4
#docommand e5 "test -w b" 0 "" ""
docommand e5 "wtest -w b" 0 "" ""
E 4

cleanup
I 2
restore
E 2
success


    
E 1
