#! /bin/sh
# included.sh:  Extra tests using the test file from bug number 111140.

# Import common functions & definitions.
. ../common/test-common


g=sf111140_testcase.txt
s=s.$g
x=x.$g 
z=z.$g
p=p.$g
u=sf111140_testcase.uue

remove $g $s $x $z $p

../../testutils/uu_decode --decode <$u || 
    miscarry could not uudecode file $u.





# If we check out version 1.16 of the provided file (in which 
# a trunk delta includes a delta that was on a trunk) we 
# should get the same body as recorded in the file sf111140.wtd.
# 

do_pair() {
    seq="$1"
    sid="$2"

    if awk "\$1 == $seq {print}" <  sf111140_full.txt | 
	sed 's/^[0-9]* //'> wanted.tmp
    then
	do_output f${seq} "${vg_get} -r${sid} -p $s" 0 wanted.tmp IGNORE
    else
	miscarry "awk failed"
    fi
}


record_pair() {
    seq="$1"
    sid="$2"

    echo_nonl "${seq} " >&2
    ${vg_get} -p -r${sid} ${s} 2>/dev/null | 
    awk "{printf(\"%s %s\n\", $seq, \$0);}" 
}


do_output s1 "${vg_get} -r1.16 -p $s"      0 sf111140.wtd IGNORE

# echo_nonl "Preparing test file... "
# (
# record_pair 1 1.1
# record_pair 3 1.2
# record_pair 4 1.3
# record_pair 5 1.4
# record_pair 6 1.5
# record_pair 7 1.6
# record_pair 8 1.7
# record_pair 9 1.8
# record_pair 10 1.9
# record_pair 12 1.10
# record_pair 13 1.9.1.1
# record_pair 14 1.11
# record_pair 15 1.12
# record_pair 16 1.13
# record_pair 17 1.14
# record_pair 18 1.14.1.1
# record_pair 19 1.14.1.2
# record_pair 20 1.14.1.3
# record_pair 21 1.14.2.1
# record_pair 22 1.15
# record_pair 23 1.14.2.2
# record_pair 24 1.14.1.4
# record_pair 25 1.14.1.5
# record_pair 26 1.16
# record_pair 27 1.17
# record_pair 28 1.18
# record_pair 29 1.19
# record_pair 30 1.18.1.1
# record_pair 31 1.19.1.1
# record_pair 32 1.18.2.1
# record_pair 33 1.20
# record_pair 34 1.21
# record_pair 35 1.22
# record_pair 36 1.23
# ) > sf111140_full.txt
# echo done


do_pair 1 1.1
do_pair 3 1.2
do_pair 4 1.3
do_pair 5 1.4
do_pair 6 1.5
do_pair 7 1.6
do_pair 8 1.7
do_pair 9 1.8
do_pair 10 1.9
do_pair 12 1.10
do_pair 13 1.9.1.1
do_pair 14 1.11
do_pair 15 1.12
do_pair 16 1.13
do_pair 17 1.14
do_pair 18 1.14.1.1
do_pair 19 1.14.1.2
do_pair 20 1.14.1.3
do_pair 21 1.14.2.1
do_pair 22 1.15
do_pair 23 1.14.2.2
do_pair 24 1.14.1.4
do_pair 25 1.14.1.5
do_pair 26 1.16
do_pair 27 1.17
do_pair 28 1.18
do_pair 29 1.19
do_pair 30 1.18.1.1
do_pair 31 1.19.1.1
do_pair 32 1.18.2.1
do_pair 33 1.20
do_pair 34 1.21
do_pair 35 1.22
do_pair 36 1.23

remove $g $s $x $z $p wanted.tmp
success
