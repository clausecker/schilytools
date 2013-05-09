/* @(#)job.h	1.3 13/03/26 Copyright 2009-2013 J. Schilling */

#include <schily/types.h>

typedef struct job	job;

struct job {
	job	*j_next;	/* Next active job in list	    */
	pid_t	j_pid;		/* Pid for this job, 0 if unused    */
	int	j_flags;	/* Flags for this job, see below    */
	int	j_excode;	/* Exit code for non awaited jobs   */
	char	*j_cmd;		/* Expanded single command line	    */
	obj_t	*j_obj;		/* obj for this command		    */
	date_t	j_date;		/* Current date for this target	    */
};

/*
 * Definitions for j_flags
 */
#define	J_SILENT	0x001	/* '@' or -s forwarded to cmd_wait()	*/
#define	J_NOERROR	0x002	/* '-' or -i forwarded to cmd_wait()	*/
#define	J_MYECHO	0x004	/* "echo ..." at cmd start handled in make  */
#define	J_NOWAIT	0x100	/* Exit code known, no wait() in cmd_wait() */

/*
 * job.c
 */
extern	job	*newjob		__PR((void));
extern	void	setup_xvars	__PR((void));
extern	void	setup_SHELL	__PR((void));
extern	BOOL	cmd_prefix	__PR((char *cmd, int pfx));
extern	BOOL	cmdlist_prefix	__PR((cmd_t *cmd, int pfx));
extern	int	docmd		__PR((char * cmd, obj_t * obj));

extern	int	curjobs;	/* # of currently active jobs		*/
extern	int	maxjobs;	/* # of max. possible jobs		*/
extern	job	jobs[];		/* The list of job			*/
