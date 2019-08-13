#define	conc(a,b)a/**/b

/*
 * Note that the concatenation would be ./bin/* and this is a comment start.
 * So the rest of the file is ignored and the closed source Sun cpp even
 * prints an error message: "t06b.c: line 5: missing * /". We inserted a
 * space between * and / to avoid problems with cpp from thic comment.
 */
conc(./bin/,*)
conc(./bin/,a)
