.\" @(#)cstyle.ms 1.8 96/08/19 SMI
.ND 96/08/19
.RP 
.TL
C Style and Coding Standards for SunOS
.br
.AU
Bill Shannon
.AI
Copyright \(co 1993 by Sun Microsystems, Inc.
.br
All rights reserved.
.AU
Version 1.8 of 96/08/19.
.\"--------------------
.\" Footnote numbering
.ds f \\u\\n+f\\d
.nr f 0 1
.ds F \\n+F.
.nr F 0 1
.\"--------------------
.AB
This document describes a set of coding standards and recommendations
that are local standards for programs written in C for the SunOS product.
The purpose of these standards is to facilitate sharing of each other's code,
as well as to
enable construction of tools (e.g., editors, formatters)
that, by incorporating knowledge of these standards,
can help the programmer in the preparation of programs.
.PP
This document is based on a similar document
written by L.W. Cannon, R.A. Elliott, L.W. Kirchhoff, J.H. Miller,
J.M. Milner, R.W. Mitze, E.P. Schan, N.O. Whittington at Bell Labs.
It also incorporates several items from a similar paper written by S. Shah.
The current version was derived from an earlier version written by the
author and Warren Teitelman.
.AE
.NH
Introduction
.LP
The scope of this document is the coding style used in writing
C programs for the SunOS product.
.ig xx
The purpose of the document is \fBnot\fP to establish a rigid style
to be imposed on everyone.
We fully expect many to disagree with, and possibly deviate from, some of
the standards set forth here.
The goal in writing down these standards is to allow  
those who do not feel strongly about a particular stylistic issue
to adopt the style
most used by others, rather than simply adopting some random style of their
own.
.xx
To the extent that we do adhere to a common style,
it will be easier for several people to cooperate in the development of
the same program.
It also will facilitate understanding and maintaining
code developed by someone else.
.ig xx
Finally, it will enable the construction of tools that incorporate knowledge
of these standards
to help the programmer in the preparation of programs.
.xx
.LP
.ig xx
For certain style issues, such as number of spaces used for indentation and the
format of variable declarations, no clear consensus exists at Sun.
In these cases, we have documented the various styles that are most
frequently used.
However, we strongly recommend that within a particular project, and
certainly within a package or module, only one style be employed.
.xx
More important than the particular coding style used is \fIconsistency\fP
of coding style.
Within a particular module, package, or project, a consistent coding
style should be used throughout.
This is particularly important when modifying an existing program;
the modifications should be coded in the same style as the program
being modified, not in the programmer's personal style, nor necessarily
in the style advocated by this document.
.LP
This document discusses ANSI C only briefly, and C++ is hardly mentioned.
A future version of this document will discuss ANSI C more fully,
describing when to use such new features as \fLconst\fP.
In addition, rules for writing C code that must interact with C++ code,
and vice versa, will be covered.
Style rules for pure C++ programs will be left to a separate document.
.ig xx
.LP
To facilitate sharing each other's code in the presence of several
competing styles, the \fIindent\fP program, a C formatter,
has been extended to allow the user to specify which of the accepted styles
to use. 
This allows an individual to take a C program written in a different style,
and convert it to a style he prefers. 
.xx
.LP
Of necessity, these standards cannot cover all situations.
Experience and informed judgment count for much.
Inexperienced programmers who encounter unusual situations should
consult 1) code written by experienced C programmers following
these rules, or 2) experienced C programmers.
.NH
File Naming Conventions
.LP
.UX
requires certain suffix conventions for names of files to be processed
by the \fLcc\fP command\*f.
Other suffixes are simply conventions that we require.
The following suffixes are required:
.FS
.IP \*F
In addition to the suffix conventions given here,
it is conventional to use `Makefile' (not `makefile') for the
control file for \fImake\fP
and
`README' for a summary of the contents of a directory or directory
tree.
.FE
.IP \(bu
C source file names end in \fI.c\fP
.IP \(bu
Assembler source file names end in \fI.s\fP
.IP \(bu
Relocatable object file names end in \fI.o\fP
.IP \(bu
Include header file names end in \fI.h\fP
.IP \(bu
Archived library files end in \fI.a\fP
.IP \(bu
Fortran source file names end in \fI.f\fP
.IP \(bu
Pascal source file names end in \fI.p\fP
.IP \(bu
C-shell source file names end in \fI.csh\fP
.IP \(bu
Bourne shell source file names end in \fI.sh\fP
.IP \(bu
Yacc source file names end in \fI.y\fP
.IP \(bu
Lex source file names end in \fI.l\fP
.NH
Program Organization
.LP
The choices made in organizing a program often affect its understandability and ease of maintenance.
Many modern programming languages provide mechanisms
for defining the modules of a program.
Although C does not have these
language features, they can be approximated using the techniques and conventions described below.
.LP
A module is a group of procedures and data that together implement some abstraction, for example, a symbol table.
Those procedures and data that can be accessed by other modules are called \fIpublic\fP.
Procedures and data known only inside the module are called \fIprivate\fP.
Modules are defined using two files: an \fIinterface\fP file and an \fIimplementation\fP file.
The interface file contains the public definitions of the module. 
The implementation file contains the private definitions and the code for the
procedures in the module.
.LP 
In C, the role of an interface file is provided by a header file. As mentioned above, the name of a header file
ends in \fI.h\fP. The implementation file for a particular header file often has
the same root name, but ends in \fI.c\fP instead of \fI.h\fP.
Private definitions in the implementation file are declared to be \fLstatic\fP.
.LP
As an example, here are the interface and implementation files for a symbol
table package:
.sp
\fIsymtab.h\fP
.LS
.sp
/*
 * Symbol table interface.
 */

typedef struct symbol Symbol;
struct	symbol {
	char	*name
	int	value;
	symbol	chain;
};

Symbol	*insert();
Symbol	*lookup();
.LE
.sp 2
\fIsymtab.c\fP
.LS
.sp
/*
 * Symbol table implementation.
 */

#include "symtab.h"

#define HASHTABLESIZE	101

static Symbol hashtable[HASHTABLESIZE];
static unsigned hash();

.LE
.LS
/*
 * insert(name) inserts "name" into the symbol table.
 * It returns a pointer to the symbol table entry.
 */
Symbol *
insert(name)
	char *name;
{
	\&...
}

.LE
.LS
/*
 * lookup(name) checks to see if name is in the symbol table.
 * It returns a pointer to the symbol table entry if it 
 * exists or NULL if not.
 */
Symbol *
lookup(name)
	char *name;
{
	\&...
}

.LE
.LS
/*
 * hash(name) is the hash function used by insert and lookup.
 */
static unsigned
hash(name)
 	char *name;
{
	\&...
}
.LE
.LP
When the implementation of an abstraction is too large for a single
file, a similar technique is used.
There is an interface file, or in some cases, several
interface files, which contain the public definitions that clients of the
package will see, and a collection of implementation files.
If it is necessary for several implementation files to share definitions or
data that should not be seen by clients of the package,
these definitions are placed in a private header file shared by those files.
A convention for naming such header files is to append '_impl'
to the root name of
the public interface file, e.g., \fIsymtab_impl.h\fP, although this
convention isn't widely followed.
Generally speaking, such implementation header files should \fBnot\fP be
shipped to customers\*f.
.FS
.IP \*F
Traditionally many kernel implementation header files have been shipped,
for use by programs that read \fL/dev/kmem\fP.
Following this tradition for kernel header files is allowed but not required.
In other areas of the system, shipping implementation header files is strongly
discouraged.
.FE
.NH
File Organization
.LP
A file consists of various sections that should be separated by blank lines.
Although there is no maximum length requirement for source files,
files with more than about 3000 lines are cumbersome to deal with.
Lines longer than 80 columns are not handled well by all terminals
and should be avoided if possible\*f.
.FS
.IP \*F
Excessively long lines which result from deep indenting are often
a symptom of poorly organized code.
.FE
.sp
.LP
The suggested order of sections for a \fIheader\fP file is as follows
(see the Appendix for a detailed example):
.sp
.IP 1.
The first thing in the file should be a comment including the
copyright notice.
This comment might also describe the purpose of this file.
.IP 2.
The second thing in the file should be an \fL#ifndef\fP that checks
whether the header file has been previously included, and
if it has, ignores the rest of the file (see section 5).
.IP 3.
Next should be a \fL#pragma ident\fP line\*f.
.FS
.IP \*F
The form \fL#pragma ident\fP is preferred over the obsolete and less
portable form \fL#ident\fP.
.FE
.IP 4.
If this header file needs to include any other header files, the
\fL#include\fP statements should be next.
.IP 5.
Next should be a guard to allow the header file to be used by C++ programs.
.IP 6.
If it did not appear at the beginning of the file, next there
should be a block comment describing the contents of the file\*f.
.FS
.IP \*F
This comment sometimes appears between items 3 and 4.
Any of these locations is acceptable.
.FE
A description of the purpose of the objects in the files (whether
they be functions, external data declarations or definitions, or
something else) is more useful than just a list of the object names.
Keep the description short and to the point.
A list of authors and modification history is \fBnot\fP appropriate here.
.IP 7.
Any \fL#define\fPs that apply to the file as a whole are next.
.IP 8.
Any \fLtypedef\fPs are next.
.IP 9.
Next come the structure declarations.
If a set of \fL#define\fPs applies to a particular piece of global data
(such as a flags word), the \fL#define\fPs should be immediately after
the data declaration.
.IP 10.
Next come the global variable declarations\*f.
Declarations of global variables should use the \fLextern\fP keyword.
(Never declare static variables in a header file.)
.FS
.IP \*F
It should be noted that declaring variables in a header file
is often a poor idea.
Frequently it is a symptom of poor partitioning of code between files.
.FE
.IP 11.
Finally come the declarations of functions.
All external functions should be declared, even those that return \fLint\fP,
or do not return a value (declare them to return \fLvoid\fP).
.IP 12.
The end of the header should close with the match to the C++ guard and
the match to the multiple inclusion guard.
.sp
.LP
The suggested order of sections for a \fI.c\fP file (implementation file) is
roughly the same as a header file, eliminating unneeded constructs (such as
the multiple inclusion and C++ guards).
Don't forget the copyright notice and \fL#pragma ident\fP.
.LP
After all the declarations come the definitions of the procedures themselves,
each preceded by a block comment.
They should be in some sort of meaningful order.
Top-down is generally better than bottom-up\*f,
.FS
.IP \*F
Declaring all functions before any are defined allows the implementor
to use the top-down ordering without running afoul of the single pass C compilers.
.FE
and a breadth-first
approach (functions on a similar level of abstraction together) is
preferred over depth-first (functions defined as soon as possible
after their calls). 
Considerable judgement is called for here.
If defining large numbers of essentially independent utility
functions, consider alphabetical order.
.NH
Header Files
.LP
Header files are files that are included in other files prior to compilation
by the C preprocessor.
Some are defined at the system level like <\fIstdio.h\fP> which must be included
by any program using the standard I/O library.
Header files are also used to contain data declarations and \fL#define\fPs
that are needed by more than one program\*f.
.FS
.IP \*F
Don't use absolute pathnames when including header files.
Use the \fI<name>\fP construction for getting them from a standard
place, or define them relative to the current directory.
The \-I option of the C compiler is the best way to handle
extensive private libraries of header files; it permits reorganizing
the directory structure without altering source files.
.FE
Header files should be functionally organized,
i.e., declarations for separate subsystems should be in separate header files.
Also, if a set of declarations is likely to change when code is
ported from one machine to another, those declarations should be
in a separate header file.
.LP
It is often convenient to be able to nest header files, for example, to 
provide the user with a single header file which includes, in the right order,
a number of other header files needed for a particular application.
However, some objects like typedefs and initialized data
definitions cannot be
seen twice by the compiler in one compilation.
Therefore, to provide for the possibility
that two master header files will both include the same
header file, each header file should contain a check for whether it has
previously been included. The standard way of doing this is to define a
variable whose name consists of the name of the header file, including
any leading directories, but with characters that are otherwise illegal
in a variable name replaced with underscores, and with a leading underscore
added.
The entire header file is then bracketed in an \fL#ifndef\fP
statement which checks whether that variable has been defined.
For example, here
is a header file that would be referenced with
\fL#include <bool.h>\fP which defines the enumerated type \fLBool\fP:
.sp
\fIbool.h\fP
.LS
/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef _BOOL_H
#define	_BOOL_H

#pragma ident	"%\&Z%%\&M%	%\&I%	%\&E% SMI"

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum { FALSE = 0, TRUE = 1 } Bool;

#ifdef	__cplusplus
}
#endif

#endif /* _BOOL_H */
.LE
.LP
It is a requirement that exported system header files be acceptable
to non-ANSI (K&R) C compilers, ANSI C compilers, and C++ compilers.
ANSI C defines the new keywords \fLconst\fP, \fLsigned\fP, and \fLvolatile\fP.
These keywords should only be used within ANSI C compatible parts of the
header file.
The biggest difference between ANSI C and K&R C is the ability to
declare the types of function parameters.
To handle both forms of declarations for external functions, the
following style should be used.
.LS
#if defined(__STDC__)
extern	bool_t bool_from_string(char *);
extern	char *bool_to_string(bool_t);
#else
extern	bool_t bool_from_string();
extern	char *bool_to_string();
#endif
.LE
.LP
Some people find it helpful to include the parameter types in comments
in the K&R form of the declaration.
.LS
extern	bool_t bool_from_string(/* char * */);
extern	char *bool_to_string(/* bool_t */);
.LE
While ANSI C allows you to use parameter names (as well as types) in
function declarations, and many people find that the names provide
useful documentation of the function parameters, the names must be chosen
extremely carefully.
A user's \fL#define\fP using the same name can render the function
declaration syntactically invalid.
.LP
The \fLextern "C"\fP guard is required in the header file for C++ compatibility.
In addition, the following C++ keywords should not be used in headers.
.LS
.TS
center;
l l l l l.
asm	friend	overload	public	throw
catch	inline	private	template	try
class	new	protected	this	virtual
delete	operator
.TE
.LE
.LP
Note that there are many additional rules for header files that are
specified by various standards, such as ANSI C, POSIX, and XPG.
.NH
Indentation
.LP
.ig xx
Although both four-space and eight-space indentation are used at Sun,
eight-space indentation is generally preferred\*f.
.FS
.IP \*F
Four-space indentation may work better in the presence of long
identifier names. In any event, one style should be chosen
and used consistently throughout a program.
.FE
.xx
Only eight-space indentation should be used, and a tab should be used rather than
eight spaces.
If eight-space indentation causes the code to be too wide to fit in 80
columns, it is often the case that the nesting structure is too complex
and the code would be clearer if it were rewritten.
The rules for how to indent particular C constructs such as
\fLif\fP statements, \fLfor\fP statements, \fLswitch\fP statements, etc., are described below in the
section on compound statements.
.ig xx
Note that the \fIindent\fP program can be instructed to use either
four- or eight-space indentation, and thus provides a way of converting between these formats. 
.xx
.NH
Comments in C Programs
.LP
Comments should be used to give overviews of code and provide
additional information that isn't readily available in the code itself.
Comments should only contain information that is germane to reading
and understanding the program.
For example, information about how
the corresponding package is built or in what directory it should
reside should not be included as a comment in a source file. 
Nor should comments include a list of authors or a modification history
for the file; this information belongs in the SCCS history.
Discussion of nontrivial design decisions
is appropriate, but avoid duplicating information that is present in
(and clear from) the code.
It's too easy for such redundant information to get out-of-date.
In general, avoid including in comments information that is likely
to become out-of-date. 
.LP
Comments should \fBnot\fP be enclosed in large boxes drawn with asterisks
or other characters.
Comments should never include special characters, such as form-feed
and backspace.
.LP
There are three styles of comments:
block, single-line, and trailing. These are discussed next.
.NH 2
Block Comments
.LP
The opening /* of a block comment that appears outside of any function
should be in column one.
There should be a * in column
2 before each line of text in the block comment, and the closing */ should be in columns 2-3 (so that the *'s line up).
This enables \fIgrep ^.\e*\fP to catch all of the block comments in a file.
There is never any text on the first or last lines of the block comment.
The initial text line is separated from the * by a single space, although later
text lines may be further indented, as appropriate.
.LS
/*
 * Here is a block comment.
 * The comment text should be spaced or tabbed over
 * and the opening slash-star and closing star-slash
 * should be alone on a line.
 */
.LE
.LP
Block comments are used to provide English descriptions
of the contents of files, the functions of procedures, and to describe data structures and algorithms.
Block comments should be used at the beginning of each file and before each procedure.
The comment at the beginning of the file containing \fLmain()\fP should include a description of what the program does and
its command line syntax.
The comments at the beginning of other files should describe the contents
of those files.
.LP
The block comment that precedes each procedure should document its function,
input parameters, algorithm, and returned value. For example,
.LS
/*
 * index(c, str) returns a pointer to the first occurrence of
 * character c in string str, or NULL if c doesn't occur
 * in the string.
 */
.LE
.LP
In many cases, block comments inside a function are appropriate, and
they should be indented to the same indentation level as the code that
they describe.
.LP
Block comments should generally contain complete English sentences
and should follow the English rules for punctuation and capitalization.
The other types of comments described below will more often contain
sentence fragments, phrases, etc.
.NH 2
Single-Line Comments
.LP
Short comments may appear on a single line indented over to the indentation level of the code that follows.
.LS
if (argc > 1) {
	/* get input file from command line */
	if (freopen(argv[1], "r", stdin) == NULL)
		error("can't open %s\en", argv[1]);
}
.LE
.LP
Two single-line comments can appear in a row if one line isn't enough, but
this is strongly discouraged.
The comment text should be separated from the opening /* and closing */
by a space\*f.
.FS
.IP \*F
Except for the special lint comments \fL/*ARGSUSED*/\fP and
\fL/*VARARGS\fP\fIn\fP\fL*/\fP
(which should appear alone on a line
immediately preceding the function to which they apply),
and \fL/*NOTREACHED*/\fP, in which the spaces are not required.
.FE
The closing */'s of several adjacent single-line comments should \fBnot\fP
be forced to be aligned vertically.
In general, a block comment should be used when a single line is insufficient.
.ig xx
Note that \fIindent\fP will automatically convert a single-line
comment that can no longer fit in the available space on a line into a block comment, i.e.,
it handles wrapping of comments.
.xx
.NH 2
Trailing Comments
.LP
Very short comments may appear on the same line as the code they describe,
but should be tabbed over far enough to separate them from the statements.
If more than one short comment appears in a block of code, they should all
be tabbed to the same tab setting.
.ig xx
\fIIndent\fP allows the user to specify this
tab setting and will automatically position comments at the indicated place.
.xx
.LS
if (a == 2)
	return (TRUE);		/* special case */
else
	return (isprime(a));	/* works only for odd a */
.LE
Trailing comments are most useful for documenting declarations.
Avoid the assembly language style of commenting every line of
executable code with a trailing comment.
.LP
Trailing comments are often also used on preprocessor \fL#else\fP and
\fL#endif\fP statements if they are far away from the corresponding test.
Occasionally, trailing comments will be used to match right braces with
the corresponding C statement, but this style is discouraged except in cases
where the corresponding statement is many pages away\*f.
.FS
.IP \*F
Many editors include a command to find a matching brace, making it easy
to navigate from the right brace back to the corresponding statement.
.FE
.NH
Declarations
.LP
There is considerable variation at Sun in the formatting of
declarations with regard to
the number of declarations per line,
and whether using tabs within declarations makes them more readable.
.NH 2
How Many Declarations Per Line?
.LP
There is a weak consensus that one declaration per line is to be preferred,
because it encourages commenting.
In other words, 
.LS
int	level;		/* indentation level */
int	size;		/* size of symbol table */
int	lines;		/* lines read from input */
.LE
is preferred over:
.LS
int	level, size, lines;
.LE
.LP
However, the latter style is frequently used, especially for declaration of several temporary variables of primitive types such as \fLint\fP or \fLchar\fP.
In no case should variables and functions be declared on the same line, e.g.,
.LS
long	dbaddr, get_dbaddr();	/* WRONG */
.LE
.NH 2
Indentation Within Declarations
.LP
Many programmers at Sun like to insert tabs in their variable declarations to
align the variables. They feel this makes their code more readable.
For example:
.LS
int		x;
extern int	y;
register int	count;
char		**pointer_to_string;
.LE
.LP
.ig xx
\fIIndent\fP allows the user to specify the number of columns between the
beginning of a declaration and the variable name. In the above example,
this number is 16. If the key words in the declaration require more than 
this number of characters, then \fIindent\fP simply inserts a
space between the last key word and the variable name, e.g.,
.xx
If variables with long type names are declared along with variables
with short type names, it may be best to indent the variables one
or two tab stops, and only use a single space after the long type names, e.g.,
.LS
int		x, y, z;
extern int	i;
register int 	count;
struct very_long_name *p;
.LE
.LP
.ig xx
Note that setting this parameter to 0 produces the following format, which
is also acceptable:
.xx
It is also acceptable to use only a single space between the type name
and the variable name:
.LS
int x;
char c;
enum rainbow y;
.LE
.LP
There is no Sun standard regarding how far over to indent variable names.
The user should choose a value that looks pleasing to him, taking
into consideration how frequently he employs lengthy declarations,
but note that declarations such as the following probably make the code
\fBharder\fP to read:
.LS
struct very_long_structure_name			*p;
struct another_very_long_structure_name		*q;
char						*s;
int						i;
short						r;
.LE
.LP
Note that the use of \fL#define\fP to declare constants and macros
follows indentation rules similar to those for other declarations.
In particular, the \fL#define\fP, the macro name, and the macro text should
all be separated from each other by tabs and properly aligned.
.NH 2
Local Declarations
.LP
Do not declare the same variable name in an inner block\*f.
.FS
.IP \*F
In fact, avoid any local declarations that override declarations
at higher levels.
.FE
For example,
.LS
func()
{
	int cnt;
	\&...
	if (condition) {
		register int cnt;	/* WRONG */
		\&...
	}
	\&...
}
.LE
.LP
Even though this is valid C, the potential confusion is enough
that
\fIlint\fP will complain about it when given the \fB\-h\fP option.
.NH 2
External Declarations
.LP
External declarations should begin in column 1.
Each declaration should be on a separate line.
A comment describing the role of the object being declared should be
included, with the exception that a list of defined constants does not
need comments if the constant names are sufficient documentation.
The comments should be tabbed so that they line up underneath each
other\*f.
.FS
.IP \*F
So should the constant names and their defined values.
.FE
Use the tab character rather than blanks.
For structure and union template declarations, each element should be alone
on a line with a comment describing it.
The opening brace (\ {\ ) should be on the same line as the structure
tag, and the closing brace should be alone on a line in column 1, e.g.,
.LS
struct	boat {
	int	wllength;	/* water line length in feet */
	int	type;		/* see below */
	long	sarea;		/* sail area in square feet */
};

/*
 * defines for boat.type
 */
#define	KETCH	1
#define	YAWL	2
#define	SLOOP	3
#define	SQRIG	4
#define	MOTOR	5
.LE
.LP
In any file which is part of a larger whole rather than a self-contained
program, maximum use should be made of the \fLstatic\fP keyword to make
functions and variables local to single files.
Variables in particular should be accessible from other files
only when there is a clear
need that cannot be filled in another way.
Such usages should be commented to make it clear that another file's
variables are being used.
.NH 2
Function Definitions
.LP
Each function should be preceded by a block comment prologue that gives
the name and a short description of what the function does.
The type of the value returned should
be alone on a line in column 1 (\fLint\fP should be specified explicitly).
If the function does not return a value then it should be given
the return type \fLvoid\fP.
If the value returned requires a long explanation, it should be given
in the prologue.
Functions that are not used outside of the file in which they are
declared should be declared as \fLstatic\fP.
This lets the reader know explicitly that a function is private,
and also eliminates the possibility of name conflicts with variables
and procedures in other files.
.LP
When defining functions using the old K&R syntax,
the function name and formal parameters should be alone on a line
beginning in column 1.
This line is followed by the declarations of the formal parameters.
Each parameter should be declared (do not default to \fLint\fP),
and tabbed over one indentation level.
The opening brace of the function body should also be alone on a line
beginning in column 1.
.LP
For functions defined using the ANSI C syntax, the style is much the same.
The examples below illustrate the acceptable forms of ANSI C declarations.
.LP
All local declarations and code within the function body should be
tabbed over at least one tab.
(Labels may appear in column 1.)
If the function uses any external variables or functions that are not
otherwise declared \fLextern\fP, these should have their
own declarations in the function body using the \fLextern\fP keyword.
If the external variable is an array, the array bounds must be repeated
in the \fLextern\fP declaration.
.LP
If an external variable or a parameter of type pointer is
changed by the function, that fact should be noted in the comment.
.LP
All comments about parameters and local variables should be
tabbed so that they line up underneath each other.
The declarations should be separated from the function's statements
by a blank line.
.LP
The following examples illustrate many of the rules for function definitions.
.LP
.LS
/*
 * sky_is_blue()
 *
 * Return true if the sky is blue, else false.
 */
int
sky_is_blue()
{
	extern int hour;

	if (hour < MORNING || hour > EVENING)
		return (0);	/* black */
	else
		return (1);	/* blue */
}
.LE
.LS
/*
 * tail(nodep)
 *
 * Find the last element in the linked list
 * pointed to by nodep and return a pointer to it.
 */
Node *
tail(nodep)
	Node *nodep;
{
	register Node *np;	/* current pointer advances to NULL */
	register Node *lp;	/* last pointer follows np */

	np = lp = nodep;
	while ((np = np->next) != NULL)
		lp = np;
	return (lp);
}
.LE
.LS
/*
 * ANSI C Form 1.
 * Use this form when the arguments easily fit on one line,
 * and no per-argument comments are needed.
 */
int
foo(int alpha, char *beta, struct bar gamma)
{
	\&...
}
.LE
.LS
/*
 * ANSI C Form 2.
 * This is a variation on form 1, using the standard continuation
 * line technique (indent by 4 spaces).  Use this form when no
 * per-argument comments are needed, but all argument declarations
 * won't fit on one line.  This form is generally frowned upon,
 * but acceptable.
 */
int
foo(int alpha, char *beta,
    struct bar gamma)
{
	\&...
}
.LE
.LS
/*
 * ANSI C Form 3.
 * Use this form when per-argument comments are needed.
 * Note that each line of arguments is indented by a full
 * tab stop.  Note carefully the placement of the left
 * and right parentheses.
 */
int
foo(
	int alpha,		/* first arg */
	char *beta,		/* arg with a vert long comment needed */
				/*   to describe its purpose */
	struct bar gamma)	/* big arg */
{
	\&...
}
.LE
.NH 2
Type Declarations
.LP
Many programmers at Sun use named types, i.e., \fLtypedef\fPs, liberally.
They feel that the use of \fLtypedef\fPs simplifies declaration lists and
can make program modification easier when types must change. 
Other programmers feel that the use of 
a \fLtypedef\fP hides the underlying type when they want to know what the type is.
This is particularly
true for programmers who need to be concerned with efficiency, e.g., kernel programmers,
and therefore need to be aware of the implementation details.
The choice of whether or not to use \fLtypedef\fPs is left to the implementor.
.LP
It should be noted, however, that \fLtypedef\fPs should be used to isolate
implementation details, rather than to save keystrokes\*f.
.FS
.IP \*F
An exception is made in the cases of \fLu_char\fP, \fLu_short\fP, etc.
that are defined in <\fIsys/types.h\fP>.
.FE
For instance, the following example demonstrates two inappropriate uses
of \fLtypedef\fP.  In both cases, the code would pass \fIlint\fP\*f,
.FS
.IP \*F
Some people claim that this is a bug in \fIlint\fP.
.FE
but nevertheless depends on the underlying types
and would break if these were to change:
.LS
typedef char *Mything1;		/* These typedefs are inappropriately */
typedef int Mything2;		/*   used in the code that follows. */

int
can_read(t1)
	Mything1 t1;
{
	Mything2 t2;

	t2 = access(t1, R_OK);	/* access() expects a (char *) */
	if (t2 == -1)		/*   and returns an (int) */
		takeaction();
	return (t2);		/* can_read() returns an (int) */
}
.LE
.LP
If one elects to use a \fLtypedef\fP in conjunction with a pointer type,
the underlying type should be \fLtypedef\fP-ed,
rather than \fLtypedef\fP-ing a pointer to underlying type, because it is often necessary and
usually helpful to be able to tell if a type is a pointer.
Thus, in the example in section 3, \fLSymbol\fP is defined to be a \fLstruct symbol\fP,
rather than \fLstruct symbol *\fP, and \fLinsert()\fP defined to return \fLSymbol *\fP.  
.NH
Statements
.LP
Each line should contain at most one statement.
In particular, do not use the comma operator to group multiple
statements on one line, or to avoid using braces.
For example,
.LS
argv++; argc--;		/* WRONG */

if (err)
	fprintf(stderr, "error"), exit(1);	/* VERY WRONG */
.LE
.LP
Do not nest the ternary conditional operator (?:).
For example:
.LS
num = cnt < tcnt ? (cnt < fcnt ? fcnt : cnt) :
    tcnt < bcnt ? tcnt : bcnt > fcnt ? fcnt : bcnt;	/* WRONG */
.LE
.LP
If the \fLreturn\fP statement is used to return a value, the expression
should always be enclosed in parentheses.
.LP
Functions that return no value should \fBnot\fP include a \fLreturn\fP
statement as the last statement in the function.
.NH 2
Compound Statements
.LP
Compound statements are statements that contain lists of statements
enclosed in {} braces.
The enclosed list should be indented one more level than the compound statement itself.
The opening left brace should be at the end of the line beginning the
compound statement and the closing right brace should be alone on a
line, positioned under the beginning of the compound statement. (See examples below.)
Note that the left brace that begins a function body is the only occurrence
of a left brace which should be alone on a line.
.LP
Braces are also used around a single statement when it is part of a control structure, such as an \fLif-else\fP or \fLfor\fP statement, as in:
.LS
if (condition) {
	if (other_condition) 
		statement;
}
.LE
.LP
Some programmers feel that braces should be used to surround \fBall\fP
statements that are part of control structures, even singletons,
because this makes it easier to add or delete statements without thinking about
whether braces should be added or removed\*f.
.FS
.IP \*F
Some programmers reason that, since some apparent function calls might
actually be macros that expand into multiple statements, always using
braces allows such macros to always work safely.
Instead, we strongly discourage the use of such macros.
If such macros must be used, they should be all upper case so as to clearly
distinguish them as macros; see the Naming Conventions section.
.FE
Thus, they would write:
.LS
if (condition) {
	return (0);
}
.LE
.LP
In either case, if one arm of an \fLif-else\fP statement contains
braces, all arms should contain braces.
Also, if the body of a \fLfor\fP or \fLwhile\fP loop is empty,
no braces are needed:
.LS
while (*p++ != c)
	;
.LE
.NH 2
Examples
.LP
\fBif, if-else, if-else if-else statements\fP
.LS
if (condition) {
	statements;
}

.LE
.LS
if (condition) {
	statements;
} else {
	statements;
}

.LE
.LS
if (condition) {
	statements;
} else if (condition) {
	statements;
}
.LE
.LP
Note that the right brace before the \fLelse\fP and the right brace
before the \fLwhile\fP of a \fLdo-while\fP statement (see below) are the
only places where a right brace appears that is not alone on a line.
.KS
.LP
\fBfor statements\fP
.LS no
for (initialization; condition; update) {
	statements;
}
.LE
.LP
When using the comma operator in the initialization or update clauses
of a \fLfor\fP statement, it is suggested that no more than three variables
should be updated.
More than this tends to make the expression too complex.
In this case it is generally better to use separate statements outside
the \fLfor\fP loop (for the initialization clause), or at the end of
the loop (for the update clause).
.LP
The infinite loop is written using a \fLfor\fP loop.
.LS no
for (;;) {
	statements;
}
.LE
.KE
.KS
.LP
\fBwhile statements\fP
.LS no
while (condition) {
	statements;
}
.LE
.KE
.KS
.LP
\fBdo-while statements\fP
.LS no
do {
	statements;
} while (condition);
.LE
.KE
.KS
.LP
\fBswitch statements\fP
.LS no
switch (condition) {
case ABC:
case DEF:
	statements;
	break;
case XYZ:
	statements;
	break;
default:
	statements;
	break;
}
.LE
.KE
.LP
The last \fLbreak\fP\*f is, strictly speaking, redundant, but it is recommended form
.FS
.IP \*F
A \fLreturn\fP statement is sometimes substituted for the \fLbreak\fP
statement, especially in the \fLdefault\fP case.
.FE
nonetheless because it prevents a fall-through error if another \fLcase\fP
is added later after the last one.
In general, the fall-through feature of the C \fLswitch\fP statement should
rarely, if ever, be used (except for multiple case labels as shown in the example).
If it is, it should be commented for future maintenance.
.LP
All \fLswitch\fP statements should include a default case\*f.
.FS
.IP \*F
With the possible exception of a switch on an \fLenum\fP variable for
which all possible values of the \fLenum\fP are listed.
.FE
Don't assume that the list of cases covers all possible cases.
New, unanticipated, cases may be added later, or bugs elsewhere in the
program may cause variables to take on unexpected values.
.ig xx
.LP
Another style is to indent the case labels one half tab setting from the switch statement, i.e., four spaces if eight spaces are used for indentation,
two spaces if four space indentation is used.
.LS
switch (condition) {
    case ABC:
    case DEF:
	statements;
	break;
    case XYZ:
	statements;
	break;
    default:
	statements;
	break;
}
.LE
.LP 
Either style is acceptable, and both are supported by \fIindent\fP.
.xx
.LP
The \fLcase\fP statement should be indented to the same level as the
\fLswitch\fP statement.
The \fLcase\fP statement should be on a line separate from the statements
within the case.
.LP
The next example shows the format that should be used for a switch
whenever the blocks of statements contain more than a couple of lines.
Note the use of blank lines to set off the individual arms of the switch.
Note also the use of a block local to one case to declare variables.
.LS
switch (condition) {

case ABC:
case DEF:
	statement1;
	.
	.
	statementn;
	break;

case XYZ: {
	int var1;

	statement1;
	.
	.
	statementm;
	break;
}

default:
	statements;
	break;
}
.LE
.NH
Using White Space
.NH 2
Vertical White Space
.LP
The previous example illustrates the use of blank lines to improve the
readability of a complicated switch statement.
Blank lines improve readability by setting off sections of code that are
logically related.
Generally, the more white space in code (within reasonable limits),
the more readable it is.
.LP
A blank line should always be used in the following circumstances:
.IP \(bu
After the \fL#include\fP section.
.IP \(bu
After blocks of \fL#define\fPs of constants, and before and after \fL#define\fPs of macros.
.IP \(bu
Between structure declarations.
.IP \(bu
Between procedures.
.IP \(bu
After local variable declarations.
.ig xx
and between the opening brace of
a function and the first statement of the function if there are no
local variable declarations.
.xx
.sp
.LP
Form-feeds should never be used to separate functions.
Instead, separate functions into separate files, if desired.
.NH 2
Horizontal White Space
.LP
Here are the guidelines for blank spaces:
.IP \(bu
A blank should follow a keyword\*f whenever a parenthesis follows the keyword.
.FS
.IP \*F
Note that \fLsizeof\fP and \fLreturn\fP are keywords, whereas \fLstrlen\fP
and \fLexit\fP are not.
.FE
Blanks should not be used between procedure names (or macro calls) and their argument list.
This helps to distinguish keywords from procedure calls.
.LS 5
if (strcmp(x, "done") == 0)	/* no space between strcmp and '(' */
	return (0);		/* space between return and '(' */
.LE
.IP \(bu
Blanks should appear after the commas in argument lists.
.IP \(bu
Blanks should \fBnot\fP appear immediately after a left parenthesis or
immediately before a right parenthesis.
.IP \(bu
All binary operators except \fL.\fP and \fL\->\fP should be separated from their
operands by blanks\*f.
In other words, blanks should appear around assignment, arithmetic, relational,
and logical operators.
.FS
.IP \*F
Some judgment is called for in the case of complex expressions,
which may be clearer if the ``inner'' operators are not surrounded
by spaces and the ``outer'' ones are.
.FE
Blanks should never separate unary operators such as unary minus,
address (`\fL&\fP'), indirection (`\fL*\fP'), increment (`\fL++\fP'),
and decrement (`\fL--\fP') from their operands.
Note that this includes the unary \fL*\fP that is a part of pointer
declarations.
.LP
Examples:
.LS
a += c + d;
a = (a + b) / (c * d);
strp\->field = str.fl - ((x & MASK) >> DISP);
while (*d++ = *s++)
	n++;
.LE
.IP \(bu
The expressions in a \fLfor\fP statement should be separated by blanks, e.g.,
.LS
for (expr1; expr2; expr3)
.LE
.IP \(bu
Casts should not be followed by a blank, with the exception of function
calls whose return values are ignored, e.g.,
.LS
(void) myfunc((unsigned)ptr, (char *)x);
.LE
.NH 2
Hidden White Space
.LP
There are many uses of blanks that will not be visible when viewed
on a terminal, and it is often difficult to distinguish blanks from tabs.
However, inconsistent use of blanks and tabs may produce unexpected results
when the code is printed with a pretty-printer, and may make simple regular
expression searches fail unexpectedly.
The following guidelines are helpful:
.IP \(bu
Avoid spaces and tabs at the end of a line.
.IP \(bu
Avoid spaces between tabs and tabs between spaces.
.IP \(bu
Use tabs to line things up in columns (e.g., for indenting code, and to line
up elements within a series of declarations) and spaces to
separate items within a line.
.IP \(bu
Use tabs to separate single line comments from the corresponding code.
.NH
Parenthesization
.LP
Since C has some unexpected precedence rules,
it is generally a good idea to use parentheses liberally in expressions involving mixed operators.
It is also important to remember that complex expressions can be used as parameters to macros,
and operator-precedence problems can arise unless \fBall\fP occurrences of
parameters in the body of a macro definition have parentheses around them.
.NH
Naming Conventions
.LP
Identifier conventions can make programs more understandable by making them
easier to read.
They can also give information about the
function of the identifier, e.g., constant, named type, that can be
helpful in understanding code.
Individual projects will no doubt have their own naming conventions.
However, each programmer should be consistent about his use of naming
conventions.
.LP
Here are some general rules about naming.
.IP \(bu
Variable and function names should be short yet meaningful.
One character variable names should be avoided except for temporary
``throwaway'' variables.
Use variables \fLi\fP, \fLj\fP, \fLk\fP, \fLm\fP, \fLn\fP
for integers, \fLc\fP, \fLd\fP, \fLe\fP for characters,
\fLp\fP, \fLq\fP for pointers, and \fLs\fP, \fLt\fP for
character pointers.
Avoid variable \fLl\fP because it is hard to distinguish \fLl\fP
from \fL1\fP on some printers and displays.
.IP \(bu
Pointer variables should have a ``p'' appended to their names for
each level of indirection.
For example, a pointer to the variable \fLdbaddr\fP (which contains
disk block addresses) can be named \fLdbaddrp\fP (or perhaps simply
\fLdp\fP).
Similarly, \fLdbaddrpp\fP would be a pointer to a pointer to the
variable \fLdbaddr\fP.
.IP \(bu
Separate "words" in a long variable name with underscores, e.g.,
\fLcreate_panel_item\f\P\*f.
.FS
.IP \*F
Mixed case names, e.g., \fLCreatePanelItem\fP, are strongly discouraged.
.FE
An initial underscore should never be used in any user-program names\*f. Trailing underscores should be avoided too.
.FS
.IP \*F
Initial underscores are reserved for global names that are internal to
software library packages.
.FE
.IP \(bu
\fL#define\fP names for constants should be in all CAPS.
.IP \(bu
Two conventions are used for named types, i.e., \fLtypedef\fPs.
Within the kernel named types are given a name ending in \fL_t\fP, e.g.,
.LS
typedef enum { FALSE, TRUE } bool_t;
typedef struct node node_t;
.LE
In many user programs named types have their first letter capitalized, e.g.,
.LS
typedef enum { FALSE, TRUE } Bool;
typedef struct node Node;
.LE
.IP \(bu
Macro names may be all CAPS or all lower case.
Some macros (such as \fIgetchar\fP and \fIputchar\fP) are in lower case
since they may also exist as functions.
There is a slight preference for all upper case macro names.
.IP \(bu
Variable names, structure tag names, and function names should be in
lower case.
.LP
Note: in general, with the exception of named types, it is best to avoid names that differ only in case, like \fLfoo\fP
and \fLFOO\fP.
The potential for confusion is considerable.
However, it is acceptable to use as a typedef a name which differs only
in capitalization from its base type, e.g.,
.LS
typedef struct node Node;
.LE
It is also acceptable to give a variable of this type a name that is the
all lower case version of the type name, e.g., 
.LS
Node node;
.LE
.IP \(bu
The individual items of enums should be guaranteed unique
names by prefixing them with a tag identifying the package to which they belong. For example,
.LS
enum rainbow { RB_red, RB_orange, RB_yellow, RB_green, RB_blue };
.LE
The \fIdbx\fP debugger supports enums in that it can print out the value
of an enum, and can also perform assignment statements using
an item in the range of an enum.
Thus, the use of enums over equivalent \fL#define\fPs may make program debugging
easier.
For example, rather than writing:
.LS
#define	SUNDAY	0
#define	MONDAY	1
\.\.
.LE
write:
.LS
enum day_of_week { dw_sunday, dw_monday, \fI...\fP };
.LE
.IP \(bu
Implementors of libraries should take care to hide the
names of any variables and functions that have been declared \fLextern\fP because
they are shared by several modules in the library, but nevertheless are private to the library. 
One technique for doing this is to prefix the name with an underscore and
a tag that is
unique to the package, e.g., \fL_panel_caret_mpr\fP.
.NH
Continuation Lines
.LP
Occasionally, an expression will not fit in the available space in a line,
for example, a procedure call with many arguments,
or a conjunction or disjunction
with many arms.
Such occurrences are especially likely when blocks are nested deeply or long
identifiers are used.
If this happens,
the expression should be broken after the last comma in the case of a function call
(never in the middle of a parameter expression),
or after the last operator that fits on the line
(the continuation line should never start with a binary operator).
The next line should be further indented by half a tab stop.
If they are needed, subsequent continuation lines should be broken in the same manner, and aligned
with each other.
For example,
.LS
if (long_logical_test_1 || long_logical_test_2 ||
    long_logical_test_3) {
	statements;
}

.LE
.LS
a = (long_identifier_term1 - long_identifier_term2) *
     long_identifier_term3;

.LE
.LS
function(long_complicated_expression1, long_complicated_expression2,
    long_complicated_expression3, long_complicated_expression4,
    long_complicated_expression5, long_complicated_expression6)
.LE  
.ig xx
Some programmers prefer to align function arguments with the first
character to the right of the left parenthesis in the previous line, e.g.,
.LS
function(long_complicated_expression1, long_complicated_expression2,
	 long_complicated_expression3, long_complicated_expression4,
	 long_complicated_expression5, long_complicated_expression6)
.LE
.LP
\fIIndent\fP will automatically format continuation lines.
Note however that \fIindent\fP will never break a line, it will simply properly align
lines that have been broken by the user.
.xx
.NH
Constants
.LP
Numerical constants should not be coded directly.
The \fL#define\fP feature of the C preprocessor should be used to
assign a meaningful name.
This will also make it easier to administer large programs since the
constant value can be changed uniformly by changing only the \fL#define\fP.
The enum data type is the preferred way to handle situations where
a variable takes on only a discrete set of values, since additional type
checking is available through \fIlint\fP.
As mentioned above, the \fIdbx\fP debugger also provides support for enums.
.LP
There are some cases where the constants 0 and 1 may appear as themselves
instead of as \fL#define\fPs.
For example if a \fLfor\fP loop indexes through an array, then
.LS
for (i = 0; i < ARYBOUND; i++)
.LE
is reasonable while the code
.LS
fptr = fopen(filename, "r");
if (fptr == 0)
	error("can't open %s\en", filename);
.LE
is not.
In the last example, the defined constant \fLNULL\fP is available as
part of the standard I/O library's header file <\fIstdio.h\fP>, and
should be used in place of the 0.
.LP
In rare cases, other constants may appear as themselves.
Some judgement is required to determine whether the semantic meaning of the
constant is obvious from its value, or whether the code would be easier
to understand if a symbolic name were used for the value.
.NH
Goto
.LP
While not completely avoidable, use of \fLgoto\fP is discouraged. In many cases,
breaking a procedure into smaller pieces, or using a different language
construct will enable elimination of \fLgoto\fPs. For example,
instead of:
.LS 0
again:
	if (s = proc(args))
		if (s == -1 && errno == EINTR)
			goto again;
.LE
write:
.LS 0
	do {
		s = proc(args);
	} while (s == -1 && errno == EINTR);	/* note\*f */
.LE
.FS
.IP \*F
These two expressions are equivalent unless \fLs\fP is asynchronously modified,
e.g., if \fLs\fP is an I/O register.
.FE
The main place where \fLgoto\fPs can be usefully employed is to break out
of several levels of \fLswitch\fP, \fLfor\fP, and \fLwhile\fP
nesting\*f, e.g.,
.FS
.IP \*F
The need to do such a thing may indicate
that the inner constructs should be broken out into
a separate function, with a success/failure return code.
.FE
.LS 0
	for (...)
		for (...) {
			...
			if (disaster)
				goto error;
		}
	\&...
error:
	\fIclean up the mess\fP
.LE
Never use a \fLgoto\fP outside of a given block to branch to a label
within a block:
.LS
goto label;	/* WRONG */
\&...
for (...) {
	\&...
label:
	statement;
	\&...
}
.LE
When a \fLgoto\fP is necessary, the accompanying label should be alone
on a line and positioned one indentation level to the left of the code that follows.
If a label is not followed by a program statement (e.g., if the next token
is a closing brace (\ }\ )) a NULL statement (\ ;\ ) must follow the label.
.NH
Variable Initialization
.LP
C permits initializing a variable where it is declared. Programmers at Sun are
equally divided about whether or not this is a good idea:
.QP
"I like to think of declarations and executable code as separate units. Intermixing them only confuses the issue. If only a scattered few declarations are initialized, it is easy not to see them."
.QP
"The major purpose of code style is clarity. I think the less hunting around for the connections between different places in the code, the better. I don't think
variables should be initialized for no reason, however. If the variable doesn't
need to be initialized, don't waste the reader's time by making him/her think that it does."
.LP
A convention used by some programmers is to only initialize automatic variables
in declarations if the value of the variable is constant throughout the block.
.LP
The decision about whether or not to initialize a variable in a declaration is
therefore left to the implementor. Use good taste. For example, don't bury a variable initialization in the middle of a long declaration, e.g.,
.LS
int	a, b, c, d = 4, e, f;		/* This is NOT good style */
.LE
.NH
Multiple Assignments
.LP
C also permits assigning several variables to the same value in a single statement, e.g.,
.LS
x = y = z = 0;
.LE
Good taste is required here also. For example, assigning several variables that are used the same way in the program in a single statement clarifies the relationship between the variables by making it more explicit, e.g.,
.LS
x = y = z = 0;
vx = vy = vz = 1;
count = 0;
scale = 1;
.LE
is good, whereas:
.LS
x = y = z = count = 0;
vx = vy = vz = scale = 1;
.LE
sacrifices clarity for brevity.
In any case, the variables that are so
assigned should all be of the same type (or all pointers being initialized
to \fLNULL\fP).
It is not a good idea (because it is hard to read) to use multiple assignments for complex expressions, e.g.,
.LS
foo_bar.fb_name.firstchar = bar_foo.fb_name.lastchar = 'c';	/* Yecch */
.LE
.NH
Preprocessor
.LP
Do not rename members of a structure using \fL#define\fP within a
subsystem; instead, use a \fIunion\fP.
However, \fL#define\fP can be used to define shorthand notations
for referencing members of a union.
For example, instead of
.LS
struct proc {
	\&...
	int	p_lock;
	\&...
};
\&...
\fIin a subsystem:\fP
#define	p_label	p_lock
.LE
use
.LS
struct proc {
	\&...
	union {
		int	p_Lock;
		int	p_Label;
	} p_un;
	\&...
};
#define	p_lock	p_un.p_Lock
#define	p_label	p_un.p_Label
.LE
.LP
Be \fIextremely\fP careful when choosing names for \fL#define\fPs.
For example, never use something like
.LS
#define	size	10
.LE
especially in a header file, since it is not unlikely that the user
might want to declare a variable named \fLsize\fP.
.LP
Remember that names used in \fL#define\fP statements come out of a global
preprocessor name space and can conflict with names in any other namespace.
For this reason, this use of \fL#define\fP is discouraged.
.LP
Note that \fL#define\fP follows indentation rules similar to other
declarations; see the section on indentation for details.
.LP
Care is needed when defining macros that replace functions since functions
pass their parameters by value whereas macros pass their arguments by
name substitution.
.LP
At the end of an \fL#ifdef\fP construct used to select among a required
set of options (such as machine types), include a final \fL#else\fP
clause containing a useful but illegal statement so that the compiler
will generate an error message if none of the options has been defined:
.LS
#ifdef vax
	\&...
#elif sun
	\&...
#elif u3b2
	\&...
#else
#error unknown machine type;
#endif /* machine type */
.LE
.LP
Note that \fL#elif\fP is an ANSI C construct and should not be used in
header files that must be able to be processed be older K&R C compilers.
Note also the use of the ANSI C \fL#error\fP statement, which also has
the desired effect when using a K&R C compiler.
.LP
Don't change C syntax via macro substitution, e.g.,
.LS
#define	BEGIN	{
.LE
It makes the program unintelligible to all but the perpetrator.
.NH
Miscellaneous Comments on Good Taste
.LP
Try to make the structure of your program match the intent. For example,
replace:
.LS
if (boolean_expression)
	return (TRUE);
else
	return (FALSE);
.LE
with:
.LS
return (boolean_expression);
.LE
Similarly, 
.LS
if (condition)
	return (x);
return (y);
.LE
is usually clearer when written as:
.LS
if (condition)
	return (x);
else
	return (y);
.LE
or even better, if the condition and return expressions are short;
.LS
return (condition ? x : y);
.LE
Do not default the boolean test for nonzero, i.e.
.LS
if (f() != FAIL)
.LE
is better than
.LS
if (f())
.LE
even though \fLFAIL\fP may have the value 0 which is considered to mean
false by C\*f.
This will help you out later when somebody decides that a failure return
should be \-1 instead of 0.
An exception is commonly made for predicates, which are functions
which meet the following restrictions:
.IP \(bu
Has no other purpose than to return true or false.
.IP \(bu
Returns 0 for false, non-zero for true.
.IP \(bu
Is named so that the meaning of (say) a `true' return
is absolutely obvious.
Call a predicate \fLis_valid\fP or \fLvalid\fP, not \fLcheck_valid\fP.
.FS
.IP \*F
A particularly notorious case is using \fLstrcmp\fP
to test for string equality, where the result should never be defaulted.
.FE
.LP
Never use the boolean negation operator (\fL!\fP) with non-boolean
expressions.
In particular, never use it to test for a NULL pointer or to test for
success of the \fLstrcmp\fP function, e.g.,
.LS
char *p;
\&...
if (!p)			/* WRONG */
	return;

if (!strcmp(*argv, "-a"))	/* WRONG */
	aflag++;
.LE
.LP
Do not use the assignment operator in a place where it could be easily
confused with the equality operator
For instance, in the simple expression
.LS
if (x = y)
	statement;
.LE
it is hard to tell whether the programmer really meant assignment or
the equality test.
Instead, use
.LS
if ((x = y) != 0)
	statement;
.LE
or something similar if the assignment is needed within the \fLif\fP statement.
.LP
There is a time and a place for embedded assignments\*f.
.FS
.IP \*F
The \fB++\fP and \fB\-\-\fP operators count as assignments.
So, for many purposes, do functions with side effects.
.FE
In some constructs there is no better way to accomplish the results
without making the code bulkier and less readable.
For example:
.LS
while ((c = getchar()) != EOF) {
	\fIprocess the character\fP
}
.LE
Using embedded assignments to improve run-time performance
is also possible.
However, one should consider the tradeoff between increased speed and
decreased maintainability that results when embedded assignments are
used in artificial places.
For example, the code:
.LS
a = b + c;
d = a + r;
.LE
should not be replaced by
.LS
d = (a = b + c) + r;
.LE
even though the latter may save one cycle.
Note that in the long run the time difference between the two will
decrease as the optimizer gains maturity, while the difference in
ease of maintenance will increase as the human memory of what's
going on in the latter piece of code begins to fade\*f.
.FS
.IP \*F
Note also that side effects within expressions can result in code
whose semantics are compiler-dependent, since C's order of evaluation
is explicitly undefined in most places.
Compilers do differ.
.FE
.LP
There is also a time and place for the ternary \fL?\ :\fP operator
and the binary comma operator.
If an expression containing a binary operator appears before the \fL?\fP,
it should be
parenthesized:
.LS
(x >= 0) ? x : \-x
.LE
Nested \fL?\ :\fP operators can be confusing and should be avoided
if possible.
There are some macros like \fIgetchar\fP where they can be useful.
The comma operator can also be useful in \fLfor\fP statements to
provide multiple initializations or incrementations.
.NH
Portability
.LP
The advantages of portable code are well known.
This section gives some guidelines for writing portable code,
where the definition of portable is a source file
can be compiled and executed on different
machines with the only source change being the inclusion of (possibly
different) header files.
The header files will contain \fL#define\fPs and \fLtypedef\fPs
that may vary from machine to machine.
.LP
There are two aspects of portability that must be considered:
hardware compatibility and interface compatibility.  The former
category encompasses problems arising from differing machine types,
e.g., byte-ordering differences.  The second category deals with
the multiplicity of
.UX
operating system interfaces, e.g., BSD4.3 and System V.
As the POSIX standard (IEEE P1003.1 Portable Operating System Interface)
becomes widely used, it will be preferable to write programs and
software packages that use POSIX semantics exclusively.  Where this
is not possible (for example, POSIX does not define the BSD \fIsocket\fP
interface), the required extensions should be documented in a comment
near the top of the source file and/or in accompanying manuals\*f.
.FS
.IP \*F
Stringent documentation requirements are defined for programs that
claim POSIX conformance.
.FE
.LP
The POSIX standards provide for portability across a wide range of systems.
The X/Open XPG standards are emerging as important standards for
portability across a wide range of
.UN
systems.
Generally it is best to avoid vendor-specific extensions and use the
standard interface that best matches your requirements for portability,
performance, functionality, etc.
.LP
The following is a list of pitfalls to be avoided and recommendations
to be considered when designing portable code:
.IP \(bu
First, one must recognize that some things are inherently non-portable.
Examples are code to deal with particular hardware registers such as
the program status word,
and code that is designed to support a particular piece of hardware
such as an assembler or I/O driver.
Even in these cases there are many routines and data organizations
that can be made machine-independent.
Source files should be organized so that the machine-independent
code and the machine-dependent code are in separate files.
Then if the program is to be moved to a new machine,
it is a much easier task to determine what needs to be changed\*f.
.FS
.IP \*F
If you \fL#ifdef\fP dependencies,
make sure that if no machine is specified,
the result is a syntax error, \fBnot\fP a default machine!
.FE
It is also possible that code in the machine-independent files
may have uses in other programs as well.
.IP \(bu
Pay attention to word sizes.
The following sizes apply to basic types in C for some common machines:
.br
.ne 2i
.TS
center;
l c c c c c c c
l r r r r r r r.
type	PDP11	VAX	Mac	PC/DOS	680x0	SPARC	PC/UNIX
_
char	8	8	8	8	8	8	8
short	16	16	16	16	16	16	16
int	16	32	16	16	32	32	32
long	32	32	32	32	32	32	32
pointer	16	32	32	16	32	32	32
.TE
In general, if the word size is important, \fLshort\fP or \fLlong\fP
should be used to get 16- or 32-bit items on any of the above machines\*f.
.FS
.IP \*F
Any unsigned type other than plain \fLunsigned int\fP should be
\fLtypedef\fPed, as such types are highly compiler-dependent.
This is also true of long and short types other than \fLlong int\fP
and \fLshort int\fP.
Large programs should have a central header file which supplies
\fLtypedef\fPs for commonly used width-sensitive types, to make
it easier to change them and to aid in finding width-sensitive code.
.FE
If a simple loop counter is being used where either 16 or 32 bits will
do, then use \fLint\fP, since it will get the most efficient (natural)
unit for the current machine.
.KS
.IP \(bu
Word size also affects shifts and masks.
The code
.LS
x &= 0177770
.LE
.KE
will clear only the three rightmost bits of an \fIint\fP on a DOS PC.
On a SPARC it will also clear the entire upper halfword.
Use
.LS
x &= ~07
.LE
instead which works properly on all machines\*f.
.FS
.IP \*F
The or operator (\ |\ ) does not have these problems, nor do bitfields.
.FE
.IP \(bu
Beware of making assumptions about the size of pointers.
They are not always the same size as \fLint\fP\*f.
.FS
.IP \*F
Nor are all pointers always the same size, or freely interchangeable.
Pointer-to-character is a particular trouble spot on machines that
do not address to the byte.
.FE
Also, be aware of potential pointer alignment problems.
On machines that do not support a uniform address space (unlike, e.g., SPARC),
the conversion of a
pointer-to-character to a pointer-to-int may result in an invalid address.
.IP \(bu
Watch out for signed characters.
On the SPARC, characters are sign extended when used in expressions,
which is not the case on some other machines.
In particular, \fIgetchar\fP is an integer-valued function (or macro)
since the value of \fLEOF\fP for the standard I/O library is \-1,
which is not possible for a character on the IBM 370\*f.
If the code depends on the character being signed rather than unsigned,
it's probably best to use the ANSI C \fLsigned\fP keyword.
.FS
.IP \*F
Actually, this is not quite the real reason why \fIgetchar\fP returns
\fLint\fP, but the comment is valid:  code that assumes either
that characters are signed or that they are unsigned is unportable.
It is best to completely avoid using \fLchar\fP to hold numbers.
Manipulation of characters as if they were numbers is also
often unportable.
.FE
.IP \(bu
On some processors on which C exists the
bits (or bytes) are numbered from right to left within a word.
Other machines number the bits from left to right.
Hence any code that depends on the left-right orientation of bits
in a word deserves special scrutiny.
Bit fields within structure members will only be portable so long as
two separate fields are never concatenated and treated as a unit.
The same applies to variables in general.
Alignment considerations and loader peculiarities make it very rash
to assume that two consecutively declared variables are together
in memory, or that a variable of one type is aligned appropriately
to be used as another type.
.IP \(bu
Become familiar with existing library functions and \fL#define\fPs\*f.
.FS
.IP \*F
But not \fBtoo\fP familiar.
The internal details of library facilities, as opposed to their
external interfaces, are subject to change without warning.
They are also often quite unportable.
.FE
You should not be writing your own string compare routine, or making
your own \fL#define\fPs for system structures\*f.
.FS
.IP \*F
Or, especially, writing your own code to control terminals.
Use the \fItermcap\fP, \fIterminfo\fP, or \fIcurses\fP packages.
.FE
Not only does this waste your time, but it prevents your program
from taking advantage of any microcode assists or other
means of improving performance of system routines\*f.
.FS
.IP \*F
It also makes your code less readable, because the reader has to
figure out whether you're doing something special in the reimplemented
stuff to justify its existence.
Furthermore, it's a fruitful source of bugs.
.FE
.IP \(bu
Use \fIlint\fP and \fImake\fP (see next sections).
.NH
Lint
.LP
\fILint\fP is a C program checker that examines C source files to
detect and report type incompatibilities, inconsistencies between
function definitions and calls,
potential program bugs, etc.
It is a good idea to use \fIlint\fP
on programs that are being released to a wide audience.
.LP
It should be noted that the best way to use \fIlint\fP is not as a barrier
that must be overcome before official acceptance of a program, but
rather as a tool to use whenever major changes or additions to the
code have been made.
\fILint\fP
can find obscure bugs and insure portability before problems occur.
.NH
Make
.LP
\fIMake\fP is a program that interprets a description file (\fLMakefile\fP)
in order to produce \fIshell\fP commands that generate target files
from their sources. All projects should have a \fLMakefile\fP in the
top-level source directory that contains rules to build
all of the relevant targets.  In general,
the top-level \fLMakefile\fP will be very simple, containing rules
that invoke recursive \fImake\fPs in the sub-directories.
.LP
In addition to project-dependent targets, the \fLMakefile\fP should contain
rules to build the following targets:
.IP \fLall\fP 10
builds all targets
.IP \fLinstall\fP 10
installs all targets and header files in the appropriate directories
.IP \fLclean\fP 10
removes all intermediate files
.IP \fLclobber\fP 10
removes all targets and intermediate files
.IP \fLlint\fP 10
executes \fIlint\fP on all targets
.LP
For SunOS, more detailed Makefile guidelines are specified elsewhere.
.NH
Project-Dependent Standards
.LP
Individual projects may wish to establish additional standards beyond
those given here.
The following issues are some of those that should be addressed by
projects.
.IP \(bu
What additional naming conventions should be followed?
In particular, systematic prefix conventions for functional grouping
of global data and also for structure or union member names can be useful.
.IP \(bu
What kind of include file organization is appropriate for the
project's particular data hierarchy?
.IP \(bu
What procedures should be established for reviewing \fIlint\fP
complaints?
A tolerance level needs to be established in concert with the \fIlint\fP
options to prevent unimportant complaints from hiding complaints about
real bugs or inconsistencies.
.IP \(bu
If a project establishes its own archive libraries, it should plan on
supplying a \fIlint\fP library file to the system administrators.
This will allow \fIlint\fP to check for compatible use of library
functions.
.NH
Conclusion
.LP
A set of standards has been presented for C programming style.
One of the most important points is the proper use of white space
and comments so that the structure of the program is evident from
the layout of the code.
Another good idea to keep in mind when writing code is that it is
likely that you or someone else will be asked to modify it or make
it run on a different machine some time in the future.
.bp
.ce 1
\fBBibliography\fP
.sp 2
.IP [1]
S. Shah,
\fIC Coding Guidelines for
.UX
System Development Issue 3\fP,
AT&T (internal document) 1987.
.IP [2]
B.W. Kernighan and D.M. Ritchie,
\fIThe C Programming Language\fP, 
Prentice-Hall 1978.
.IP [3]
S.P. Harbison and G.L. Steele,
\fIA C Reference Manual\fP, 
Prentice-Hall 1984.
.IP [4]
Evan Adams, Dave Goldberg, et al,
\fINaming Conventions for Software Packages\fP,
Sun Microsystems (internal document) 1987.
.IP [5]
ANSI/X3.159-198x,
\fIProgramming Language C Standard\fP,
(Draft) 1986.
.IP [6]
IEEE/P1003.1,
\fIPortable Operating System for Computer Environments\fP,
(Draft) 1987.
.bp
.NH
Appendix 1:	SCCS ident strings and copyrights
.LP
The following are SCCS ident strings and copyrights for various kinds of files.
(Omit the Copyright comment if not relevant.)
Note that \fL%\&W%\fP is an acceptable substitute for \fL%\&Z%%\&M% %\&I%\fP.
Note also that there are tabs between the \fL%\&M%\fP, \fL%\&I%\fP,
\fL%\&E%\fP, and \fL%\&W%\fP keywords, to make the output of the \fBwhat(1)\fP
command more readable.
.sp
\fBHeader Files\fP
.sp
.LS 0
/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#ifndef \fIguard\fP
#define	\fIguard\fP

#pragma ident	"%\&Z%%\&M% %\&I% %\&E% SMI"

#ifdef	__cplusplus
extern "C" {
#endif

\fIbody of header\fP

#ifdef	__cplusplus
}
#endif

#endif \fIguard\fP
.LE
.sp
\fBC or Assembler Files\fP
.sp
.LS 0
/*
 * Copyright (c) 1993 by Sun Microsystems, Inc.
 * All rights reserved.
 */

#pragma ident	"%\&Z%%\&M% %\&I% %\&E% SMI"
.LE
.sp
\fBMakefiles\fP
.sp
.LS 0
#
# %\&Z%%\&M% %\&I% %\&E% SMI
#
.LE
.sp
\fBShell Files\fP
.sp
.LS 0
#!/bin/sh	(\fRor\fP /bin/ksh, \fRor\fP /bin/csh)
#
# %\&Z%%\&M% %\&I% %\&E% SMI
#
.LE
.sp
.LS 0
\fBManual Pages\fP
.sp
\0.\\\\" %\&Z%%\&M% %\&I% %\&E% SMI
.LE
