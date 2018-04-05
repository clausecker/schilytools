#ident @(#)default.mk	1.3 18/03/28 
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
TARGET=		default
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.aux
###########################################################################
