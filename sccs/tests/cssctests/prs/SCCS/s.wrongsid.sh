h12451
s 00033/00000/00000
d D 1.1 11/04/26 03:04:16 joerg 1 0
c date and time created 11/04/26 03:04:16 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

# wrongsid.sh:  what happens if the desired SID does not exist?

# Import common functions & definitions.
. ../common/test-common

cleanup () {
    for prefix in s p z l
    do
	remove ${prefix}.foo ${prefix}.bar
    done
    remove command.log
}

cleanup

# Create files
docommand ws1 "${admin} -i/dev/null -r5.1 -n s.foo" 0 "" IGNORE
docommand ws2 "${admin} -i/dev/null -r10.1 -n s.bar" 0 "" IGNORE


# Basic successful operations.
docommand ws3 "${vg_prs} -r5.1 -d':M:-:I:\nX' s.foo" 0 "foo-5.1\nX\n" ""
docommand ws4 "${vg_prs} -r10.1 -d':M:-:I:\nX' s.bar" 0 "bar-10.1\nX\n" ""

# Similar but specifying a nonexistent SID
docommand ws5 "${vg_prs} -r10.1 -d':M:-:I:\nX' s.foo" 1 "" IGNORE
# This time, make sure the failure of the first command is remembered.
docommand ws6 "${vg_prs} -r10.1 -d':M:-:I:' s.foo s.bar" 1 "bar-10.1\n" IGNORE

cleanup
success
E 1
