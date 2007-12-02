#ident "@(#)rules.prg	1.15 06/07/07 "
###########################################################################
# Written 1996 by J. Schilling
###########################################################################
#
# Generic rules for program names
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
#
# This file holds definitions that are common to all architectures.
# It should be included first and then partially overwritten,
# if the current architecture requires some changes.
#
###########################################################################
#
# Use the object file extension from the autoconf run for '$o' (.o).
# It may be overwritten my the compiler configuration rules cc-*.rul
#
###########################################################################
o=		$(OBJEXT)

CLEAN_FILES=	core err

SHELL=		/bin/sh
LN=		/bin/ln
SYMLINK=	/bin/ln -s
RM=		/bin/rm
MV=		/bin/mv
LORDER=		lorder
TSORT=		tsort
CTAGS=		vctags
ETAGS=		etags
UMASK=		umask $(UMASK_VAL)
UMASK_DEF=	002

RM_FORCE=	-f
RM_RECURS=	-r
RM_RF=		$(RM_RECURS) $(RM_FORCE)

RM_F=		$(RM) $(RM_FORCE)

INSMODEF_DEF=	444
INSMODEX_DEF=	755
INSUSR_DEF=	bin
INSGRP_DEF=	bin

_DEFUMASK=	$(_UNIQ)$(DEFUMASK)
__DEFUMASK=	$(_DEFUMASK:$(_UNIQ)=$(UMASK_DEF))
UMASK_VAL=	$(__DEFUMASK:$(_UNIQ)%=%)

_DEFINSMODEF=	$(_UNIQ)$(DEFINSMODEF)
__DEFINSMODEF=	$(_DEFINSMODEF:$(_UNIQ)=$(INSMODEF_DEF))
INSMODEF=	$(__DEFINSMODEF:$(_UNIQ)%=%)

_DEFINSMODEX=	$(_UNIQ)$(DEFINSMODEX)
__DEFINSMODEX=	$(_DEFINSMODEX:$(_UNIQ)=$(INSMODEX_DEF))
INSMODEX=	$(__DEFINSMODEX:$(_UNIQ)%=%)

_DEFINSUSR=	$(_UNIQ)$(DEFINSUSR)
__DEFINSUSR=	$(_DEFINSUSR:$(_UNIQ)=$(INSUSR_DEF))
INSUSR=		$(__DEFINSUSR:$(_UNIQ)%=%)

_DEFINSGRP=	$(_UNIQ)$(DEFINSGRP)
__DEFINSGRP=	$(_DEFINSGRP:$(_UNIQ)=$(INSGRP_DEF))
INSGRP=		$(__DEFINSGRP:$(_UNIQ)%=%)


LD=		@echo "	==> LINKING   \"$@\""; ld
LOCALIZE=	@echo "	==> LOCALIZING \"$@\""; $(RM_F) $@; cp
INSTALL=	@echo "	==> INSTALLING \"$@\""; sh $(SRCROOT)/conf/install-sh -c -m $(INSMODEINS) -o $(INSUSR) -g $(INSGRP)
CHMOD=		@echo "	==> SEETING PERMISSIONS ON \"$@\""; chmod
CHOWN=		@echo "	==> SETTING OWNER ON \"$@\""; chown
CHGRP=		@echo "	==> SETTING GROUP ON \"$@\""; chgrp
AR=		@echo "	==> ARCHIVING  \"$@\""; ar
ARFLAGS=	cr
#YACC=		@echo "	==> YACCING \"$@\""; yacc
#LEX=		@echo "	==> LEXING \"$@\""; lex
#AWK=		@echo "	==> AWKING \"$@\""; awk
RANLIB=		@echo "	==> RANDOMIZING ARCHIVE \"$@\""; true
MKDEP=		@echo "	==> MAKING DEPENDENCIES \"$@\""; makedepend
MKDEP_OUT=	-f -
MKDIR=		@echo "	==> MAKING DIRECTORY \"$@\""; $(UMASK); mkdir
MKDIR_SH=	@echo "	==> MAKING DIRECTORY \"$@\""; $(UMASK); sh $(SRCROOT)/conf/mkdir-sh
