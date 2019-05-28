h05096
s 00012/00000/00088
d D 1.6 19/05/16 21:35:51 joerg 6 5
c Hinweis auf date probleme in betriebssystemen
e
s 00005/00000/00083
d D 1.5 19/05/12 14:12:21 joerg 5 4
c prs -D:DI: unterscheidet nun fuer non-POSIX Varianten
c Kommentar verbessert
e
s 00002/00002/00081
d D 1.4 15/06/03 00:06:44 joerg 4 3
c ../common/test-common -> ../../common/test-common
e
s 00003/00001/00080
d D 1.3 11/05/31 21:53:45 joerg 3 2
c expect_fail=true um den Abbruch auf einigen Plattformen zu vermeiden
e
s 00028/00002/00053
d D 1.2 11/05/30 01:16:59 joerg 2 1
c Neue Tests mit SCCS 4-digit cut-off (z.B. -c1968/1231235959)
e
s 00055/00000/00000
d D 1.1 11/04/26 11:44:52 joerg 1 0
c date and time created 11/04/26 11:44:52 by joerg
e
u
U
f e 0
f y 
t
T
I 1
#! /bin/sh

#################################################################
###          WARNING: this test is CSSC-specific!             ###
#################################################################

# ext.sh:       Testing for the century-specification
#               of CSSC.  This is an extension; other
#               SCCS implementations do not do this.

# Import common functions & definitions.
D 4
. ../common/test-common
. ../common/real-thing
E 4
I 4
. ../../common/test-common
. ../../common/real-thing
E 4

I 5
# $TESTING_SCCS_V5	Test SCCSv5 features from SunOS
# $TESTING_CSSC		Relict from CSSC tests, also applies to SCCS
# $TESTING_REAL_CSSC	Test real CSSC 4-digit year extensions
# $TESTING_REAL_SCCS	Test real Schily SCCS 4 digit year extensions
# $TESTING_SCCS_V6	Test SCCSv6 features
E 5

s=s.y2k.txt

brief='"-d:I: :D: :T:"'

r1_5="1.5 68/12/31 23:59:59\n" # 2068: the last year we have
r1_4="1.4 00/02/29 00:00:00\n" # Year 2000 is a leap year.
r1_3="1.3 00/01/01 00:00:00\n" # Just after the milennium
r1_2="1.2 99/12/31 23:59:59\n" # Just before the milennium
r1_1="1.1 69/01/01 00:00:00\n" # 1969: the earliest year we have

allrevs="${r1_5}${r1_4}${r1_3}${r1_2}${r1_1}"


D 2
if "$TESTING_CSSC"
E 2
I 2
if "$TESTING_REAL_CSSC"
E 2
then
I 6
    # Sine CSSC does not use time stamps internally, there is no reason
    # why CSSC should fail with dates that are unsupportd by the current
    # platform.
    #
E 6
I 2
    echo Testing the CSSC century specifier.
E 2
    ## Tests for the century field.

    # Ask for exerything after the end of 1968.  Since the first
    # year we have int he s. file is 1969, we should get everything.
    docommand c1 "${vg_prs} ${brief} -l -c19681231235959  $s" 0 \
	"${allrevs}" ""

    # Ask for exerything before the end of 1968.  Since the first
    # year we have int he s. file is 1969, we should get NOTHING.
    docommand c2 "${vg_prs} ${brief} -e -c19681231235959  $s" 0 \
	"" ""

    # Ask for exerything before the end of 2069.  
    # We chould get everything.
    docommand c3 "${vg_prs} ${brief} -e -c20691231235959  $s" 0 \
	"${allrevs}" ""


else
D 2
    echo No testing done for century specifier.
E 2
I 2
    echo No testing done for CSSC century specifier.
E 2
fi

I 2
if "$TESTING_REAL_SCCS"
then
I 6
    # SCCS uses UNIX based time stamps internally. Since 32 Bit UNIX versions
    # support the time range between 1901..2038 but 32 Bit SCCS should support
    # the time range between 1969..2068, we definitely need a calendar
    # implementation that supports a negative time_t. Some platforms implement
    # bugs for time stamps before 1970 and for this reason, we can only grant
    # to support the range between 1970 and 2038. This is why we need to be
    # prepared to fail with the following tests.
    #
E 6
D 3
    echo Testing the SCCS century specifier.
E 3
I 3
    echo "Testing the SCCS century specifier."
    echo "This may fail for times before January 1 1970 on some platforms."
    expect_fail=true
E 3
    ## Tests for the century field.

    # Ask for exerything after the end of 1968.  Since the first
    # year we have int he s. file is 1969, we should get everything.
    docommand c1 "${vg_prs} ${brief} -l -c1968/1231235959  $s" 0 \
	"${allrevs}" ""

    # Ask for exerything before the end of 1968.  Since the first
    # year we have int he s. file is 1969, we should get NOTHING.
    docommand c2 "${vg_prs} ${brief} -e -c1968/1231235959  $s" 0 \
	"" ""

    # Ask for exerything before the end of 2069.  
    # We chould get everything.
    docommand c3 "${vg_prs} ${brief} -e -c2069/1231235959  $s" 0 \
	"${allrevs}" ""


else
    echo No testing done for SCCS century specifier.
fi

E 2

remove command.log
success
E 1
