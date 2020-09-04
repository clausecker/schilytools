#! /bin/sh
# @(#)rcs2sccs.sh	1.20 20/08/25 Copyright 2011-2018 J. Schilling
#
#
# Id: rcs2sccs,v 1.12 90/10/04 20:52:23 kenc Exp Locker: kenc

############################################################
PATH=INS_BASE/SCCS_BIN_PREbin:$PATH:/usr/ccs/bin
############################################################
ex_code=0
do_v6=""
do_rm=FALSE
is_schily_sccs=FALSE

usage() {
	echo "Usage: rcs2sccs2 [-help] [-rm] [-V] [-V6] [file...]"
	echo "Options:"
	echo "	-help	print this help"
	echo "	-rm	Remove RCS files after completion"
	echo "	-V	Print SCCS version and exit"
	echo "	-V6	Create SCCS v6 history files"
}

while [ $# -ge 1 ]; do
	case "$1" in
	-rm | -remove)
		do_rm=TRUE
		shift
		continue;;
	-V | -version | --version)
		echo "$0 PROVIDER-SCCS version VERSION VDATE (HOST_SUB)"
		exit 0
		;;
	-V4)
		do_v6="-V4"
		shift
		continue;;
	-V6)
		do_v6=-V6
		shift
		continue;;
	'-?' | -help | --help)
		usage
		exit
		;;
	-*)
		echo "Illegal option $1"
		usage
		exit 1
		;;
	*)
		break;;
	esac
done

if [ "$do_v6" = "-V6" ]; then
	TZ=GMT
	export TZ
fi
############################################################
# Error checking
#
if [ ! -d SCCS ] ; then
    mkdir SCCS
fi

vers=`sccs -V | grep 'schily.*version'`
if [ -n "$vers" ]; then
	is_schily_sccs=TRUE
fi
if [ "$is_schily_sccs" = FALSE ]; then
	echo "No recent SCCS found"
	exit 1
fi

logfile=/tmp/rcs2sccs_$$_log
rm -f $logfile
tmpfile=/tmp/rcs2sccs_$$_tmp
rm -f $tmpfile
emptyfile=/tmp/rcs2sccs_$$_empty
echo -n "" > $emptyfile
initialfile=/tmp/rcs2sccs_$$_init
echo "Initial revision" > $initialfile
sedfile=/tmp/rcs2sccs_$$_sed
rm -f $sedfile
revfile=/tmp/rcs2sccs_$$_rev
rm -f $revfile
commentfile=/tmp/rcs2sccs_$$_comment
rm -f $commentfile

#
# NOTE: Signal numbers other then 1, 2, 3, 6, 9, 14, and 15 are not portable.
#
trap "rm -f $logfile $tmpfile $emptyfile $initialfile $sedfile $revfile $commentfile; exit" 0 1 2 3 15

# create the sed script
cat > $sedfile << EOF
s,;Id;,%Z%%M% %I% %E%,g
s,;SunId;,%Z%%M% %I% %E%,g
s,;RCSfile;,%M%,g
s,;Revision;,%I%,g
s,;Date;,%E%,g
s,;Id:.*;,%Z%%M% %I% %E%,g
s,;SunId:.*;,%Z%%M% %I% %E%,g
s,;RCSfile:.*;,%M%,g
s,;Revision:.*;,%I%,g
s,;Date:.*;,%E%,g
EOF
sed -e 's/;/\\$/g' $sedfile > $tmpfile
cp $tmpfile $sedfile

if sort -k 1,1 /dev/null 2>/dev/null; then
	sort_args='-k 1 -k 2 -k 3 -k 4 -k 5 -k 6 -k 7 -k 8 -k 9' 
else
	sort_args='+0 +1 +2 +3 +4 +5 +6 +7 +8'
fi

errlog()
{
	tail $logfile
	echo "See complete log in $logfile"
	logfile=""
}

