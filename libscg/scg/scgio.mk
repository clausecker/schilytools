#ident %W% %E% %Q%
###########################################################################
# Sample makefile for installing non-localized auxiliary files
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		include/scg
TARGET=		scgio.h
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.aux
###########################################################################

IFDEF=	-UJOSxx

XRELFILES=	scgio.h scgio.mk

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.rel
###########################################################################
MAKE_LICENSE=MKCDDL
