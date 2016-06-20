#! /bin/sh
#
# @(#)asym-context.sh	1.2 16/06/16 2015-2016 J. Schilling
#

# asym-context.sh:  Testing matching offsets and asymmetic
#			context around the delta.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C


#
# Create a patch with asymmetric context
#
cat > x.diff <<EOF
--- x
+++ x
@@ -1,3 +1,4 @@
 2
+insert
 3
 4
EOF

#
# Starting at one causes one line offset
#
seq 1 5 > x
docommand ASC1 "${patch} < x.diff" 0 IGNORE "Hmm...  Looks like a unified diff to me...
The text leading up to this was:
--------------------------
|--- x
|+++ x
--------------------------
Patching file x using Plan A...
Hunk #1 succeeded at 2 (offset 1 line).
done
"

#
# Starting at two, now it fits the patch
#
seq 2 5 > x
docommand ASC2 "${patch} < x.diff" 0 IGNORE "Hmm...  Looks like a unified diff to me...
The text leading up to this was:
--------------------------
|--- x
|+++ x
--------------------------
Patching file x using Plan A...
done
"

#
# Asymmetric, but correct offset for first context line
#
cat > x.diff <<EOF
--- x
+++ x
@@ -2,3 +2,4 @@
 2
+insert
 3
 4
EOF

#
# Correct offset for first context line if we start at 1
#
seq 1 5 > x
docommand ASC3 "${patch} < x.diff" 0 IGNORE "Hmm...  Looks like a unified diff to me...
The text leading up to this was:
--------------------------
|--- x
|+++ x
--------------------------
Patching file x using Plan A...
done
"

#
# Wrong offset for first context line if we start at 2
#
seq 2 5 > x
docommand ASC4 "${patch} < x.diff" 0 IGNORE "Hmm...  Looks like a unified diff to me...
The text leading up to this was:
--------------------------
|--- x
|+++ x
--------------------------
Patching file x using Plan A...
Hunk #1 succeeded at 1 (offset -1 lines).
done
"

#
# Now less context lines after the insertion
#
cat > x.diff <<EOF
--- x
+++ x
@@ -2,3 +2,4 @@
 2
 3
+insert
 4
EOF

#
# Correct offset for first context line if we start at 1
#
seq 1 5 > x
docommand ASC5 "${patch} < x.diff" 0 IGNORE "Hmm...  Looks like a unified diff to me...
The text leading up to this was:
--------------------------
|--- x
|+++ x
--------------------------
Patching file x using Plan A...
done
"

#
# Wrong offset for first context line if we start at 2
#
seq 2 5 > x
docommand ASC6 "${patch} < x.diff" 0 IGNORE "Hmm...  Looks like a unified diff to me...
The text leading up to this was:
--------------------------
|--- x
|+++ x
--------------------------
Patching file x using Plan A...
Hunk #1 succeeded at 1 (offset -1 lines).
done
"

remove x x.diff x.orig
success
