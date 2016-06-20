#! /bin/sh
#
# @(#)offset.sh	1.2 16/06/16 2015-2016 J. Schilling
#

# offset.sh:   Testing matching with various offsets.
#

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C

cat > a.diff <<EOF
--- a
+++ a
@@ -1,3 +1,4 @@
 2
+insert
 3
 4
EOF

seq 1 5 > a
docommand OFF1 "${patch} < a.diff" 0 IGNORE "Hmm...  Looks like a unified diff to me...
The text leading up to this was:
--------------------------
|--- a
|+++ a
--------------------------
Patching file a using Plan A...
Hunk #1 succeeded at 2 (offset 1 line).
done
"

seq 2 5 > a
docommand OFF2 "${patch} < a.diff" 0 IGNORE "Hmm...  Looks like a unified diff to me...
The text leading up to this was:
--------------------------
|--- a
|+++ a
--------------------------
Patching file a using Plan A...
done
"

cat > a.diff <<EOF
--- a
+++ a
@@ -2,3 +2,4 @@
 2
+insert
 3
 4
EOF

seq 1 5 > a
docommand OFF3 "${patch} < a.diff" 0 IGNORE "Hmm...  Looks like a unified diff to me...
The text leading up to this was:
--------------------------
|--- a
|+++ a
--------------------------
Patching file a using Plan A...
done
"

cat > a.diff <<EOF
--- a
+++ a
@@ -2,3 +2,4 @@
 2
 3
+insert
 4
EOF

seq 1 5 > a
docommand OFF4 "${patch} < a.diff" 0 IGNORE "Hmm...  Looks like a unified diff to me...
The text leading up to this was:
--------------------------
|--- a
|+++ a
--------------------------
Patching file a using Plan A...
done
"

remove a a.diff a.orig
success
