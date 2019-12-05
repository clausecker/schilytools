#ident @(#)Makefile	1.3 17/05/28 
###########################################################################
# Sample makefile for creating SVr4 packages
###########################################################################
SRCROOT=	../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

include		$(SRCROOT)/$(RULESDIR)/rules.csw
###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.pkg
###########################################################################

VERSION :sh=	../../../proto/`../../../conf/oarch.sh`/opt/csw/bin/make -v | head -1 | awk '{ print $5}'
