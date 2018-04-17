#ident @(#)avoffset.mk	1.3 18/04/09 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		include/schily/$(OARCH)
TARGET=		avoffset.h
TARGETC=	avoffset
CPPOPTS +=	-DUSE_SCANSTACK
CPPOPTS +=	-D__OPRINTF__
CFILES=		avoffset.c fpoff.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.inc
###########################################################################
