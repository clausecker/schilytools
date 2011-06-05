#! /bin/sh

# y2k.sh:  Y2K tests for the "val" command.

# Import common functions & definitions.
. ../common/test-common

files="f s.f"

remove $files

docommand y1 "${vg_val} ../year-2000/s.y2k.txt" 0 IGNORE IGNORE

remove $files
success
