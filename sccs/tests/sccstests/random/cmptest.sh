#!/bin/sh
#
# cmptest @(#)cmptest.sh	1.9 16/11/07 Copyright 2015-2016 J. Schilling
#
# Usage: cmptest	---> runs 1000 test loops
#	 cmptest #	---> runs # test loops
#
# A file is generated, then modified.
#

# Read test core functions
. ../../common/test-common

[ "$NO_RANDOM" = TRUE ] && exit

cmd=admin		# for ../../common/optv
ocmd=${admin}		# for ../../common/optv
g=foo
s=s.$g
p=p.$g
z=z.$g

remove $z $s $p $g

: ${AWK=/usr/bin/nawk}
#$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=/usr/bin/nawk
$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=/usr/bin/gawk
$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=/usr/bin/awk
$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=nawk
$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=gawk
$AWK 'BEGIN {print rand()}' < /dev/null > /dev/null 2> /dev/null || AWK=awk

trap cleanup EXIT INT HUP

cleanup() {
	rm -f saved_orig changed patch_file expected original original.* failure xo xm xof xmf xef
}

rrand() {
	$AWK '
	function random(low, range) {
		return int(range * rand()) + low
	}

	BEGIN {
		base = ARGV[1]
		max = ARGV[2]
		seed = ARGV[3]

		srand()		# Initialze with current time
		s = srand()	# get previous seed
		s = s + seed	# Current time + seed
		srand(s)	# Better new seed

		x = random(base, max)
		print x
	}
	' "$@"
}

makefile() {
	$AWK '
	BEGIN {
		nflines = ARGV[1]
		for (i = 1; i <= nflines; i++) {
			printf("This is original line %d\n", i);
		}
	}
	' "$@"
}

changefile() {
	$AWK '
	function random(n) {
		return int(n * rand())
	}

	BEGIN {
		nflines = ARGV[1]
		chlines = ARGV[2]
		seed = chlines

		ARGV[1] = ""	# Do not use as filename
		ARGV[2] = ""	# Do not use as filename

		srand()		# Initialze with current time
		s = srand()	# get previous seed
		s = s + seed	# Current time + seed
		srand(s)	# Better new seed

		for (i = 1; i <= nflines; i++) {
			change_line[i] = 0
		}

		while (chlines > 0) {
			i = random(nflines)
			if (change_line[i] == 0) {
				chlines--
				change_line[i] = 1
			}
		}
		i = 1;
	}

	{
		if (change_line[i] != 0) {
			what = random(3)
			if (what == 0) {
				print "Modified, was: " $0
			} else if (what == 1) {
				print $0
				print "New line, inserted after: " $0
			}
		} else {
			print $0
		}
		i++
		next
	}
	' "$@"
}

nlines=$$	# seed startup helper, awk would get seed based on time_t
maxlines=5000	# Longest file for our tests
maxch=4		# Max. 25% of all lines are changed

idx=0
maxidx=${1:-1000}
while [ $idx -le $maxidx ]
do
	nlines=`rrand 10 $maxlines $nlines`

	max_changes=`expr $nlines / $maxch`
	changes=`rrand 1 $max_changes $nlines`

	makefile $nlines > original
	cp original saved_orig

	echo Test $idx: testing nlines=$nlines changes=$changes

	changefile $nlines $changes original > changed
	cp saved_orig original

	echo testing file...
	remove $s foo
			${admin} -fy -ioriginal $s
	[ $? -eq 0 ] && ${get} -e $s			&& cp changed foo
	[ $? -eq 0 ] && ${delta} -ycomment $s
	[ $? -eq 0 ] && ${get} $s
	ret=$?
	if [ $ret -ne 0 ]; then
		echo Test $idx: sccs test returned $ret
		diff original expected > failure
		trap 0
		exit 1
	fi

	diff changed foo > failure
	ret=$?
	if [ $ret -ne 0 ]; then
		echo Test $idx: diff returned $ret
		[ $ret -eq 1 ] && trap 0
		exit 1
	fi
	idx=`expr $idx + 1`
done
idx=`expr $idx - 1`
echo Test succeeded after $idx runs...

remove $z $s $p $g
