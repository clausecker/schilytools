h49976
s 00020/00000/00000
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
# root.sh:          You can't run the test suite as root.  Make sure the 
#                   suite aborts early if you do.
#                   

# Import common functions & definitions.
. ../common/test-common

# The test suite fails if you run it as root, particularly because
# "test -w foo" returns 0 if you are root, even if foo is a readonly
# file.
# So please don't run the test suite as root, because it will spuriously
# fail.
true

echo_nonl "r1..."
. ../common/not-root
echo "passed "

success
E 1
