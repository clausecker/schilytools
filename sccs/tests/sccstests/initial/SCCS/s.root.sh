hV6,sum=42198
s 00002/00002/00008
d D 1.2 2015/06/03 00:06:45+0200 joerg 2 1
S s 14728
c ../common/test-common -> ../../common/test-common
e
s 00010/00000/00000
d D 1.1 2011/05/29 20:11:52+0200 joerg 1 0
S s 14450
c date and time created 11/05/29 20:11:52 by joerg
e
u
U
f e 0
G r 0e46e8b627eed
G p sccs/tests/sccstests/initial/root.sh
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
