#ident "@(#)rules.prg	1.24 21/04/28 "
###########################################################################
# Written 1996-2017 by J. Schilling
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
# A copy of the CDDL is also available via the Internet at
# http://www.opensource.org/licenses/cddl1.txt
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

#
# Setting $(SHELL) inside a makefile is a really bad idea.
# Since we allow "smake" to default SHELL to /bin/bosh in
# case that /bin/sh is broken but /bin/bosh exists, this
# must not be defined anymore.
#
#SHELL=		/bin/sh

LN=		/bin/ln
SYMLINK=	/bin/ln -s
RM=		/bin/rm
MV=		/bin/mv
LORDER=		$(LORDER_PROG)
TSORT=		$(TSORT_PROG)
CTAGS=		vctags
ETAGS=		etags
UMASK=		umask $(UMASK_VAL)
UMASK_DEF=	002
INSUMASK=	umask $(INSUMASK_VAL)
INSUMASK_DEF=	022

RM_FORCE=	-f
RM_RECURS=	-r
RM_RF=		$(RM_RECURS) $(RM_FORCE)

RM_F=		$(RM) $(RM_FORCE)

INSMODEF_DEF=	444
INSMODED_DEF=	755
INSMODEX_DEF=	755
INSUSR_DEF=	root
INSGRP_DEF=	bin

_DEFINSUMASK=	$(_UNIQ)$(DEFINSUMASK)
__DEFINSUMASK=	$(_DEFINSUMASK:$(_UNIQ)=$(INSUMASK_DEF))
INSUMASK_VAL=	$(__DEFINSUMASK:$(_UNIQ)%=%)

_DEFUMASK=	$(_UNIQ)$(DEFUMASK)
__DEFUMASK=	$(_DEFUMASK:$(_UNIQ)=$(UMASK_DEF))
UMASK_VAL=	$(__DEFUMASK:$(_UNIQ)%=%)

_DEFINSMODEF=	$(_UNIQ)$(DEFINSMODEF)
__DEFINSMODEF=	$(_DEFINSMODEF:$(_UNIQ)=$(INSMODEF_DEF))
INSMODEF=	$(__DEFINSMODEF:$(_UNIQ)%=%)

_DEFINSMODED=	$(_UNIQ)$(DEFINSMODED)
__DEFINSMODED=	$(_DEFINSMODED:$(_UNIQ)=$(INSMODED_DEF))
INSMODED=	$(__DEFINSMODED:$(_UNIQ)%=%)

_DEFINSMODEX=	$(_UNIQ)$(DEFINSMODEX)
__DEFINSMODEX=	$(_DEFINSMODEX:$(_UNIQ)=$(INSMODEX_DEF))
INSMODEX=	$(__DEFINSMODEX:$(_UNIQ)%=%)

_DEFINSUSR=	$(_UNIQ)$(DEFINSUSR)
__DEFINSUSR=	$(_DEFINSUSR:$(_UNIQ)=$(INSUSR_DEF))
INSUSR=		$(__DEFINSUSR:$(_UNIQ)%=%)

_DEFINSGRP=	$(_UNIQ)$(DEFINSGRP)
__DEFINSGRP=	$(_DEFINSGRP:$(_UNIQ)=$(INSGRP_DEF))
INSGRP=		$(__DEFINSGRP:$(_UNIQ)%=%)


LD=		$(NOECHO)echo "	==> LINKING   \"$@\""; ld
LOCALIZE=	$(NOECHO)echo "	==> LOCALIZING \"$@\""; $(RM_F) $@; cp
INSTALL=	$(NOECHO)echo "	==> INSTALLING \"$@\""; sh $(SRCROOT)/conf/install-sh -c -m $(INSMODEINS) -o $(INSUSR) -g $(INSGRP)
CHMOD=		$(NOECHO)echo "	==> SETTING PERMISSIONS ON \"$@\""; chmod
CHOWN=		$(NOECHO)echo "	==> SETTING OWNER ON \"$@\""; chown
CHGRP=		$(NOECHO)echo "	==> SETTING GROUP ON \"$@\""; chgrp
AR=		$(NOECHO)echo "	==> ARCHIVING  \"$@\""; ar
ARFLAGS=	cr
#YACC=		$(NOECHO)echo "	==> YACCING \"$@\""; yacc
#LEX=		$(NOECHO)echo "	==> LEXING \"$@\""; lex
#AWK=		$(NOECHO)echo "	==> AWKING \"$@\""; awk
RANLIB=		$(NOECHO)echo "	==> RANDOMIZING ARCHIVE \"$@\""; true
MKDEP=		$(NOECHO)echo "	==> MAKING DEPENDENCIES \"$@\""; makedepend
MKDEP_OUT=	-f -
_MKDIR=		$(UMASK); mkdir
MKDIR=		$(NOECHO)echo "	==> MAKING DIRECTORY \"$@\""; $(UMASK); mkdir
_MKDIR_SH=	$(UMASK); sh $(SRCROOT)/conf/mkdir-sh
MKDIR_SH=	$(NOECHO)echo "	==> MAKING DIRECTORY \"$@\""; $(UMASK); sh $(SRCROOT)/conf/mkdir-sh
INSMKDIR=	$(NOECHO)echo "	==> MAKING DIRECTORY \"$@\""; $(INSUMASK); mkdir
INSMKDIR_SH=	$(NOECHO)echo "	==> MAKING DIRECTORY \"$@\""; $(INSUMASK); sh $(SRCROOT)/conf/mkdir-sh
