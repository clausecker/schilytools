#! /bin/sh

# incr.sh:  Testing incremental backups and restores.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C export LC_ALL

#d=`../testutils/realpwd`

#
# Basic tests with remove, rename, append, ...
#
docommand INCR-basic "tar=${tar}; export tar; sh basic 2> err.out" 0 "" ""

#
# This is where GNU tar always fails as GNU tar is unable to hndle renamed
# directories with incremental restores.
#
docommand INCR-dir-rename "tar=${tar}; export tar; sh rename-dir 2> err.out" 0 "" ""

#
# Two plain files are removed and the same inode numbers with a different
# file type (named pipe) appears in the incremental.
#
docommand INCR-same-ino "tar=${tar}; export tar; sh same-ino 2> err.out" 0 "" ""

remove err.out

success
