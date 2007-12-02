/* @(#)sym.h	1.14 04/03/11 Copyright 1985, 1999 J. Schilling */
/*
 *	A program to produce a static calltree for C-functions
 *
 *	symbol definitions
 *
 *	Copyright (c) 1985, 1999 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

/*
 * The heart of all name lists used in calltree.
 *
 * All symbol nodes that are members of the global function list use
 * the s_uses member to keep track of callers/callees,
 *
 * while all symbol nodes that are in a symbol tree bejond a
 * the s_uses pointer use the s_sym pointer as a back pointer to the
 * symbol node of the caller/callee.
 *
 */
typedef struct symbol sym_t;

struct symbol {
	sym_t	*s_left;		/* ... will be sorted before this   */
	sym_t	*s_right;		/* ... will be sorted after this    */
	char	*s_filename;		/* Filename where this sym was found */
	char	*s_name;		/* Name of this symbol		    */
	union {
		sym_t	*v_uses;	/* Caller or Callee usage tree	    */
		sym_t	*v_sym;		/* Ptr to caller/calle in main tree */
	} s_value;
	int	s_lineno;		/* Line number this sym was found   */
	int	s_flags;		/* Flags			    */
};

/*
 * Shortcut's for readability of the code.
 */
#define	s_uses	s_value.v_uses
#define	s_sym	s_value.v_sym

/*
 * Definitions for s_flags
 */
#define	S_RECURSE	0x01		/* Found a recursive call	    */
#define	S_USED		0x02		/* This func has been used	    */
#define	S_DEF		0x04		/* Found a definition for this func */
#define	S_WARN		0x08		/* Warned about multiple definitions */

/*
 * The third parameter of lookup() ... if not L_LOOK/L_CREATE, this is a
 * symbol tree where we could find already allocated strings in s_name.
 */
#define	L_LOOK		((sym_t *)0)	/* Only lookup a symbol		    */
#define	L_CREATE	((sym_t *)1)	/* Create the symbol if not yet def'd */

extern	sym_t	*lookup		__PR((char *name, sym_t **table, sym_t *gtab));
