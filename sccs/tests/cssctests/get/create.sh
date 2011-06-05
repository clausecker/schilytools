#! /bin/sh
# create.sh:  Testing for "get -e" -- the making of new deltas.

# Import common functions & definitions.
. ../common/test-common

remove command.log log log.stdout log.stderr passwd
mkdir test 2>/dev/null

# Create the input files.
cat > base <<EOF
This is a test file containing nothing interesting.
EOF
for i in 1 2 3 4 5 6
do 
    cat base > test/passwd.$i || miscarry "could not create test/passwd.$i"
    echo "This is file number" $i >> test/passwd.$i  \
	|| miscarry "could not append data to test/passwd.$i"
done 
remove base test/[xz].*
remove test/[spx].passwd


#
# Create an SCCS file with several branches to work on.
# We generally ignore stderr output since we produce "Warning: no id keywords"
# more often than "real" SCCS.
#
docommand L1 "${admin} -itest/passwd.1 test/s.passwd" 0 "" IGNORE
docommand L1a "${vg_get} -p test/s.passwd" 0 \
"This is a test file containing nothing interesting.
This is file number 1
" IGNORE

docommand L2 "${vg_get} -e -g test/s.passwd"             0 "1.1\nnew delta 1.2\n" IGNORE

cp test/passwd.2 passwd
docommand L3 "${delta} -y\"\" test/s.passwd" 0 "1.2\n1 inserted\n1 deleted\n1 unchanged\n" IGNORE
docommand L3a "${vg_get} -p test/s.passwd" 0 \
"This is a test file containing nothing interesting.
This is file number 2
" IGNORE

docommand L4 "${vg_get} -e -g test/s.passwd"  0 "1.2\nnew delta 1.3\n" IGNORE
cp test/passwd.3 passwd
docommand L5 "${delta} -y'' test/s.passwd" 0 "1.3\n1 inserted\n1 deleted\n1 unchanged\n" IGNORE
docommand L5a "${vg_get} -p test/s.passwd" 0 \
"This is a test file containing nothing interesting.
This is file number 3
" IGNORE


docommand L5b "${vg_get} -p -x1.2 test/s.passwd" 0 \
"This is a test file containing nothing interesting.
This is file number 1
This is file number 3
" IGNORE


docommand L6 "${vg_get} -e -g -x1.2 test/s.passwd" 0 "Excluded:\n1.2\n1.3\nnew delta 1.4\n" IGNORE
cp test/passwd.4 passwd
docommand L7 "${delta} -y'' test/s.passwd" 0 "1.4\n1 inserted\n2 deleted\n1 unchanged\n" IGNORE
docommand L7a "${vg_get} -p test/s.passwd" 0 \
"This is a test file containing nothing interesting.
This is file number 4
" IGNORE


docommand L8 "${vg_get} -e -g test/s.passwd" 0 "1.4\nnew delta 1.5\n" IGNORE
cp test/passwd.5 passwd
docommand L9 "${delta} -y'' test/s.passwd" 0 "1.5\n1 inserted\n1 deleted\n1 unchanged\n" IGNORE

docommand L9a "${vg_get} -p test/s.passwd" 0 \
"This is a test file containing nothing interesting.
This is file number 5
" IGNORE


docommand L10 "${vg_get} -e -g -r1.3 test/s.passwd" 0 "1.3\nnew delta 1.3.1.1\n" IGNORE
cp test/passwd.6 passwd
docommand L11 "${delta} -y'' test/s.passwd" 0 \
    "1.3.1.1\n1 inserted\n1 deleted\n1 unchanged\n" IGNORE
docommand L11a "${vg_get} -p test/s.passwd" 0 \
"This is a test file containing nothing interesting.
This is file number 5
" IGNORE


do_output L12 "${vg_get} -r1.1 -p test/s.passwd"      0 test/passwd.1 IGNORE
do_output L13 "${vg_get} -r1.2 -p test/s.passwd"      0 test/passwd.2 IGNORE
do_output L14 "${vg_get} -r1.3 -p test/s.passwd"      0 test/passwd.3 IGNORE
do_output L15 "${vg_get} -r1.4 -p test/s.passwd"      0 test/passwd.4 IGNORE
do_output L16 "${vg_get} -r1.5 -p test/s.passwd"      0 test/passwd.5 IGNORE
do_output L17 "${vg_get} -r1.3.1.1 -p test/s.passwd"  0 test/passwd.6 IGNORE

rm -rf test
remove passwd command.log
success
