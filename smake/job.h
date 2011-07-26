/* @(#)job.h	1.1 09/09/19 Copyright 2009 J. Schilling */

#include <schily/types.h>

typedef struct job	job;

struct job {
	job	*j_next;
	pid_t	j_pid;
	int	j_flags;
	int	j_excode;
	char	*j_cmd;
	obj_t	*j_obj;
	date_t	j_date;		/* Current date for this target	    */
};

/*
 * Definitions for j_flags
 */
#define	J_SILENT	0x001
#define	J_NOERROR	0x002
#define	J_NOWAIT	0x100
