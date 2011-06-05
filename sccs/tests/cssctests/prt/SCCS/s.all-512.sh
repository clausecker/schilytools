h63969
s 00026/00000/00000
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
. ../common/test-common
. ../common/need-prt
export prt


remove s.testfile2
../../testutils/uu_decode --decode < s.testfile2.uue || miscarry could not uudecode testfile2.uue.

sh all-variations.txt 2>&1 >got.stdout | 
    grep -v "feature not fully tested: excluded delta"

remove all.expected

/bin/sh ../../testutils/decompress_stdin.sh <all.expected.Z >all.expected \
    || miscarry could not decompress expected output 

if diff all.expected got.stdout >/dev/null 
then
    remove all.expected s.testfile2
    success
else
    echo "output differs --"
    diff -c all.expected got.stdout | head -30
    fail output differs
fi
E 1
