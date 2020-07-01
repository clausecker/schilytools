hV6,sum=12385
s 00024/00000/00123
d D 1.5 2019/05/12 14:28:12+0200 joerg 5 4
S s 42040
c Neuer Hinweis: :DI: Bug wurde 1984 surch AT&T eingefuehrt
e
s 00037/00002/00086
d D 1.4 2019/05/12 14:12:21+0200 joerg 4 3
S s 03340
c prs -D:DI: unterscheidet nun fuer non-POSIX Varianten
c Kommentar verbessert
e
s 00001/00001/00087
d D 1.3 2015/06/03 00:06:44+0200 joerg 3 2
S s 52052
c ../common/test-common -> ../../common/test-common
e
s 00006/00006/00082
d D 1.2 2011/10/21 23:07:38+0200 joerg 2 1
S s 51913
c prs -d:DI: Tests sind nun POSIX konform
e
s 00088/00000/00000
d D 1.1 2011/05/10 23:24:24+0200 joerg 1 0
S s 51793
c date and time created 11/05/10 23:24:24 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ee39bea
G p sccs/tests/cssctests/prs/delta_ixg.sh
t
T
I 1
#! /bin/sh

# delta_ixg.sh:  Testing for reporting included, excluded, ignored deltas.

# Import common functions & definitions.
D 3
. ../common/test-common
E 3
I 3
. ../../common/test-common
I 4
. ../../common/real-thing
E 4
E 3

I 4
# $TESTING_SCCS_V5	Test SCCSv5 features from SunOS
# $TESTING_CSSC		Relict from CSSC tests, also applies to SCCS
# $TESTING_REAL_CSSC	Test real CSSC 4-digit year extensions
# $TESTING_REAL_SCCS	Test real Schily SCCS 4 digit year extensions
# $TESTING_SCCS_V6	Test SCCSv6 features

E 4
cleanup () {
    remove command.log
}

# CSSC's prs emits a warning when it sees an ignored delta, because
# the feature is not fully tested.  We use the macro NO_STDERR to
# indicate cases where there should be no output on stderr, but in
# fact a warning message is issued by CSSC (but not SCCS).
NO_STDERR=IGNORE

# This table summarises our example s-file.   
# The first column is three characters.   
# An i appears if the indicated SID includes a delta.
# An x appears if the indicated SID excludes a delta.
# A  g appears if the indicated SID ignores  a delta.
# In each of these cases, just one delta is being included/excluded/ignored.
# 
# ---    1.1 
# --g    1.5
# -x-    1.2.1.1
# -xg    1.1.1.2
# i--    1.1.1.1
# i-g    1.3.1.1
# ix-    1.2.2.1
# ixg    1.6


# Base case (---): no included, excluded, ignored deltas.
docommand n1 "${prs} -d':I: :Dn:' -r1.1 s.delta_ixg" 0 "1.1 \n"      "${NO_STDERR}"
docommand x1 "${prs} -d':I: :Dx:' -r1.1 s.delta_ixg" 0 "1.1 \n"      "${NO_STDERR}"
docommand g1 "${prs} -d':I: :Dg:' -r1.1 s.delta_ixg" 0 "1.1 \n"      "${NO_STDERR}"
I 4
if $TESTING_REAL_SCCS
then
E 4
D 2
docommand I1 "${prs} -d':I: :DI:' -r1.1 s.delta_ixg" 0 "1.1 \n"      "${NO_STDERR}"
E 2
I 2
docommand I1 "${prs} -d':I: :DI:' -r1.1 s.delta_ixg" 0 "1.1 //\n"    "${NO_STDERR}"
I 4
else
I 5
#
# Not POSIX compliant for -d:DI
# This applies to AT&T SCCS past 1984 and to CSSC
#
E 5
docommand I1-nonPOSIX "${prs} -d':I: :DI:' -r1.1 s.delta_ixg" 0 "1.1 \n"    "${NO_STDERR}"
fi
E 4
E 2

