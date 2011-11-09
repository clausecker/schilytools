/* @(#)strstr.c	1.2 11/10/21 Copyright 1985-2011 J. Schilling */
/*
 *	find string s2 in string s1
 *	return NULL if s2 is not found
 *	otherwise return pointer of first occurrence of s2 in s1
 *
 *	Copyright (c) 1985-2011 J. Schilling
 */
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	HAVE_STRSTR

EXPORT char *
strstr(s1, s2)
	register const char	*s1;
		const char	*s2;
{
	register const char	*a;
	register const char	*b;

	if (*s2 == '\0')
		return ((char *)s1);

	for (; *s1 != '\0'; s1++) {
		for (a = s2, b = s1; *a == *b++; ) {
			if (*a++ == '\0')
				return ((char *)s1);
		}
		if (*a == '\0')
			return ((char *)s1);
	}
	return ((char *)NULL);
}

#endif
