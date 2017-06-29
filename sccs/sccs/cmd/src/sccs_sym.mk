#ident @(#)sccs_sym.mk	1.4 17/06/28 
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

uninstall:
		$(RM) $(RM_FORCE) $(PSYMLINKS)

$(PSYMLINKS):	$(DEST_DIR)$(INS_BASE)/ccs/bin/$(TARGET)$(_EXEEXT)
	$(MKDIR) -p $(DEST_DIR)$(INS_BASE)/$(INSDIR)
	@echo "	==> INSTALLING symlink \"$@\""; $(RM) $(RM_FORCE) $@; $(SYMLINK) ../ccs/bin/$(TARGET)$(_EXEEXT) $@
