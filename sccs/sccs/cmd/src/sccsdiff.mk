#ident @(#)sccsdiff.mk	1.2 11/04/03 
###########################################################################
# Sample makefile for installing localized shell scripts
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#INSDIR=	sccs
INSDIR=		ccs/bin
INSMODE=	0755
TARGET=		sccsdiff
SCRFILE=	sccsdiff.sh
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.scr
###########################################################################

CPPOPTS +=	-I../../hdr

LOCALIZE=	@echo "	==> LOCALIZING \"$@\""; $(RM_F) $@; \
	SVERS=`$(CPP) $(CPPFLAGS) version.c | grep '^version' | awk '{ print $$2 }' | sed 's/"//g'`\
	SPROV=`$(CPP) $(CPPFLAGS) version.c | grep '^provider' | awk '{ print $$2 }' | sed 's/"//g'`\
	SHOST=`$(CPP) $(CPPFLAGS) version.c | grep '^host_sub' | awk '{ print $$2 }' | sed 's/"//g'`\
	export SVERS SPROV;\
	sed "s/VERSION/$$SVERS/;s/PROVIDER/$$SPROV/;s/HOST_SUB/$$SHOST/" $(SRCFILE) > $@ ; :
