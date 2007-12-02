#!/bin/sh
#ident "@(#)mkdep-aix.sh	1.1 02/10/11 "
###########################################################################
# Copyright 2002 by J. Schilling
###########################################################################
#
# Create dependency list with AIX cc
#
###########################################################################
#
# This script will probably not work correctly with a list of C-files
# but as we don't need it with 'smake' or 'gmake' it seems to be sufficient.
#
# Note that AIX cc will create a file foo.u for every foo.c file. The file
# foo.u is located in the directory where cc is run.
# For this reason, there may be problems of other software likes to create
# foo.u during compilation. Please report problems.
#
###########################################################################
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
# (the "License").  You may not use this file except in compliance
# with the License.
#
# See the file CDDL.Schily.txt in this distribution for details.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file CDDL.Schily.txt from this distribution.
###########################################################################
FILES=
UFILES=
trap 'rm -f $UFILES ; exit 1' 1 2 15

for i in "$@"; do

	case "$i" in

	-*)	# ignore options
		;;
	*.c)	if [ ! -z "$FILES" ]; then
			FILES="$FILES "
		fi
		b=`basename $i ''`
		FILES="$FILES$b"
		;;
	esac
done

UFILES=`echo "$FILES" | sed -e 's;\([^.]*\)\.c;\1.u;g'`

rm -f $UFILES
cc -M -E > /dev/null "$@"
cat $UFILES
rm -f $UFILES
