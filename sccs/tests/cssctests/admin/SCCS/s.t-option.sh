h17481
s 00001/00001/00037
d D 1.2 15/06/03 00:06:43 joerg 2 1
c ../common/test-common -> ../../common/test-common
e
s 00038/00000/00000
d D 1.1 10/05/11 11:30:00 joerg 1 0
c date and time created 10/05/11 11:30:00 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

# t-option.sh:  Testing for correct operation of admin -t.

# Import common functions & definitions.
D 2
. ../common/test-common
E 2
I 2
. ../../common/test-common
E 2

expands_to () {
    # $1 -- label
    # $2 -- format
    # $3 -- expansion
docommand $1 "${prs} \"-d$2\" s.bar" 0 "$3"
}

remove [sxzp].bar x.bar.bak

# Create file with description.
echo "Descriptive Text" > DESC
docommand T1 "${vg_admin} -n -tDESC s.bar" 0 "" ""
remove DESC

# Make sure the decription is there.
expands_to T2 ':FD:'   'Descriptive Text\n\n'


# Remove the description.
docommand T3 "${vg_admin} -t s.bar" 0 "" ""

# Make sure the decription has been removed.
expands_to T4 ':FD:'   'none\n\n'
remove s.bar

# Empty -t option is incompatible with -n and -i.
docommand T5 "${admin} -n -t s.bar" 1 "" IGNORE


remove [sxzp].bar  x.bar.bak
success
E 1
