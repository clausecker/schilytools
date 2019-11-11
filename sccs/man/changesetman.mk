#ident @(#)changesetman.mk	1.2 19/11/08 
###########################################################################
# Sample makefile for installing manual pages
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MANDIR=		man
TARGETMAN=	changeset
MANSECT=	$(MANSECT_FILEFORM)
MANSUFFIX=	$(MANSUFF_FILEFORM)
MANFILE=	changeset.4
LOCALIZE=	@echo "	==> LOCALIZING \"$@\""; $(RM_F) $@; \
		sh -c 'sed -e "s,man4/,$(MANSECT_FILEFORM)/," -e "s,.4$$,.$(MANSUFF_FILEFORM)," "$$1" > "$$2"' sed 

changeset.4.html: sccschangeset.4

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.man
###########################################################################
