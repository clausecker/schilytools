#! /bin/sh

# basic.sh:  Testing for running cp 
#              is specified by an absolute path name.

# Import common functions & definitions.
. ../common/test-common

LC_ALL=C export LC_ALL

#d=`../testutils/realpwd`

#do_outerr B "${cpp} t.c" 0 t.i t.e

#
# Check filename "\"
#
do_outerr B01 "${cpp} t01.c" 0 t01.i t01.e

#
# Check __LINE__, __FILE__
#
do_outerr B02 "${cpp} t02.c" 1 t02.i t02.e
do_outerr B03 "${cpp} t03.c" 0 t03.i t03.e
do_outerr B03a "${cpp} t03a.c" 0 t03a.i t03a.e

#
# Check #if
#
do_outerr B04 "${cpp} t04.c" 0 t04.i t04.e
do_outerr B05 "${cpp} t05.c" 0 t05.i t05.e

#
# Check concat macro
# Note that the closed Sun CPP prints an error message:
#	t06a.c: line 5: missing */
# we should do that as well in the future.
#
do_outerr B06a "${cpp} t06a.c" 0 t06a.i t06a.e
do_outerr B06b "${cpp} t06b.c" 0 t06b.i t06b.e

#
# Check character 0xFE (þ)
#
do_outerr B07 "${cpp} t07.c" 0 t07.i t07.e
do_outerr B08 "${cpp} t08.c" 0 t08.i t08.e

#
# Check extra text after cpp directive
#
do_outerr B09 "${cpp} t09.c" 0 t09.i t09.e
do_outerr B09a "${cpp} -p t09.c" 0 t09.i t09.e2

success
