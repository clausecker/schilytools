#ident @(#)sccs_sym.mk	1.2 08/01/06 
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
	$(MKDIR) -p $(DEST_DIR)$(INS_BASE)/$(INSDIR)
	@echo "	==> INSTALLING symlink \"$@\""; $(RM) $(RM_FORCE) $@; $(SYMLINK) ../ccs/bin/$(TARGET)$(_EXEEXT) $@
