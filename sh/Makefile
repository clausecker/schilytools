#ident "@(#)Makefile	1.90 15/10/24  Copyright (c) 2006-2015 J. Schilling"
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
#CPPOPTS +=	-DNO_WAITID		# Don't use waitid()

CPPOPTS +=	-D_iBCS2 				# SCO echo compat
CPPOPTS +=	-DTEXT_DOMAIN='"SUNW_OST_OSCMD"'	# gettext()
CPPOPTS +=	-D_TS_ERRNO 				# Enable multi-threaded errno
CPPOPTS +=	-DACCT					# Shell Accounting
#CPPOPTS +=	RES			# "Research" include "login", disable others

CPPOPTS +=	-DDO_SHRCFILES		# Enable rcfiles "/etc/sh.shrc" "$HOME/.shrc"
CPPOPTS +=	-DDO_HASHCMDS		# Include hash cmds (line starts with #)
CPPOPTS +=	-DDO_HOSTPROMPT		# Set PS1 to "hostname uname> "
CPPOPTS +=	-DDO_SYSALIAS		# Include alias/unalias builtin
CPPOPTS +=	-DDO_SYSALLOC		# Include the "alloc" debug builtin
CPPOPTS +=	-DDO_SYSREPEAT		# Include the "repeat" builtin
CPPOPTS +=	-DDO_SYSDOSH		# Include the "dosh" builtin
CPPOPTS +=	-DDO_SYSPUSHD		# Include pushd / popd / dirs builtin && cd -
CPPOPTS +=	-DDO_SYSTRUE		# Include true / false builtin
CPPOPTS +=	-DDO_EXEC_AC		# Include support for exec -c -a
CPPOPTS +=	-DDO_READ_R		# Include support for read -r
CPPOPTS +=	-DDO_SET_O		# Include support for set -o
CPPOPTS +=	-DDO_MULTI_OPT		# Include support for sh -v -x / set -v -x
CPPOPTS +=	-DDO_UMASK_S		# Include support for umask -S
CPPOPTS +=	-DDO_GETOPT_LONGONLY	# Include support for getopts "?900?(lo)"
CPPOPTS +=	-DDO_POSIX_FOR		# Support for i; do .... with semicolon
CPPOPTS +=	-DDO_POSIX_TYPE		# Report keywords as well
CPPOPTS +=	-DDO_PIPE_SEMI_SYNTAX_E	# Report a syntax error for "echo foo |;"
CPPOPTS +=	-DDO_PIPE_SYNTAX_E	# Report a syntax error for "echo foo |"
CPPOPTS +=	-DDO_READ_ALLEXPORT	# Bugfix for set -a; read VAR
CPPOPTS +=	-DDO_O_APPEND		# Support O_APPEND instead of lseek() for >>
CPPOPTS +=	-DDO_EXPAND_DIRSLASH	# Expand dir*/ to dir/
CPPOPTS +=	-DDO_SIGNED_EXIT	# Allow negative exit(1) parameters
CPPOPTS +=	-DDO_DOT_SH_PARAMS	# Include support for ${.sh.xxx} parapeters
CPPOPTS +=	-DDO_POSIX_SPEC_BLTIN	# Only special builtins keep var assignment
CPPOPTS +=	-DDO_POSIX_FAILURE	# Only special builtins exit() on errors
CPPOPTS +=	-DDO_POSIX_EXIT		# Use POSIX exit codes 126/127
CPPOPTS +=	-DDO_POSIX_EXPORT	# Support export/readonly -p name=value
CPPOPTS +=	-DDO_POSIX_UNSET	# Support unset -f / -v
CPPOPTS +=	-DDO_POSIX_TEST		# Implement POSIX test -e & text -S
CPPOPTS +=	-DDO_POSIX_TRAP		# Implement POSIX trap -- for output
CPPOPTS +=	-DDO_EXT_TEST		# Implement extended test features
CPPOPTS +=	-DDO_TILDE		# Include support for tilde expansion
CPPOPTS +=	-DDO_BGNICE		# Include support for set -o bgnice
CPPOPTS +=	-DDO_TIME		# Include support for set -o time
CPPOPTS +=	-DDO_FULLEXCODE		# Include support for set -o fullexitcode
CPPOPTS +=	-DDO_NOCLOBBER		# Include support for set -o noclobber
CPPOPTS +=	-DDO_NOTIFY		# Include support for set -o notify
CPPOPTS +=	-DDO_FDPIPE		# Include support for 2| for stderr pipe
CPPOPTS +=	-DDO_KILL_L_SIG		# Include support for kill -l signo
CPPOPTS +=	-DDO_PS34		# Include support for PS3 and PS4
CPPOPTS +=	-DDO_PPID		# Include support for POSIX PPID
CPPOPTS +=	-DDO_LINENO		# Include support for POSIX LINENO
CPPOPTS +=	-DDO_ULIMIT_OPTS	# Add options to the ulimit(1) output
CPPOPTS +=	-DDO_SYSBUILTIN		# Include the "builtin" builtin
CPPOPTS +=	-DDO_SYSATEXPR		# Include the "@" builtin
CPPOPTS +=	-DDO_SYSSYNC		# Include the "sync" builtin
CPPOPTS +=	-DDO_SYSPGRP		# Include the "pgrp" builtin
CPPOPTS +=	-DDO_SYSKILLPG		# Include the "killpg" builtin
CPPOPTS +=	-DDO_SYSERRSTR		# Include the "errstr" builtin
CPPOPTS +=	-DDO_SYSFIND		# Include the "find" builtin
LIB_FIND +=	-lfind			# Add libfind

#CPPOPTS +=	-DPARSE_DEBUG		# Include debug code/messages for parser
#CPPOPTS +=	-DSTAK_DEBUG		# Include debug code for stak.c
CPPOPTS +=	-DTOSSGROWING_MACRO	# Make stak.c::tossgrowing() a macro
#CPPOPTS +=	-DTOSSCHECK		# Debug / verify magic in tossgrowing()
#CPPOPTS +=	-DARGS_RIGHT_TO_LEFT	# Evaluate var2=val2 var1=val1 left to right
#CPPOPTS +=	-DMY_GMATCH		# Enforce to use our local gmatch()
					# instead if the gmatch() from -lgen


CPPOPTS +=	-DINTERACTIVE		# Include command line history editor
CPPOPTS +=	-DINT_DOLMINUS		# Auto set -i for interactive shell

#CPPOPTS +=	-DNO_SIGSEGV		# Do not install a SIGSEGV handler for debug

CFILES=		abbrev.c args.c bltin.c cmd.c ctype.c defs.c echo.c error.c \
		expand.c fault.c func.c hash.c hashserv.c io.c jobs.c macro.c \
		main.c msg.c name.c print.c pwd.c service.c \
		sh_policy.c stak.c string.c test.c ulimit.c word.c xec.c \
		signames.c gmatch.c umask.c alias.c builtin.c find.c \
		hashcmd.c hashtab.c \
		optget.c

HFILES=		abbrev.h ctype.h defs.h dup.h hash.h mac.h mode.h name.h \
		sh_policy.h stak.h sym.h timeout.h version.h hashtab.h

# -lgen		fuer gmatch()
# -lsecdb	fuer getexecuser() free_execattr()
# $(LIB_ACL_TEST) fuer libfind
#
LIBS=		$(LIB_GEN) $(LIB_SECDB) -lshelledit -lxtermcap -lgetopt \
		$(LIB_FIND) \
		-lschily \
		$(LIB_ACL_TEST) \
		$(LIB_INTL) $(LIB_DL)
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


