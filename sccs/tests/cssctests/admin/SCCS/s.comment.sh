h14274
s 00001/00001/00075
d D 1.3 11/10/21 23:07:38 joerg 3 2
c prs -d:DI: Tests sind nun POSIX konform
e
s 00003/00003/00073
d D 1.2 11/05/30 01:13:10 joerg 2 1
c sed -ne '/^COMMENTS:$/,/$/ p' -> sed -ne '/^COMMENTS:$/,/^.*$/ p'
e
s 00076/00000/00000
d D 1.1 10/04/29 02:05:14 joerg 1 0
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

# comment.sh:  Testing for comments at initialisation time.

# Import common functions & definitions.
. ../common/test-common

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
tail -1 prs.$s | egrep \
 '^date and time created [0-9][0-9]/[0-1][0-9]/[0-3][0-9] [0-2][0-9]:[0-5][0-9]:[0-5][0-9] by ' >/dev/null\
    || fail "default message format error."
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

docommand C12 "${vg_prs} -d':M:\n' s.1 s.2" 0 "1
2
" ""

# We should only be able to create one file if we use -i.
docommand C13 "${admin} -n -ifoo s.1 s.2" 1 "" IGNORE

remove s.1 s.2 foo command.log $s
success
E 1
