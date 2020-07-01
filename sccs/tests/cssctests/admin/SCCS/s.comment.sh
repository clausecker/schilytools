hV6,sum=18996
s 00001/00000/00110
d D 1.7 2019/05/12 14:28:12+0200 joerg 7 6
S s 54010
c Neuer Hinweis: :DI: Bug wurde 1984 surch AT&T eingefuehrt
e
s 00024/00000/00086
d D 1.6 2019/05/12 14:12:22+0200 joerg 6 5
S s 50201
c prs -D:DI: unterscheidet nun fuer non-POSIX Varianten
c Kommentar verbessert
e
s 00002/00002/00084
d D 1.5 2015/06/03 00:06:43+0200 joerg 5 4
S s 64482
c ../common/test-common -> ../../common/test-common
e
s 00011/00001/00075
d D 1.4 2014/08/26 20:11:31+0200 joerg 4 3
S s 64204
c Anpassung an erweitertes Datumsformat von SCCSv6
e
s 00001/00001/00075
d D 1.3 2011/10/21 23:07:38+0200 joerg 3 2
S s 38692
c prs -d:DI: Tests sind nun POSIX konform
e
s 00003/00003/00073
d D 1.2 2011/05/30 01:13:10+0200 joerg 2 1
S s 38598
c sed -ne '/^COMMENTS:$/,/$/ p' -> sed -ne '/^COMMENTS:$/,/^.*$/ p'
e
s 00076/00000/00000
d D 1.1 2010/04/29 02:05:14+0200 joerg 1 0
S s 38052
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
G r 0e46e8eb9e04f
G p sccs/tests/cssctests/admin/comment.sh
t
T
I 1
#! /bin/sh

# comment.sh:  Testing for comments at initialisation time.

# Import common functions & definitions.
D 5
. ../common/test-common
E 5
I 5
. ../../common/test-common
E 5

I 4
# Import function which tells us if we're testing CSSC, or something else.
D 5
. ../common/real-thing
E 5
I 5
. ../../common/real-thing
E 5

I 6
# $TESTING_SCCS_V5	Test SCCSv5 features from SunOS
# $TESTING_CSSC		Relict from CSSC tests, also applies to SCCS
# $TESTING_REAL_CSSC	Test real CSSC 4-digit year extensions
# $TESTING_REAL_SCCS	Test real Schily SCCS 4 digit year extensions
# $TESTING_SCCS_V6	Test SCCSv6 features

E 6
E 4
s=s.new.txt
remove foo new.txt [xzs].new.txt [xzs].1 [xzs].2 command.log


remove foo
echo '%M%' > foo
test `cat foo` = '%M%' || miscarry cannot create file foo.

# Create an empty SCCS file to work on.
docommand C1 "${admin} -ifoo $s" 0 "" ""

# Check the format of the default comment.
echo_nonl C2...
remove prs.$s
D 2
${vg_prs} $s | sed -ne '/^COMMENTS:$/,/$/ p' > prs.$s || fail prs failed.
E 2
I 2
${vg_prs} $s | sed -ne '/^COMMENTS:$/,/^.*$/ p' > prs.$s || fail prs failed.
E 2
test `wc -l < prs.$s` -eq 2 || fail wrong comment format.
test `head -1 prs.$s` = "COMMENTS:" || fail Comment doesn\'t start COMMENTS:
D 4
tail -1 prs.$s | egrep \
E 4
I 4
if $TESTING_SCCS_V6
then
 tail -1 prs.$s | egrep \
 '^date and time created [0-9][0-9][0-9][0-9]/[0-1][0-9]/[0-3][0-9] [0-2][0-9]:[0-5][0-9]:[0-5][0-9].* by ' >/dev/null\
    || fail "default message format error."
else
 tail -1 prs.$s | egrep \
E 4
 '^date and time created [0-9][0-9]/[0-1][0-9]/[0-3][0-9] [0-2][0-9]:[0-5][0-9]:[0-5][0-9] by ' >/dev/null\
    || fail "default message format error."
I 4
fi
E 4
echo passed
remove $s prs.$s 

# Force a blank comment and check it was blank.
docommand C3 "${admin} -ifoo -y $s" 0 "" ""
docommand C4 "${vg_prs} $s | \
D 2
	    sed -ne '/^COMMENTS:$/,/$/ p'"   0  \
E 2
I 2
	    sed -ne '/^COMMENTS:$/,/^.*$/ p'"   0  \
E 2
	    "COMMENTS:\n\n" ""
remove $s


# Specify some comment and check it works.
docommand C5 "${admin} -ifoo -yMyComment $s" 0 "" ""
docommand C6 "${vg_prs} $s | \
D 2
	    sed -ne '/^COMMENTS:$/,/$/ p'"   0  \
E 2
I 2
	    sed -ne '/^COMMENTS:$/,/^.*$/ p'"   0  \
E 2
	    "COMMENTS:\nMyComment\n" ""

# Detach the comment arg and check it no longer works.
remove MyComment $s
docommand C7 "${vg_admin} -y MyComment -n $s" 1 "" IGNORE

# Ensure the same form does work normally.
remove MyComment $s
docommand C8 "${vg_admin} -n -yMyComment $s" 0 "" IGNORE


# Can we create multiple files if we don't use -i ?
docommand C9 "${vg_admin} -n s.1 s.2" 0 "" ""

I 6
if $TESTING_REAL_SCCS
then

E 6
# Check both generated files.
for n in 1 2
do
    stage=C`expr 9 + $n`
    docommand $stage "${prs} \
  -d':B:\n:BF:\n:DI:\n:DL:\n:DT:\n:I:\n:J:\n:LK:\n:MF:\n:MP:\n:MR:\n:Z:' s.1" \
  0                                                                           \
D 3
  "\nno\n\n00000/00000/00000\nD\n1.1\nno\nnone\nno\nnone\n\n@(#)\n"       \
E 3
I 3
  "\nno\n//\n00000/00000/00000\nD\n1.1\nno\nnone\nno\nnone\n\n@(#)\n"       \
E 3
  ""
done

I 6
else

# Not POSIX compliant for -d:DI
I 7
# This applies to AT&T SCCS past 1984 and to CSSC
E 7
# Check both generated files.
for n in 1 2
do
    stage=C`expr 9 + $n`
    docommand $stage-nonPOSIX "${prs} \
  -d':B:\n:BF:\n:DI:\n:DL:\n:DT:\n:I:\n:J:\n:LK:\n:MF:\n:MP:\n:MR:\n:Z:' s.1" \
  0                                                                           \
  "\nno\n\n00000/00000/00000\nD\n1.1\nno\nnone\nno\nnone\n\n@(#)\n"       \
  ""
done

fi
E 6
docommand C12 "${vg_prs} -d':M:\n' s.1 s.2" 0 "1
2
" ""

# We should only be able to create one file if we use -i.
docommand C13 "${admin} -n -ifoo s.1 s.2" 1 "" IGNORE

remove s.1 s.2 foo command.log $s
success
E 1
