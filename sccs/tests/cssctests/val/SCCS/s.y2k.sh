h27703
s 00015/00000/00000
d D 1.1 10/05/03 03:11:28 joerg 1 0
c date and time created 10/05/03 03:11:28 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

# y2k.sh:  Y2K tests for the "val" command.

# Import common functions & definitions.
. ../common/test-common

files="f s.f"

remove $files

docommand y1 "${vg_val} ../year-2000/s.y2k.txt" 0 IGNORE IGNORE

remove $files
success
E 1
