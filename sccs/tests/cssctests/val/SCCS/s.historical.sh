h44963
s 00014/00000/00000
d D 1.1 11/05/10 23:24:24 joerg 1 0
c date and time created 11/05/10 23:24:24 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

# historical.sh:  Validation of SCCS file features only found in 
#                 SCCS files makde by historical versions of SCCS.

# Import common functions & definitions.
. ../common/test-common

# s.comment-nospace has a comment line in which 
# no space follows the ^Ac.  A space is more
# usual and (as far as I know) no current SCCS
# implementation omits it.
docommand c1 "${vg_val} s.comment-nospace" 0 IGNORE IGNORE

E 1
