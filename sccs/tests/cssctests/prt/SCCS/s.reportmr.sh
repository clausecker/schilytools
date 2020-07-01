hV6,sum=42353
s 00002/00002/00021
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 15269
c ../common/test-common -> ../../common/test-common
e
s 00023/00000/00000
d D 1.1 2010/05/11 11:30:00+0200 joerg 1 0
S s 14991
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ee710ab
G p sccs/tests/cssctests/prt/reportmr.sh
t
T
I 1
#! /bin/sh
# reportmr.sh:  Testing for MR the reporting of numbers.

# Import common functions & definitions.
D 2
. ../common/test-common
. ../common/need-prt
E 2
I 2
. ../../common/test-common
. ../../common/need-prt
E 2


g=reportmr.1 
s=inputs/s.$g
remove $g

# Vanilla operation, but with two files.
docommand R1 "${vg_prt} $s $s" 0 "\ninputs/s.reportmr.1:\n\nD 1.1\t98/05/10 20:44:58 james\t1 0\t00000/00000/00000\nMRs:\t123\nMRs:\t456\ndate and time created 98/05/10 20:44:58 by james\n\ninputs/s.reportmr.1:\n\nD 1.1\t98/05/10 20:44:58 james\t1 0\t00000/00000/00000\nMRs:\t123\nMRs:\t456\ndate and time created 98/05/10 20:44:58 by james\n" ""

# Vanilla operation, but with two files and the "-y" flag.
docommand R2 "${vg_prt} -y $s $s" 0 "\ninputs/s.reportmr.1:\tD 1.1\t98/05/10 20:44:58 james\t1 0\t00000/00000/00000 MRs:\t123 MRs:\t456 date and time created 98/05/10 20:44:58 by james\n\ninputs/s.reportmr.1:\tD 1.1\t98/05/10 20:44:58 james\t1 0\t00000/00000/00000 MRs:\t123 MRs:\t456 date and time created 98/05/10 20:44:58 by james\n" ""


remove $g
remove command.log passwd 

success
E 1
