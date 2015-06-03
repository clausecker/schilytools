h35344
s 00002/00002/00008
d D 1.2 15/06/03 00:06:45 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00010/00000/00000
d D 1.1 11/05/29 20:11:52 joerg 1 0
c date and time created 11/05/29 20:11:52 by joerg
e
u
U
f e 0
t
T
I 1
#! /bin/sh

# Basic tests for extended SCCS: do not test as root!

# Read test core functions
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

echo_nonl "root..."
D 2
. ../common/not-root
E 2
I 2
. ../../common/not-root
E 2
echo "passed"
E 1
