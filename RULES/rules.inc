#ident @(#)rules.inc	1.12 21/04/28 
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
# A copy of the CDDL is also available via the Internet at
# http://www.opensource.org/licenses/cddl1.txt
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file CDDL.Schily.txt from this distribution.
###########################################################################
#_EXEEXT=	$(EXEEXT)	# Do not install as xxx.h.exe
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
#
# If calling "$(PTARGETC)$(_XEXEEXT)" fails, this may be a cross compilation,
# so retry to run the command under control of "$(CONFIG_RMTCALL)".
#
# We previously had;
# $(PTARGET):	$(OINCSDIR) $(PTARGETCX)
# but that caused $(PTARGET) not only to depend on the existence but also
# on the time stamp of the directory $(OINCSDIR), that could change.
# Since rules.top contains a rule to create $(OINCSDIR), we may safely assume
# that $(OINCSDIR) exists before we create $(PTARGET)
#
$(PTARGET):	$(PTARGETCX)
		$(NOECHO)echo "	==> GENERATING include file \"$@\""; \
		$(PTARGETC)$(_XEXEEXT) > $@ || \
		$(CONFIG_RMTCALL) $(PTARGETC)$(_XEXEEXT) > $@

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/sub.htm
include		$(SRCROOT)/$(RULESDIR)/rules.clr
include		$(SRCROOT)/$(RULESDIR)/rules.ins
include		$(SRCROOT)/$(RULESDIR)/rules.tag
include		$(SRCROOT)/$(RULESDIR)/rules.hlp
include		$(SRCROOT)/$(RULESDIR)/rules.dep
###########################################################################
