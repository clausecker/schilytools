#ident @(#)rules.inc	1.8 07/05/06 
###########################################################################
# Written 1999 by J. Schilling
###########################################################################
#
# Rules for creating include files (usually found in .../incs/machdir)
#
###########################################################################
# Copyright (c) J. Schilling
###########################################################################
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only
# (the "License").  You may not use this file except in compliance
# with the License.
#
# See the file CDDL.Schily.txt in this distribution for details.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file CDDL.Schily.txt from this distribution.
###########################################################################
_EXEEXT=	$(EXEEXT)
_XEXEEXT=	$(XEXEEXT)
###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.obj
###########################################################################

PTARGET=	$(OINCSDIR)/$(TARGET)
PTARGETC=	$(ARCHDIR)/$(TARGETC)
PTARGETCX=	$(ARCHDIR)/$(TARGETC)$(_EXEEXT)

_INSMODEI=	$(_UNIQ)$(INSMODE)
__INSMODEI=	$(_INSMODEI:$(_UNIQ)=$(INSMODEF))
INSMODEI=	$(__INSMODEI:$(_UNIQ)%=%)

__LD_OUTPUT_OPTION=	$(_UNIQ)$(LD_OUTPUT_OPTION)
___LD_OUTPUT_OPTION=	$(__LD_OUTPUT_OPTION:$(_UNIQ)=-o $@)
_LD_OUTPUT_OPTION=	$(___LD_OUTPUT_OPTION:$(_UNIQ)%=%)

LIBS_PATH += $(LIBS_PATH_STATIC)

all:		$(PTARGETCX) $(PTARGET)

$(PTARGETCX):	$(OFILES) $(SRCLIBS)
		$(LDCC) $(_LD_OUTPUT_OPTION) $(POFILES) $(LDFLAGS) $(LDLIBS)

$(PTARGET):	$(OINCSDIR) $(PTARGETCX)
		@echo "	==> GENERATING include file \"$@\""; $(PTARGETC)$(_XEXEEXT) > $@

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/sub.htm
include		$(SRCROOT)/$(RULESDIR)/rules.clr
include		$(SRCROOT)/$(RULESDIR)/rules.ins
include		$(SRCROOT)/$(RULESDIR)/rules.tag
include		$(SRCROOT)/$(RULESDIR)/rules.hlp
include		$(SRCROOT)/$(RULESDIR)/rules.dep
###########################################################################
