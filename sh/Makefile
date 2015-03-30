#ident "@(#)Makefile	1.44 15/03/29  Copyright (c) 2006-2015 J. Schilling"
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
					# Shell
TARGET=		sh
SYMLINKS=	pfsh			# Profile Bourne Shell
SYMLINKS +=	bosh			# Bo urne Shell
SYMLINKS +=	jsh			# Job control Bourne Shell

##CPPOPTS +=	-DNO_USER_MALLOC	# Do not use our own malloc()

CPPOPTS +=	-DSCHILY_INCLUDES	# Tell the code to use schily include files
CPPOPTS +=	-DBOURNE_SHELL		# Tell the code that we compile for sh
CPPOPTS +=	-DUSE_LARGEFILES	# Allow Large Files (> 2 GB)
CPPOPTS +=	-DUSE_NLS		# Enable NLS support in include/schily/*.h
CPPOPTS +=	-DUSE_JS_BOOL		# Allow to use schily/dbgmalloc.h
#CPPOPTS +=	-DNO_LOCALE		# Don't use setlocale()
#CPPOPTS +=	-DNO_WCHAR		# Don't use wide chars
#CPPOPTS +=	-DNO_VFORK		# Don't use vfork()

CPPOPTS +=	-D_iBCS2 				# SCO echo compat
CPPOPTS +=	-DTEXT_DOMAIN='"SUNW_OST_OSCMD"'	# gettext()
CPPOPTS +=	-D_TS_ERRNO 				# Enable multi-threaded errno
CPPOPTS +=	-DACCT					# Shell Accounting
#CPPOPTS +=	RES			# "Research" include "login", disable others

CPPOPTS +=	-DDO_SHRCFILES		# Enable rcfiles "/etc/sh.shrc" "$HOME/.shrc"
CPPOPTS +=	-DDO_SYSALIAS		# Include alias/unalias builtin
CPPOPTS +=	-DDO_SYSALLOC		# Include the "alloc" debug builtin
CPPOPTS +=	-DDO_SYSREPEAT		# Include the "repeat" builtin
CPPOPTS +=	-DDO_SYSDOSH		# Include the "dosh" builtin
CPPOPTS +=	-DDO_SYSPUSHD		# Include pushd / popd / dirs builtin && cd -
CPPOPTS +=	-DDO_SYSTRUE		# Include true / false builtin
CPPOPTS +=	-DDO_READ_R		# Include support for read -r
CPPOPTS +=	-DDO_SET_O		# Include support for set -o
CPPOPTS +=	-DDO_UMASK_S		# Include support for umask -S
CPPOPTS +=	-DDO_POSIX_FOR		# Support for i; do .... with semicolon
CPPOPTS +=	-DDO_READ_ALLEXPORT	# Bugfix for set -a; read VAR

#CPPOPTS +=	-DSTAK_DEBUG		# Include debug code for stak.c
CPPOPTS +=	-DTOSSGROWING_MACRO	# Make stak.c::tossgrowing() a macro
#CPPOPTS +=	-DTOSSCHECK		# Debug / verify magic in tossgrowing()
#CPPOPTS +=	-DARGS_RIGHT_TO_LEFT	# Evaluate var2=val2 var1=val1 left to right
#CPPOPTS +=	-DMY_GMATCH		# Enforce to use our local gmatch()
					# instead if the gmatch() from -lgen


CPPOPTS +=	-DINTERACTIVE		# Include command line history editor

#CPPOPTS +=	-DNO_SIGSEGV		# Do not install a SIGSEGV handler for debug

CFILES=		abbrev.c args.c bltin.c cmd.c ctype.c defs.c echo.c error.c \
		expand.c fault.c func.c hash.c hashserv.c io.c jobs.c macro.c \
		main.c msg.c name.c print.c pwd.c service.c \
		sh_policy.c stak.c string.c test.c ulimit.c word.c xec.c \
		signames.c gmatch.c umask.c alias.c

HFILES=		abbrev.h ctype.h defs.h dup.h hash.h mac.h mode.h name.h \
		sh_policy.h stak.h sym.h timeout.h version.h

# -lgen		fuer gmatch()
# -lsecdb	fuer getexecuser() free_execattr()
#
LIBS=		$(LIB_GEN) $(LIB_SECDB) -lshelledit -lxtermcap -lgetopt -lschily \
		$(LIB_INTL)
#LIBS=		-lgen -lsecdb 
XMK_FILE=	Makefile.man bosh.mk1 jsh.mk1 pfsh.mk1

signames.c abbrev.c abbrev.h:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
$(ALLTARGETS): signames.c abbrev.c abbrev.h

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count: $(HFILES) $(CFILES) 
	count $r1


