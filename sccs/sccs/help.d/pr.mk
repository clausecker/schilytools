#ident "@(#)pr.mk	1.2 18/03/28 "
###########################################################################
# Sample makefile for installing non-localized auxiliary files
###########################################################################
SRCROOT=	../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

PREINSDIR=	$(SCCS_HELP_PRE)
SCCS_HELP_PRE=	ccs/
INSDIR=		lib/help/locale/C
TARGET=		pr
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.aux
###########################################################################