############################################################
# Convert one RCS history file
#
rcs_to_sccs() {
    vfile="$1"

    # get rid of the ",v" at the end of the name
    file=`echo "$vfile" | sed -e 's/,v$//'`

    # work on each rev of that file in ascending order
    firsttime=1
    if sccs istext $do_v6 -s "$vfile"; then
	encode=""
    else
	encode=-b
    fi
    if test -x "$vfile"; then
	exec=TRUE
    else
	exec=FALSE
    fi
    rlog $file | grep "^revision [0-9][0-9]*\." | awk '{print $2}' | sed -e 's/\./ /g' | sort -n -u $sort_args | sed -e 's/ /./g' > $revfile
    for rev in `cat $revfile`; do
        if [ $? != 0 ]; then
		echo ERROR - revision
		exit 1
	fi
        # get file into current dir and get stats
	if [ "$do_v6" = "-V6" ]; then
	        date=`rlog -r$rev $file | grep "^date: " | awk '{print $2; exit}'`
	        time=`rlog -r$rev $file | grep "^date: " | awk '{print $3; exit}' | sed -e 's/;/+0000/'`
	else
	        date=`rlog -r$rev $file | grep "^date: " | awk '{print $2; exit}' | \
				sed -e 's,^19\([0-9][0-9]/\),\1,' -e 's,^20\([0-9][0-9]/\),\1,'`
	        time=`rlog -r$rev $file | grep "^date: " | awk '{print $3; exit}' | sed -e 's/;//'`
	fi
        author=`rlog -r$rev $file | grep "^date: " | awk '{print $5; exit}' | sed -e 's/;//'`
	date="$date $time"
        echo ""
	rlog -r$rev $file | sed -e '/^branches: /d' -e '1,/^date: /d' -e '/^===========/d' -e 's/$/\\/' > $commentfile
        echo "==> file $file, rev=$rev, date=$date, author=$author"
	rm -f $file
        co -r$rev $file >> $logfile  2>&1
        if [ $? != 0 ]; then
		echo ERROR - co
		errlog
		exit 1
	fi
        echo checked out of RCS

	if [ "$encode" = "" ]; then
		# add SCCS keywords in place of RCS keywords
		sed -f $sedfile $file > $tmpfile
		if [ $? != 0 ]; then
			echo ERROR - sed
			exit 1
		fi
		echo performed keyword substitutions
		rm -f $file
		cp $tmpfile $file
	fi

        # check file into SCCS
        if [ "$firsttime" = "1" ]; then
            firsttime=0
	    echo about to do sccs admin
            echo sccs admin $do_v6 $encode -n -i$file $file < $commentfile
            sccs admin $do_v6 $encode -n -i$file $file < $commentfile >> $logfile 2>&1
            if [ $? != 0 ]; then
		    echo ERROR - sccs admin
		    errlog
		    exit 1
	    fi
	    if [ "$do_v6" = "-V6" ]; then
		sed -e "s;^c date and time created ..../../.. ..:..:..+0000 by [^ ]*;c date and time created $date by $author;" SCCS/s.$file > $tmpfile
	    else
		sed -e "s;^c date and time created ../../.. ..:..:.. by [^ ]*;c date and time created $date by $author;" SCCS/s.$file > $tmpfile
	    fi
	    rm -f SCCS/s.$file
	    cp $tmpfile SCCS/s.$file
	    chmod 444 SCCS/s.$file
	    sccs admin -z $file
	    if [ $? != 0 ]; then
		    echo ERROR - sccs admin -z
		    exit 1
	    fi
            echo initial rev checked into SCCS
        else
	    #
	    # Unfortunately, rlog(1) does not print the "next" information that
	    # points to the predecessor revision, so we tell sccs to check out
	    # the highest revision of the same branch.
	    #
	    case $rev in
	    *.*.*.*)
		brev=`echo $rev | sed -e 's/\.[0-9]*$//'`
		sccs admin -fb $file 2>>$logfile
		echo sccs get -e -p -r$brev $file
		sccs get -e -p -r$brev $file >/dev/null 2>>$logfile
		;;
	    *)
		brev=`echo $rev | sed -e 's/\.[0-9]*$//'`
		echo sccs get -e -p -r$brev $file
		sccs get -e -p -r$brev $file >/dev/null 2>> $logfile
		;;
	    esac
	    if [ $? != 0 ]; then
		    echo ERROR - sccs get
		    errlog
		    exit 1
	    fi
	    sccs delta $file < $commentfile >> $logfile 2>&1
            if [ $? != 0 ]; then
		    echo ERROR - sccs delta -r$rev $file
		    errlog
		    exit 1
	    fi
            echo checked into SCCS
	fi
	if [ "$do_v6" = "-V6" ]; then
		sed -e "s;^d D $rev ..../../.. ..:..:..+0000 [^ ][^ ]*;d D $rev $date $author;" SCCS/s.$file > $tmpfile
	else
		sed -e "s;^d D $rev ../../.. ..:..:.. [^ ][^ ]*;d D $rev $date $author;" SCCS/s.$file > $tmpfile
	fi
	rm -f SCCS/s.$file
	cp $tmpfile SCCS/s.$file
	chmod 444 SCCS/s.$file
	if [ "$exec" = TRUE ]; then
		chmod +x SCCS/s.$file
	fi
	sccs admin -z $file
        if [ $? != 0 ]; then
		echo ERROR - sccs admin -z
		exit 1
	fi
    done
    if [ -s "$revfile" ]; then
	rm -f $file
    else
	echo No Revisions in "$vfile"
	ex_code=1
	do_rm=FALSE
    fi
}

############################################################
# Loop over every RCS file in RCS dir
#
if [ $# -gt 0 ]; then
	for vfile in "$@"; do
		rcs_to_sccs "$vfile"
	done
else
	for vfile in *,v; do
		rcs_to_sccs "$vfile"
	done
fi

############################################################
# Clean up
#
echo cleaning up...
rm -f $logfile $tmpfile $emptyfile $initialfile $sedfile $revfile $commentfile
echo ===================================================
echo "       Conversion Completed Successfully"
echo ===================================================

if [ "$do_rm" = TRUE ]; then
	if [ $# -gt 0 ]; then
		rm -f "$@"
	else
		rm -f *,v
	fi
fi

exit $ex_code
