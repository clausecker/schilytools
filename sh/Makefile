#ident "@(#)Makefile	1.25 12/06/10  Copyright (c) 2006-2012 J. Schilling"
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

CPPOPTS +=	-DBOURNE_SHELL		# Tell the code that we compile for sh
CPPOPTS +=	-DUSE_LARGEFILES	# Allow Large Files (> 2 GB)
CPPOPTS +=	-DUSE_NLS
#CPPOPTS +=	-DNO_LOCALE		# Don't use setlocale()
#CPPOPTS +=	-DNO_WCHAR		# Don't use wide chars

CPPOPTS +=	-D_iBCS2 				# SCO echo compat
CPPOPTS +=	-DTEXT_DOMAIN='"SUNW_OST_OSCMD"'	# gettext()
CPPOPTS +=	-D_TS_ERRNO 				# Enable multi-threaded errno
CPPOPTS +=	-DACCT					# Shell Accounting
#CPPOPTS +=	RES			# "Research" include "login", disable others

CPPOPTS +=	-DDO_SYSALLOC		# Include the "alloc" debug builtin
#CPPOPTS +=	-DSTAK_DEBUG		# Include debug code for stak.c
#CPPOPTS +=	-DARGS_RIGHT_TO_LEFT	# Evaluate var2=val2 var1=val1 left to right
#CPPOPTS +=	-DMY_GMATCH		# Enforce to use our local gmatch()
					# instead if the gmatch() from -lgen


CPPOPTS +=	-DINTERACTIVE		# Include command line history editor

CFILES=		abbrev.c args.c bltin.c cmd.c ctype.c defs.c echo.c error.c \
		expand.c fault.c func.c hash.c hashserv.c io.c jobs.c macro.c \
		main.c msg.c name.c print.c pwd.c service.c \
		sh_policy.c stak.c string.c test.c ulimit.c word.c xec.c \
		signames.c gmatch.c umask.c alias.c

HFILES=		abbrev.h brkincr.h ctype.h defs.h dup.h hash.h mac.h mode.h name.h \
		sh_policy.h stak.h sym.h timeout.h version.h

#LIBS=		-lunos
#OOOLIBS=		-lxtermcap -lschily $(LIB_CRYPT)

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


