#! /bin/sh
# reportmr.sh:  Testing for MR the reporting of numbers.

# Import common functions & definitions.
. ../common/test-common
. ../common/need-prt


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