# Simple example: ignores (--g).
# Delta 1.5 (seq 5) ignores 1.3 (seq 3).
docommand n5 "${prs} -d':I: :Dn:' -r1.5 s.delta_ixg" 0 "1.5 \n"      "${NO_STDERR}"
docommand x5 "${prs} -d':I: :Dx:' -r1.5 s.delta_ixg" 0 "1.5 \n"      "${NO_STDERR}"
docommand g5 "${prs} -d':I: :Dg:' -r1.5 s.delta_ixg" 0 "1.5 3\n"     "${NO_STDERR}"
I 4
if $TESTING_REAL_SCCS
then
E 4
D 2
docommand I5 "${prs} -d':I: :DI:' -r1.5 s.delta_ixg" 0 "1.5 /3\n"    "${NO_STDERR}"
E 2
I 2
docommand I5 "${prs} -d':I: :DI:' -r1.5 s.delta_ixg" 0 "1.5 //3\n"   "${NO_STDERR}"
I 4
else
I 5
#
# Not POSIX compliant for -d:DI
# This applies to AT&T SCCS past 1984 and to CSSC
#
E 5
docommand I5-nonPOSIX "${prs} -d':I: :DI:' -r1.5 s.delta_ixg" 0 "1.5 /3\n"   "${NO_STDERR}"
fi
E 4
E 2

# Exclude (-x-): 1.2.1.1 (seq 10) excludes 1.1 (seq 1)
docommand n10 "${prs} -d':I: :Dn:' -r1.2.1.1 s.delta_ixg" 0 "1.2.1.1 \n"      "${NO_STDERR}"
docommand x10 "${prs} -d':I: :Dx:' -r1.2.1.1 s.delta_ixg" 0 "1.2.1.1 1\n"     "${NO_STDERR}"
docommand g10 "${prs} -d':I: :Dg:' -r1.2.1.1 s.delta_ixg" 0 "1.2.1.1 \n"      "${NO_STDERR}"
I 4
if $TESTING_REAL_SCCS
then
E 4
D 2
docommand I10 "${prs} -d':I: :DI:' -r1.2.1.1 s.delta_ixg" 0 "1.2.1.1 /1\n"    "${NO_STDERR}"
E 2
I 2
docommand I10 "${prs} -d':I: :DI:' -r1.2.1.1 s.delta_ixg" 0 "1.2.1.1 /1/\n"   "${NO_STDERR}"
I 4
else
I 5
#
# Not POSIX compliant for -d:DI
# This applies to AT&T SCCS past 1984 and to CSSC
#
E 5
docommand I10-nonPOSIX "${prs} -d':I: :DI:' -r1.2.1.1 s.delta_ixg" 0 "1.2.1.1 /1\n"   "${NO_STDERR}"
fi
E 4
E 2

# -xg    1.1.1.2 (seq 11) excludes 1.2 (seq 2) and ignores 1.1 (seq 1)
docommand n11 "${prs} -d':I: :Dn:' -r1.1.1.2 s.delta_ixg" 0 "1.1.1.2 \n"      "${NO_STDERR}"
docommand x11 "${prs} -d':I: :Dx:' -r1.1.1.2 s.delta_ixg" 0 "1.1.1.2 2\n"     "${NO_STDERR}"
docommand g11 "${prs} -d':I: :Dg:' -r1.1.1.2 s.delta_ixg" 0 "1.1.1.2 1\n"     "${NO_STDERR}"
docommand I11 "${prs} -d':I: :DI:' -r1.1.1.2 s.delta_ixg" 0 "1.1.1.2 /2/1\n"  "${NO_STDERR}"

# i--    1.1.1.1 (seq 7) includes 1.2 (seq 2).
docommand n7 "${prs} -d':I: :Dn:' -r1.1.1.1 s.delta_ixg" 0 "1.1.1.1 2\n"      "${NO_STDERR}"
docommand x7 "${prs} -d':I: :Dx:' -r1.1.1.1 s.delta_ixg" 0 "1.1.1.1 \n"       "${NO_STDERR}"
docommand g7 "${prs} -d':I: :Dg:' -r1.1.1.1 s.delta_ixg" 0 "1.1.1.1 \n"       "${NO_STDERR}"
I 4
if $TESTING_REAL_SCCS
then
E 4
D 2
docommand I7 "${prs} -d':I: :DI:' -r1.1.1.1 s.delta_ixg" 0 "1.1.1.1 2\n"      "${NO_STDERR}"
E 2
I 2
docommand I7 "${prs} -d':I: :DI:' -r1.1.1.1 s.delta_ixg" 0 "1.1.1.1 2//\n"    "${NO_STDERR}"
I 4
else
I 5
#
# Not POSIX compliant for -d:DI
# This applies to AT&T SCCS past 1984 and to CSSC
#
E 5
docommand I7-nonPOSIX "${prs} -d':I: :DI:' -r1.1.1.1 s.delta_ixg" 0 "1.1.1.1 2\n"    "${NO_STDERR}"
fi
E 4
E 2

