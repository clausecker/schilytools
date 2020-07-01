hV6,sum=49022
s 00030/00000/00000
d D 1.1 2010/04/18 18:20:27+0200 joerg 1 0
S s 35059
c date and time created 10/04/18 18:20:27 by joerg
e
u
U
f e 0
f y 
G r 0e46e8eb0ad84
G p sccs/tests/cssctests/all-tests.sh
t
T
I 1
#!/bin/sh

if [ $# -gt 0 ]
then
    srcdir=$1
    shift
else
    srcdir=.
fi
echo Using $srcdir as the srcdir.

set -e

for i in admin delta get prs unget 
do 
    echo ============== Tests for $i ==================
    ret=`pwd`
    cd $i
    for s in ../${srcdir}/${i}/*.sh
    do
	testname=${i}/`basename $s`
	echo ------------ $testname ---------------------
	sh $s
	echo ------------ pass: $testname ---------------
    done

    cd $ret
    echo ============== PASS: $i ==================
done
exit 0
E 1
