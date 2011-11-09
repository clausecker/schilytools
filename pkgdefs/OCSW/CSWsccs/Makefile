#ident @(#)Makefile	1.4 11/09/28 
###########################################################################
# Sample makefile for creating SVr4 packages
###########################################################################
SRCROOT=	../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

PKG_RELOC=no
PKGROOTX=,.

include		$(SRCROOT)/$(RULESDIR)/rules.csw
###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.pkg
###########################################################################

VERSION :sh=	../../../proto/`../../../conf/oarch.sh`/opt/csw/bin/sccs -V | awk '{ print $4}'
