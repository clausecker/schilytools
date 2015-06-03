#! /bin/sh

# historical.sh:  Validation of SCCS file features only found in 
#                 SCCS files makde by historical versions of SCCS.

# Import common functions & definitions.
. ../../common/test-common

# s.comment-nospace has a comment line in which 
# no space follows the ^Ac.  A space is more
# usual and (as far as I know) no current SCCS
# implementation omits it.
docommand c1 "${vg_val} s.comment-nospace" 0 IGNORE IGNORE

