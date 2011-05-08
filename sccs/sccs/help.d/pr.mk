#ident "@(#)pr.mk	1.1 11/04/30 "
###########################################################################
# Sample makefile for installing non-localized auxiliary files
###########################################################################
SRCROOT=	../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		ccs/lib/help/locale/C
TARGET=		pr
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.aux
###########################################################################
