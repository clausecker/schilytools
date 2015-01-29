h18174
s 00010/00008/00061
d D 1.2 15/01/28 20:08:03 joerg 2 1
c /tmp/SCCS -> /tmp/sccstest.$$/SCCS
e
s 00069/00000/00000
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
# This is a test for a bug in sccs.c where the command 
D 2
#   sccs unedit /tmp/SCCS/s.foo
E 2
I 2
#   sccs unedit /tmp/sccstest.$$/SCCS/s.foo
E 2
# causes the deletion of s.foo (instead, the file ./foo should be deleted).

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

# We assume that all the files we want to work on are in the current
# directory (unless we specify a full path, which in fact we do).

unset PROJECTDIR

echo "Using the driver program ${sccs}"

files="foo"
sfiles="s.foo"


cleanup () {
D 2
    if [ -d /tmp/SCCS ] 
E 2
I 2
    if [ -d /tmp/sccstest.$$/SCCS ] 
E 2
    then
D 2
	for i in $files; do /bin/rm -f /tmp/SCCS/[spzd].$i $i; done
	rmdir /tmp/SCCS
E 2
I 2
	for i in $files; do /bin/rm -f /tmp/sccstest.$$/SCCS/[spzd].$i $i; done
	rmdir /tmp/sccstest.$$/SCCS
	rmdir /tmp/sccstest.$$/
E 2
    fi
    rm -f $files $sfiles
}

cleanup
remove command.log log log.stdout log.stderr 
D 2
mkdir /tmp/SCCS
E 2
I 2
mkdir /tmp/sccstest.$$
mkdir /tmp/sccstest.$$/SCCS
E 2

echo "Creating the input files..."
D 2
${admin} -n /tmp/SCCS/s.foo
E 2
I 2
${admin} -n /tmp/sccstest.$$/SCCS/s.foo
E 2
${admin} -n s.foo

docommand d1 "test -f s.foo" 0 "" IGNORE
D 2
docommand d2 "${vg_sccs} edit /tmp/SCCS/s.foo" 0 IGNORE IGNORE
E 2
I 2
docommand d2 "${vg_sccs} edit /tmp/sccstest.$$/SCCS/s.foo" 0 IGNORE IGNORE
E 2
docommand d3 "test -f foo" 0 "" IGNORE

# When we have the bug, this step will probably fail, because the delete
# removes the wrong file, so the subsequent get finds that ./foo exists and 
# is writable, so it fails.
D 2
docommand d4 "${vg_sccs} unedit /tmp/SCCS/s.foo" 0 IGNORE IGNORE
E 2
I 2
docommand d4 "${vg_sccs} unedit /tmp/sccstest.$$/SCCS/s.foo" 0 IGNORE IGNORE
E 2

# This is the heart of the test; make sure sccs.c deleted the right file.
# (the file should have been recreated as read-only).
docommand d5 "test -r foo"   0 "" IGNORE
docommand d5 "test -w foo"   1 "" IGNORE

# make sure we didn't delete the innocent bystander file s.foo.
docommand d6 "test -f s.foo" 0 "" IGNORE

cleanup
success


    
E 1