D 4

E 4
# i-g    1.3.1.1 (seq 9) includes 1.4 (seq 4) and ignores 1.2 (seq 2)
docommand n9 "${prs} -d':I: :Dn:' -r1.3.1.1 s.delta_ixg" 0 "1.3.1.1 4\n"      "${NO_STDERR}"
docommand x9 "${prs} -d':I: :Dx:' -r1.3.1.1 s.delta_ixg" 0 "1.3.1.1 \n"       "${NO_STDERR}"
docommand g9 "${prs} -d':I: :Dg:' -r1.3.1.1 s.delta_ixg" 0 "1.3.1.1 2\n"      "${NO_STDERR}"
I 4
if $TESTING_REAL_SCCS
then
E 4
D 2
docommand I9 "${prs} -d':I: :DI:' -r1.3.1.1 s.delta_ixg" 0 "1.3.1.1 4/2\n"    "${NO_STDERR}"
E 2
I 2
docommand I9 "${prs} -d':I: :DI:' -r1.3.1.1 s.delta_ixg" 0 "1.3.1.1 4//2\n"   "${NO_STDERR}"
I 4
else
I 5
#
# Not POSIX compliant for -d:DI
# This applies to AT&T SCCS past 1984 and to CSSC
#
E 5
docommand I9-nonPOSIX "${prs} -d':I: :DI:' -r1.3.1.1 s.delta_ixg" 0 "1.3.1.1 4/2\n"   "${NO_STDERR}"
fi
E 4
E 2

# ix-    1.2.2.1 (seq 12) includes 1.3 (seq 3) and excludes 1.1 (seq 1)
docommand n12 "${prs} -d':I: :Dn:' -r1.2.2.1 s.delta_ixg" 0 "1.2.2.1 3\n"     "${NO_STDERR}"
docommand x12 "${prs} -d':I: :Dx:' -r1.2.2.1 s.delta_ixg" 0 "1.2.2.1 1\n"     "${NO_STDERR}"
docommand g12 "${prs} -d':I: :Dg:' -r1.2.2.1 s.delta_ixg" 0 "1.2.2.1 \n"      "${NO_STDERR}"
I 4
if $TESTING_REAL_SCCS
then
E 4
D 2
docommand I12 "${prs} -d':I: :DI:' -r1.2.2.1 s.delta_ixg" 0 "1.2.2.1 3/1\n"   "${NO_STDERR}"
E 2
I 2
docommand I12 "${prs} -d':I: :DI:' -r1.2.2.1 s.delta_ixg" 0 "1.2.2.1 3/1/\n"  "${NO_STDERR}"
I 4
else
I 5
#
# Not POSIX compliant for -d:DI
# This applies to AT&T SCCS past 1984 and to CSSC
#
E 5
docommand I12-nonPOSIX "${prs} -d':I: :DI:' -r1.2.2.1 s.delta_ixg" 0 "1.2.2.1 3/1\n"  "${NO_STDERR}"
fi
E 4
E 2

D 4

E 4
# All of them (ixg)
# Delta 1.6 (seq 6) includes 1.3 (seq 3), excludes 1.1 (seq 1) and ignores 1.2 (seq 2).
docommand n6 "${prs} -d':I: :Dn:' -r1.6 s.delta_ixg" 0 "1.6 3\n"     "${NO_STDERR}"
docommand x6 "${prs} -d':I: :Dx:' -r1.6 s.delta_ixg" 0 "1.6 1\n"     "${NO_STDERR}"
docommand g6 "${prs} -d':I: :Dg:' -r1.6 s.delta_ixg" 0 "1.6 2\n"     "${NO_STDERR}"
docommand I6 "${prs} -d':I: :DI:' -r1.6 s.delta_ixg" 0 "1.6 3/1/2\n" "${NO_STDERR}"

cleanup
success
E 1
