hV6,sum=62285
s 00001/00001/00013
d D 1.2 2015/06/03 00:06:44+0200 joerg 2 1
S s 36816
c ../common/test-common -> ../../common/test-common
e
s 00014/00000/00000
d D 1.1 2011/05/10 23:24:24+0200 joerg 1 0
S s 36677
c date and time created 11/05/10 23:24:24 by joerg
e
u
U
f e 0
f y 
G r 0e46e8ef42086
G p sccs/tests/cssctests/val/historical.sh
t
T
I 1
#! /bin/sh

# historical.sh:  Validation of SCCS file features only found in 
#                 SCCS files makde by historical versions of SCCS.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

# s.comment-nospace has a comment line in which 
# no space follows the ^Ac.  A space is more
# usual and (as far as I know) no current SCCS
# implementation omits it.
docommand c1 "${vg_val} s.comment-nospace" 0 IGNORE IGNORE

E 1
