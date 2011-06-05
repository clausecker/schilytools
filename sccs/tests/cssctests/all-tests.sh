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
