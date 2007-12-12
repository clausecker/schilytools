#ident @(#)sccs_sym.mk	1.1 07/12/12 
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		sccs
PSYMLINKS=	$(DEST_DIR)$(INS_BASE)/$(INSDIR)/$(TARGET)$(_EXEEXT)

install: $(PSYMLINKS)

$(PSYMLINKS):	$(DEST_DIR)$(INS_BASE)/ccs/bin/$(TARGET)$(_EXEEXT)
	@echo "	==> INSTALLING symlink \"$@\""; $(RM) $(RM_FORCE) $@; $(SYMLINK) ../ccs/bin/$(TARGET)$(_EXEEXT) $@
