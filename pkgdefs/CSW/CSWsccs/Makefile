#ident @(#)Makefile	1.3 09/04/04 
###########################################################################
# Sample makefile for creating SVr4 packages
###########################################################################
SRCROOT=	../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

PKG_RELOC=no

include		$(SRCROOT)/$(RULESDIR)/rules.csw
###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.pkg
###########################################################################

VERSION :sh=	../../../proto/`../../../conf/oarch.sh`/opt/csw/bin/sccs -V | awk '{ print $4}'
