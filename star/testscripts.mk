#ident @(#)testscripts.mk	1.2 09/04/14 
###########################################################################
# Sample makefile for installing non-localized auxiliary files
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

install:
	rm -rf  $(DEST_DIR)$(INSBASE)/share/doc/star/testscripts
	mkdir -p $(DEST_DIR)$(INSBASE)/share/doc/star/testscripts
	chmod 755 $(DEST_DIR)$(INSBASE)/share/doc/star/testscripts
	cp -p testscripts/* $(DEST_DIR)$(INSBASE)/share/doc/star/testscripts || true

#INSDIR=		share/doc/cdrecord
#TARGET=		README.copy
##XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.pkg
#include		$(SRCROOT)/$(RULESDIR)/rules.aux
###########################################################################
