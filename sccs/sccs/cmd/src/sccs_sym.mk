#ident @(#)sccs_sym.mk	1.7 18/04/04 
###########################################################################
SRCROOT=	../../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################
_EXEEXT=	$(EXEEXT)

#PREINSDIR=	$(SCCS_BIN_PRE)
SCCS_HELP_PRE=	ccs/
SCCS_BIN_PRE=	ccs/
INSDIR=		bin
TARGET=		sccs
PSYMLINKS=	$(DEST_DIR)$(INS_BASE)/$(PREINSDIR)$(INSDIR)$(SUBINSDIR)$(SUBINS)/$(TARGET)$(_EXEEXT)

install:

_IN_PRE=	$(_UNIQ)$(SCCS_BIN_PRE)
__IN_PRE=	$(_IN_PRE:$(_UNIQ)=no-)
IN_PRE=		$(__IN_PRE:$(_UNIQ)%=)

$(IN_PRE)install: $(PSYMLINKS)

uninstall:
		$(RM) $(RM_FORCE) $(PSYMLINKS)

$(PSYMLINKS):	$(DEST_DIR)$(INS_BASE)/$(SCCS_BIN_PRE)/bin/$(TARGET)$(_EXEEXT)
	$(MKDIR) -p $(DEST_DIR)$(INS_BASE)/$(PREINSDIR)$(INSDIR)
	@echo "	==> INSTALLING symlink \"$@\""; $(RM) $(RM_FORCE) $@; $(SYMLINK) ../$(SCCS_BIN_PRE)bin/$(TARGET)$(_EXEEXT) $@
