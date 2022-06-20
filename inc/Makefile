#ident "@(#)Makefile	1.4 20/07/08 "
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MK_FILES= align_test.mk avoffset.mk

$(ALLTARGETS): align_test.c
$(SRCROOT)/$(RULESDIR)/rules.mks: align_test.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.mks
###########################################################################
