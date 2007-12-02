#ident %W% %E% %Q%
###########################################################################
# Sample makefile for installing non-localized auxiliary files
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

install:
	rm -rf  $(SRCROOT)/$(PROTODIR)/$(INS_BASE)/share/doc/star/testscripts
	mkdir -p $(SRCROOT)/$(PROTODIR)/$(INS_BASE)/share/doc/star/testscripts
	chmod 755 $(SRCROOT)/$(PROTODIR)/$(INS_BASE)/share/doc/star/testscripts
	cp -p testscripts/* $(SRCROOT)/$(PROTODIR)/$(INS_BASE)/share/doc/star/testscripts || true

#INSDIR=		share/doc/cdrecord
#TARGET=		README.copy
##XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.pkg
#include		$(SRCROOT)/$(RULESDIR)/rules.aux
###########################################################################
