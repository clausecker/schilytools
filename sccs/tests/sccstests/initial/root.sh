#! /bin/sh

# Basic tests for extended SCCS: do not test as root!

# Read test core functions
. ../../common/test-common

echo_nonl "root..."
. ../../common/not-root
echo "passed"
