# @(#)Makefile	1.10 19/02/10 Copyright 1999-2019 J. Schilling

#CFLAGS=	-Iinclude -I. -DNO_FLOATINGPOINT -DNO_SCANSTACK -DNO_USG_STDIO -DNO_SYSINFO $(CDEFS)

CFLAGS=	-Iinclude -I. -DSCHILY_BUILD -DSCHILY_PRINT -DNO_FLOATINGPOINT -DNO_FPRFORMAT -DNO_NL_ARGS -DNO_SCANSTACK -DNO_USG_STDIO $(CDEFS)

o=o

OFILES=make.$o readfile.$o parse.$o update.$o rules.$o archconf.$o job.$o memory.$o \
	astoi.$o astoll.$o comerr.$o error.$o \
	eaccess.$o \
	format.$o \
	getexecpath.$o \
	jsprintf.$o jssnprintf.$o \
	saveargs.$o strcatl.$o strlcpy.$o strncpy.$o strnlen.$o \
	streql.$o getargs.$o geterrno.$o seterrno.$o \
	movebytes.$o fillbytes.$o raisecond.$o rename.$o \
	searchinpath.$o file_raise.$o \
	fileopen.$o fcons.$o cvmod.$o flag.$o flush.$o dat.$o \
	filewrite.$o fileread.$o fileseek.$o niread.$o \
	niwrite.$o

smake: $(OFILES)
	$(CC) -o $@ $(OFILES)

.SUFFIXES: .obj
.c.o:
	$(CC) -c $(CFLAGS) $<

.c.obj:
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f xconfig.h rules.cnf *.$o config.cache config.log config.status
