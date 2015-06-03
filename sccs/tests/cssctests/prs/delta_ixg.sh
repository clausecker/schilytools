#! /bin/sh

# delta_ixg.sh:  Testing for reporting included, excluded, ignored deltas.

# Import common functions & definitions.
. ../../common/test-common

cleanup () {
    remove command.log
}

# CSSC's prs emits a warning when it sees an ignored delta, because
# the feature is not fully tested.  We use the macro NO_STDERR to
# indicate cases where there should be no output on stderr, but in
# fact a warning message is issued by CSSC (but not SCCS).
NO_STDERR=IGNORE

# This table summarises our example s-file.   
# The first column is three characters.   
# An i appears if the indicated SID includes a delta.
# An x appears if the indicated SID excludes a delta.
# A  g appears if the indicated SID ignores  a delta.
# In each of these cases, just one delta is being included/excluded/ignored.
# 
# ---    1.1 
# --g    1.5
# -x-    1.2.1.1
# -xg    1.1.1.2
# i--    1.1.1.1
# i-g    1.3.1.1
# ix-    1.2.2.1
# ixg    1.6


# Base case (---): no included, excluded, ignored deltas.
docommand n1 "${prs} -d':I: :Dn:' -r1.1 s.delta_ixg" 0 "1.1 \n"      "${NO_STDERR}"
docommand x1 "${prs} -d':I: :Dx:' -r1.1 s.delta_ixg" 0 "1.1 \n"      "${NO_STDERR}"
docommand g1 "${prs} -d':I: :Dg:' -r1.1 s.delta_ixg" 0 "1.1 \n"      "${NO_STDERR}"
docommand I1 "${prs} -d':I: :DI:' -r1.1 s.delta_ixg" 0 "1.1 //\n"    "${NO_STDERR}"

# Simple example: ignores (--g).
# Delta 1.5 (seq 5) ignores 1.3 (seq 3).
docommand n5 "${prs} -d':I: :Dn:' -r1.5 s.delta_ixg" 0 "1.5 \n"      "${NO_STDERR}"
docommand x5 "${prs} -d':I: :Dx:' -r1.5 s.delta_ixg" 0 "1.5 \n"      "${NO_STDERR}"
docommand g5 "${prs} -d':I: :Dg:' -r1.5 s.delta_ixg" 0 "1.5 3\n"     "${NO_STDERR}"
docommand I5 "${prs} -d':I: :DI:' -r1.5 s.delta_ixg" 0 "1.5 //3\n"   "${NO_STDERR}"

# Exclude (-x-): 1.2.1.1 (seq 10) excludes 1.1 (seq 1)
docommand n10 "${prs} -d':I: :Dn:' -r1.2.1.1 s.delta_ixg" 0 "1.2.1.1 \n"      "${NO_STDERR}"
docommand x10 "${prs} -d':I: :Dx:' -r1.2.1.1 s.delta_ixg" 0 "1.2.1.1 1\n"     "${NO_STDERR}"
docommand g10 "${prs} -d':I: :Dg:' -r1.2.1.1 s.delta_ixg" 0 "1.2.1.1 \n"      "${NO_STDERR}"
docommand I10 "${prs} -d':I: :DI:' -r1.2.1.1 s.delta_ixg" 0 "1.2.1.1 /1/\n"   "${NO_STDERR}"

# -xg    1.1.1.2 (seq 11) excludes 1.2 (seq 2) and ignores 1.1 (seq 1)
docommand n11 "${prs} -d':I: :Dn:' -r1.1.1.2 s.delta_ixg" 0 "1.1.1.2 \n"      "${NO_STDERR}"
docommand x11 "${prs} -d':I: :Dx:' -r1.1.1.2 s.delta_ixg" 0 "1.1.1.2 2\n"     "${NO_STDERR}"
docommand g11 "${prs} -d':I: :Dg:' -r1.1.1.2 s.delta_ixg" 0 "1.1.1.2 1\n"     "${NO_STDERR}"
docommand I11 "${prs} -d':I: :DI:' -r1.1.1.2 s.delta_ixg" 0 "1.1.1.2 /2/1\n"  "${NO_STDERR}"

# i--    1.1.1.1 (seq 7) includes 1.2 (seq 2).
docommand n7 "${prs} -d':I: :Dn:' -r1.1.1.1 s.delta_ixg" 0 "1.1.1.1 2\n"      "${NO_STDERR}"
docommand x7 "${prs} -d':I: :Dx:' -r1.1.1.1 s.delta_ixg" 0 "1.1.1.1 \n"       "${NO_STDERR}"
docommand g7 "${prs} -d':I: :Dg:' -r1.1.1.1 s.delta_ixg" 0 "1.1.1.1 \n"       "${NO_STDERR}"
docommand I7 "${prs} -d':I: :DI:' -r1.1.1.1 s.delta_ixg" 0 "1.1.1.1 2//\n"    "${NO_STDERR}"


# i-g    1.3.1.1 (seq 9) includes 1.4 (seq 4) and ignores 1.2 (seq 2)
docommand n9 "${prs} -d':I: :Dn:' -r1.3.1.1 s.delta_ixg" 0 "1.3.1.1 4\n"      "${NO_STDERR}"
docommand x9 "${prs} -d':I: :Dx:' -r1.3.1.1 s.delta_ixg" 0 "1.3.1.1 \n"       "${NO_STDERR}"
docommand g9 "${prs} -d':I: :Dg:' -r1.3.1.1 s.delta_ixg" 0 "1.3.1.1 2\n"      "${NO_STDERR}"
docommand I9 "${prs} -d':I: :DI:' -r1.3.1.1 s.delta_ixg" 0 "1.3.1.1 4//2\n"   "${NO_STDERR}"

# ix-    1.2.2.1 (seq 12) includes 1.3 (seq 3) and excludes 1.1 (seq 1)
docommand n12 "${prs} -d':I: :Dn:' -r1.2.2.1 s.delta_ixg" 0 "1.2.2.1 3\n"     "${NO_STDERR}"
docommand x12 "${prs} -d':I: :Dx:' -r1.2.2.1 s.delta_ixg" 0 "1.2.2.1 1\n"     "${NO_STDERR}"
docommand g12 "${prs} -d':I: :Dg:' -r1.2.2.1 s.delta_ixg" 0 "1.2.2.1 \n"      "${NO_STDERR}"
docommand I12 "${prs} -d':I: :DI:' -r1.2.2.1 s.delta_ixg" 0 "1.2.2.1 3/1/\n"  "${NO_STDERR}"


# All of them (ixg)
# Delta 1.6 (seq 6) includes 1.3 (seq 3), excludes 1.1 (seq 1) and ignores 1.2 (seq 2).
docommand n6 "${prs} -d':I: :Dn:' -r1.6 s.delta_ixg" 0 "1.6 3\n"     "${NO_STDERR}"
docommand x6 "${prs} -d':I: :Dx:' -r1.6 s.delta_ixg" 0 "1.6 1\n"     "${NO_STDERR}"
docommand g6 "${prs} -d':I: :Dg:' -r1.6 s.delta_ixg" 0 "1.6 2\n"     "${NO_STDERR}"
docommand I6 "${prs} -d':I: :DI:' -r1.6 s.delta_ixg" 0 "1.6 3/1/2\n" "${NO_STDERR}"

cleanup
success
