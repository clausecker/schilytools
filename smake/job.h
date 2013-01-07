/* @(#)job.h	1.2 12/12/29 Copyright 2009-2012 J. Schilling */

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
#define	J_SILENT	0x001	/* '@' or -s forwarded to cmd_wait()	*/
#define	J_NOERROR	0x002	/* '-' or -i forwarded to cmd_wait()	*/
#define	J_MYECHO	0x004	/* "echo ..." at cmd start handled in make  */
#define	J_NOWAIT	0x100	/* Exit code known, no wait() in cmd_wait() */
