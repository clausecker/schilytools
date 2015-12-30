/* @(#)mdigest.c	1.4 15/12/27 Copyright 2009-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
/*static	const char sccsid[] =*/
	"@(#)mdigest.c	1.4 15/12/27 Copyright 2009-2015 J. Schilling";
#endif
/*
 *	Compute the message digest for files
 *
 *	Copyright (c) 2009-2015 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/utypes.h>
#include <schily/fcntl.h>
#include <schily/string.h>
#include <signal.h>
#include <schily/schily.h>
#include <schily/errno.h>
#include <schily/md4.h>
#include <schily/md5.h>
#include <schily/rmd160.h>
#include <schily/sha1.h>
#include <schily/sha2.h>
#include <schily/sha3.h>

typedef union uctx {
	MD4_CTX		ctx_md4;
	MD5_CTX		ctx_md6;
	RMD160_CTX	ctx_rmd160;
	SHA1_CTX	ctx_sha1;
	SHA2_CTX	ctx_sha2;
#ifdef	sha3_224_hash_size
	SHA3_CTX	ctx_sha3;
#endif
} U_CTX;

struct adigest {
	char	*name;
	int	digest_len;
	void	(*init)		__PR((void *));
	void	(*update)	__PR((void *, const void *, size_t));
	void	(*pad)		__PR((void *));
	void	(*final)	__PR((UInt8_t *, void *));
};

#define	PI	void (*) __PR((void *))
#define	PU	void (*) __PR((void *, const void *, size_t))
#define	PP	void (*) __PR((void *))
#define	PF	void (*) __PR((UInt8_t *, void *))

struct adigest dlist[] = {
{ "md4", MD4_DIGEST_LENGTH, (PI) MD4Init, (PU) MD4Update, (PP) MD4Pad, (PF) MD4Final },
{ "md5", MD5_DIGEST_LENGTH, (PI) MD5Init, (PU) MD5Update, (PP) MD5Pad, (PF) MD5Final },
{ "rmd160", RMD160_DIGEST_LENGTH, (PI) RMD160Init, (PU) RMD160Update, (PP) RMD160Pad, (PF) RMD160Final },
{ "sha1", SHA1_DIGEST_LENGTH, (PI) SHA1Init, (PU) SHA1Update, (PP) SHA1Pad, (PF) SHA1Final },
{ "sha256", SHA256_DIGEST_LENGTH, (PI) SHA256Init, (PU) SHA256Update, (PP) SHA256Pad, (PF) SHA256Final },
#ifdef	SHA384_BLOCK_LENGTH
{ "sha384", SHA384_DIGEST_LENGTH, (PI) SHA384Init, (PU) SHA384Update, (PP) SHA384Pad, (PF) SHA384Final },
#endif
#ifdef	SHA512_BLOCK_LENGTH
{ "sha512", SHA512_DIGEST_LENGTH, (PI) SHA512Init, (PU) SHA512Update, (PP) SHA512Pad, (PF) SHA512Final },
#endif
#ifdef	sha3_224_hash_size
{ "sha3-224", sha3_224_hash_size, (PI) SHA3_224_Init, (PU) SHA3_Update, (PP) NULL, (PF) SHA3_Final },
#endif
#ifdef	sha3_256_hash_size
{ "sha3-256", sha3_256_hash_size, (PI) SHA3_256_Init, (PU) SHA3_Update, (PP) NULL, (PF) SHA3_Final },
#endif
#ifdef	sha3_384_hash_size
{ "sha3-384", sha3_384_hash_size, (PI) SHA3_384_Init, (PU) SHA3_Update, (PP) NULL, (PF) SHA3_Final },
#endif
#ifdef	sha3_512_hash_size
{ "sha3-512", sha3_512_hash_size, (PI) SHA3_512_Init, (PU) SHA3_Update, (PP) NULL, (PF) SHA3_Final },
#endif
{ NULL, 0, (PI) NULL, (PU) NULL, (PP) NULL, (PF) NULL },
};

#undef	PI
#undef	PU
#undef	PP
#undef	PF

#define	BUF_SIZE	32768

char	options[] =
"help,version,verbose,v,l,a*";

LOCAL	BOOL	verbose = FALSE;

LOCAL	void	usage		__PR((int exitcode));
EXPORT	int	main		__PR((int ac, char **av));
LOCAL	void	digest		__PR((FILE *f, char *name, struct adigest *dp, char *buf));

LOCAL void
usage(exitcode)
	int	exitcode;
{
	error("Usage:	mdigest [options] [file1...filen]\n");
	error("Options:\n");
	error("\t-help\t\tprint this online help\n");
	error("\t-version\tprint version number\n");
	error("\t-verbose,-v\tprint more verbose output\n");
	error("\t-a alrorithm\tspecify digest algorithm\n");
	error("\t-l\t\tprint a list of supported algorithms\n");
	exit(exitcode);
}


EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	*const *cav;
	BOOL	help = FALSE;
	BOOL	prvers = FALSE;
	BOOL	list = FALSE;
	char	*alg = NULL;
	char	*filename;
	char	*buf;
	FILE	*f;
struct	adigest	*dp = dlist;

	save_args(ac, av);
	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, options, &help, &prvers,
			&verbose, &verbose,
			&list, &alg) < 0) {
		errmsgno(EX_BAD, "Bad flag: %s.\n", cav[0]);
		usage(EX_BAD);
	}
	if (help) usage(0);
	if (prvers) {
		printf("mdigest %s (%s-%s-%s)\n\n", "1.4", HOST_CPU, HOST_VENDOR, HOST_OS);
		printf("Copyright (C) 2009-2015 Jörg Schilling\n");
		printf("This is free software; see the source for copying conditions.  There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		exit(0);
	}
	if (list) {
		for (; dp->name; dp++) {
			printf("%s\n", dp->name);
		}
		exit(0);
	}
	if (alg == NULL)
		usage(EX_BAD);

	for (; dp->name; dp++) {
		if (streql(dp->name, alg))
			break;
	}
	if (dp->name == NULL) {
		errmsgno(EX_BAD, "Unknown algorith '%s'.\n", alg);
		usage(EX_BAD);
	}
#ifdef	HAVE_VALLOC
	buf = valloc(BUF_SIZE);
#else
	buf = malloc(BUF_SIZE);
#endif
	if (buf == NULL)
		comerr("Cannot malloc read buffer.\n");

	cac = ac;
	cav = av;
	getfiles(&cac, &cav, options);

	if (cac == 0) {
		filename = "";
		f = stdin;
		digest(f, "", dp, buf);
	} else {
		for (;;) {
			if ((f = fileopen(*cav, "rub")) == (FILE *) NULL) {
				errmsg("Cannot open '%s'.\n", *cav);
			} else {
				digest(f, *cav, dp, buf);
				fclose(f);
			}
			cac--, cav++;
			getfiles(&cac, &cav, options);
			if (cac <= 0)
				break;
		}
	}
	exit(0);
	return (0);	/* Keep lint happy */
}

LOCAL void
digest(f, name, dp, buf)
		FILE	*f;
		char	*name;
	struct	adigest *dp;
		char	*buf;
{
		UInt8_t	result[256];
	register int	cnt;

	U_CTX	ctx;

	dp->init(&ctx);
	while ((cnt = ffileread(f, buf, BUF_SIZE)) > 0)
		dp->update(&ctx, buf, cnt);
	dp->final(result, &ctx);

	if (verbose)
		printf("%s ", dp->name);
	for (cnt = 0; cnt < dp->digest_len; cnt++)
		printf("%02x", result[cnt]);
	printf("  %s\n", name);
}
