h27581
s 00013/00000/00000
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

## no-sfile.sh
#     Make sure that we don't coredump if there is no input file.

# Import common functions & definitions.
. ../common/test-common


docommand N1 "${vg_get} -p" 1 IGNORE IGNORE

remove command.log
success
E 1
