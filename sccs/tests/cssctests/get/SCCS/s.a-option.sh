h13133
s 00107/00000/00000
d D 1.1 10/04/29 02:05:14 joerg 1 0
c date and time created 10/04/29 02:05:14 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh
# a-option.sh:  Testing for the -a option.   

# Import common functions & definitions.
. ../common/test-common


# Get a test file...
g=testfile
s=s.$g
x=x.$g
p=p.$g
z=z.$g
remove $s $g $p $z


get_expect () {
label="$1"         ; shift
r_option=$1      ; shift
a_option=$1      ; shift
sid_expected=$1  ; shift
docommand "$label" "${vg_get} ${toption} -g ${r_option} ${a_option} ${t_option} \
$s" 0 "$sid_expected\n" IGNORE
}

## Create the file (empty).
docommand prep1 "${admin} -n $s" 0 "" ""

## make some deltas.
docommand prep2 "${get} -e $s"    0  "1.1\nnew delta 1.2\n0 lines\n" ""
docommand prep3 "${delta} -y $s"  \
    0  "1.2\n0 inserted\n0 deleted\n0 unchanged\n" IGNORE
docommand prep4 "${get} -e -r2 $s"   0  "1.2\nnew delta 2.1\n0 lines\n" ""
docommand prep5 "${delta} -y $s"  \
    0  "2.1\n0 inserted\n0 deleted\n0 unchanged\n" IGNORE

docommand prep6 "${admin} -fb $s" 0 "" ""
docommand prep7 "${get} -e -b $s" 0 "2.1\nnew delta 2.1.1.1\n0 lines\n" IGNORE
docommand prep8 "${delta} -y $s" 0 \
	"2.1.1.1\n0 inserted\n0 deleted\n0 unchanged\n" IGNORE
docommand prep9 "${get} -e -r2.1.1 $s" 0 \
	"2.1.1.1\nnew delta 2.1.1.2\n0 lines\n" IGNORE
docommand prep10 "${delta} -y $s" 0 \
	"2.1.1.2\n0 inserted\n0 deleted\n0 unchanged\n" IGNORE

## Also make a branch on release 1.
docommand prep11 "${get} -e -r1.2 $s" 0 \
	"1.2\nnew delta 1.2.1.1\n0 lines\n" IGNORE
docommand prep12 "${delta} -y $s" 0 \
	"1.2.1.1\n0 inserted\n0 deleted\n0 unchanged\n" IGNORE


all_seqs() {
    ${prs} -d:DS: -l -r1.1 "$@" | sort -n
}

all_sids() {
    ${prs} -d:I: -l -r1.1 "$@"
}

sid_for_seq() {
    seq=$1
    shift
    ${prs} -d":DS: :I:" -l -r1.1 "$@" | grep "^${seq} " | while read seq sid
    do
      echo "$sid"
    done
}

seq_for_sid() {
    ${prs} -d":DS:" -r$1 "$2"
}


# Do various forms of get on the file and make sure we get the right SID.
seqlist=`all_seqs $s`
sidlist=`all_sids $s`

for sid in $sidlist
do
  for gotsid in $sidlist
  do
    gotseq=`seq_for_sid $gotsid $s` || \
      miscarry "Cannot find sequence number for SID $gotsid"
    for t_option in "  " "-t"
    do
      get_expect "ar${gotseq}-${sid}${t_option}" -r${sid} -a${gotseq} "${gotsid}"
    done
  done
done

for gotsid in $sidlist
do
  gotseq=`seq_for_sid $gotsid $s` || \
    miscarry "Cannot find sequence number for SID $gotsid"
  for t_option in "  " "-t"
  do
    get_expect  "a${gotseq}${t_option}" "" -a${gotseq} "${gotsid}"
  done
done

# None of the above commands should have left a g-file lying around.
docommand g1 "test -f $g" 1 "" IGNORE

remove $s $g

success
E 1
