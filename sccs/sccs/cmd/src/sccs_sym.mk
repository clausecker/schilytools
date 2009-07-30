#ident @(#)sccs_sym.mk	1.3 09/07/28 
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################
_EXEEXT=	$(EXEEXT)

INSDIR=		bin
TARGET=		sccs
PSYMLINKS=	$(DEST_DIR)$(INS_BASE)/$(INSDIR)/$(TARGET)$(_EXEEXT)

install: $(PSYMLINKS)

$(PSYMLINKS):	$(DEST_DIR)$(INS_BASE)/ccs/bin/$(TARGET)$(_EXEEXT)
	$(MKDIR) -p $(DEST_DIR)$(INS_BASE)/$(INSDIR)
	@echo "	==> INSTALLING symlink \"$@\""; $(RM) $(RM_FORCE) $@; $(SYMLINK) ../ccs/bin/$(TARGET)$(_EXEEXT) $@
