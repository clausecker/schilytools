h62673
s 00029/00000/00000
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

# default.sh: test the default behaviour.

# Import common functions & definitions.
. ../common/test-common

cleanup () {
    for prefix in s p z l
    do
	remove ${prefix}.foo
    done
    remove command.log foo
}

cleanup

# Create files
docommand dprs1 "${admin} -i/dev/null -n s.foo" 0 "" IGNORE
docommand dprs2 "${get} -e s.foo" 0 IGNORE IGNORE
docommand dprs3 "${delta} -yNone s.foo" 0 IGNORE IGNORE

# With -d, processing stops after the first match.
docommand dprs4 "${vg_prs} -d':M:-:I:\n' s.foo" 0 "foo-1.2\n" ""

# Without -d, by default processing includes all deltas.
docommand dprs5 "${vg_prs} s.foo | grep -c '^D'" 0 "2\n" IGNORE

cleanup
E 1
