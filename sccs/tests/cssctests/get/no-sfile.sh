#! /bin/sh

## no-sfile.sh
#     Make sure that we don't coredump if there is no input file.

# Import common functions & definitions.
. ../../common/test-common


docommand N1 "${vg_get} -p" 1 IGNORE IGNORE

remove command.log
success
