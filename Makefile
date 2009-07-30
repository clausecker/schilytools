#ident @(#)Makefile	1.5 08/09/10 
###########################################################################
SRCROOT=	.
DIRNAME=	SRCROOT
RULESDIR=	RULES
###########################################################################
all:
all clean depend config install: RULES autoconf libedc libschily readcd smake

RULES autoconf libedc libschily readcd smake:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS

$(SRCROOT)/$(RULESDIR)/rules.top: RULES
###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

#
# include Targetdirs no longer needed, we use SSPM
#include		$(SRCROOT)/TARGETS/Targetdirs
#include		$(SRCROOT)/TARGETS/Targetdirs.$(M_ARCH)

###########################################################################
# Due to a bug in SunPRO make we need special rules for the root makefile
#
include		$(SRCROOT)/$(RULESDIR)/rules.rdi
###########################################################################
