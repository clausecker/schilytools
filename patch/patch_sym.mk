#ident "@(#)patch_sym.mk	1.1 11/09/05 "
###########################################################################
SRCROOT=	../
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################
_EXEEXT=	$(EXEEXT)

INSDIR=		ccs/bin
TARGET=		sccspatch
ORIG=		spatch
PSYMLINKS=	$(DEST_DIR)$(INS_BASE)/$(INSDIR)/$(TARGET)$(_EXEEXT)

install: $(PSYMLINKS)

$(PSYMLINKS):	$(DEST_DIR)$(INS_BASE)/bin/$(ORIG)$(_EXEEXT)
	$(MKDIR) -p $(DEST_DIR)$(INS_BASE)/$(INSDIR)
	@echo "	==> INSTALLING symlink \"$@\""; $(RM) $(RM_FORCE) $@; $(SYMLINK) ../../bin/$(ORIG)$(_EXEEXT) $@
