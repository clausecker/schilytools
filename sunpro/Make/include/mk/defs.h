#ifndef _MK_DEFS_H
#define _MK_DEFS_H
/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may use this file only in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2004 Sun Microsystems, Inc. All rights reserved.
 * Use is subject to license terms.
 */
/*
 * @(#)defs.h 1.61 06/12/12
 */

#pragma	ident	"@(#)defs.h	1.61	06/12/12"

/*
 * Copyright 2017-2021 J. Schilling
 * Copyright 2022 the schilytools team
 *
 * @(#)defs.h	1.21 21/09/06 2017-2021 J. Schilling
 */

/*
 * Included files
 */
#ifdef DISTRIBUTED
#	include <dm/Avo_AcknowledgeMsg.h>
#	include <dm/Avo_DoJobMsg.h>
#	include <dm/Avo_JobResultMsg.h>
#endif

#include <mksh/defs.h>

#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
#	include <rw/xdrstrea.h>
#endif


/*
 * Defined macros
 */

#define SKIPSPACE(x)	while (*x &&				\
			       ((*x == (int) space_char) ||	\
			        (*x == (int) tab_char) ||	\
			        (*x == (int) comma_char))) {	\
					x++;			\
			}

#define SKIPWORD(x)	while (*x &&				\
			       (*x != (int) space_char) &&	\
			       (*x != (int) tab_char) &&	\
			       (*x != (int) newline_char) &&	\
			       (*x != (int) comma_char) &&	\
			       (*x != (int) equal_char)) {	\
					x++;			\
			}

#define SKIPTOEND(x)	while (*x &&				\
			       (*x != (int) newline_char)) {	\
					x++;			\
			}

#define PMAKE_DEF_MAX_JOBS	2	/* Default number of parallel jobs. */

#define OUT_OF_DATE(a,b) \
	(((a) < (b)) || (((a) == file_doesnt_exist) && ((b) == file_doesnt_exist)))

#define OUT_OF_DATE_SEC(a,b) \
	(((a).tv_sec < (b).tv_sec) || (((a).tv_sec == file_doesnt_exist.tv_sec) && ((b).tv_sec == file_doesnt_exist.tv_sec)))
	
#define SETVAR(name, value, append) \
				setvar_daemon(name, value, append, no_daemon, \
					      true, debug_level)
#ifdef SUN5_0
#undef	MAX
#define MAX(a,b)		(((a)>(b))?(a):(b))
/*
 * New feature added to SUN5_0 make,  invoke the vanilla svr4 make when
 * the USE_SVR4_MAKE environment variable is set.
 */
#define SVR4_MAKE		"/usr/ccs/lib/svr4.make"
#define USE_SVR4_MAKE		"USE_SVR4_MAKE"
#endif
/*
 * The standard MAXHOSTNAMELEN is 64. We want 32.
 */
#define	MAX_HOSTNAMELEN		32


/*
 * typedefs & structs
 */
typedef enum {
	no_state,
	scan_name_state,
	scan_command_state,
	enter_dependencies_state,
	enter_conditional_state,
	enter_equal_state,
	illegal_bytes_state,
	illegal_eoln_state,
	poorly_formed_macro_state,
	exit_state
}                       Reader_state;

struct _Name_vector {
	struct _Name		*names[64];
	struct _Chain		*target_group[64];
	short                   used;
	struct _Name_vector	*next;
};

struct _Running {
	struct _Running		*next;
	Doname			state;
	struct _Name		*target;
	struct _Name		*true_target;
	struct _Property	*command;
	struct _Name		*sprodep_value;
	char			*sprodep_env;
	int			recursion_level;
	Boolean			do_get;
	Boolean			implicit;
	Boolean			redo;
	int			auto_count;
	struct _Name		**automatics;
	pid_t			pid;
#if defined(TEAMWARE_MAKE_CMN) || defined(PMAKE)
	int			job_msg_id;
#else
	int			host;
#endif
	char			*stdout_file;
	char			*stderr_file;
	struct _Name		*temp_file;
	int			conditional_cnt;
	struct _Name		**conditional_targets;
#if defined(TEAMWARE_MAKE_CMN) || defined(PMAKE)
	Boolean			make_refd;
#endif
};

typedef enum {
	serial_mode,
	parallel_mode,
	distributed_mode
} DMake_mode;

typedef enum {
	txt1_mode,
	txt2_mode,
	html1_mode
} DMake_output_mode;

struct _Recursive_make {
	struct _Recursive_make	*next;	/* Linked list */
	wchar_t			*target;/* Name of target */
	wchar_t			*oldline;/* Original line in .nse_depinfo */
	wchar_t			*newline;/* New line in .nse_depinfo */
	wchar_t			*cond_macrostring; 
					/* string built from value of 
					 * conditional macros used by 
					 * this target
					 */
	Boolean			removed;/* This target is no longer recursive*/
};

struct _Dyntarget {
	struct _Dyntarget	*next;
	struct _Name		*name;
};


/*
 * Typedefs for all structs
 */
typedef struct _Cmd_line	*Cmd_line, Cmd_line_rec;
typedef struct _Dependency	*Dependency, Dependency_rec;
typedef struct _Macro		*Macro, Macro_rec;
typedef struct _Name_vector	*Name_vector, Name_vector_rec;
typedef struct _Percent		*Percent, Percent_rec;
typedef struct _Dyntarget	*Dyntarget;
typedef struct _Recursive_make	*Recursive_make, Recursive_make_rec;
typedef struct _Running		*Running, Running_rec;


/*
 *	extern declarations for all global variables.
 *	The actual declarations are in globals.cc
 */
extern	Boolean		allrules_read;
extern	Name		posix_name;
extern	Name		svr4_name;
extern	Boolean		sdot_target;
extern	Boolean		all_parallel;
extern	Boolean		assign_done;
extern	Boolean		build_failed_seen;
#ifdef DISTRIBUTED
extern	Boolean		building_serial;
#endif
extern	Name		built_last_make_run;
extern	Name		c_at;
#ifdef DISTRIBUTED
extern	Boolean		called_make;
#endif
extern	Boolean		command_changed;
extern	Boolean		commands_done;
extern	Chain		conditional_targets;
extern	Name		conditionals;
extern	Boolean		continue_after_error;
extern	Property	current_line;
extern	Name		current_make_version;
extern	Name		current_target;
extern	short		debug_level;
extern	Cmd_line	default_rule;
extern	Name		default_rule_name;
extern	Name		default_target_to_build;
extern	Boolean		depinfo_already_read;
extern	Name		dmake_group;
extern	Name		dmake_max_jobs;
extern	Name		dmake_mode;
extern	DMake_mode	dmake_mode_type;
extern	Name		dmake_compat_mode;
extern	Name		dmake_output_mode;
extern	DMake_output_mode	output_mode;
extern	Name		dmake_odir;
extern	Name		dmake_rcfile;
extern	Name		dollar;
extern	Name		done;
extern	Name		dot;
extern	Name		dot_keep_state;
extern	Name		dot_keep_state_file;
extern	Name		empty_name;
extern	Boolean		fatal_in_progress;
extern	int		file_number;
extern	Name		force;
extern	Name		ignore_name;
extern	Boolean		ignore_errors;
extern	Boolean		ignore_errors_all;
#ifdef	DO_INCLUDE_FAILED
extern	Name		include_failed_name;
extern	Boolean		include_failed;
#endif
extern	Name		init;
extern	int		job_msg_id;
extern	Boolean		keep_state;
extern	Name		make_state;
#if defined(TEAMWARE_MAKE_CMN) || defined(PMAKE)
extern	timestruc_t	make_state_before;
#endif
extern	Boolean		make_state_locked;
extern	Dependency	makefiles_used;
extern	Name		makeflags;
extern	Name		make_version;
extern	char		mbs_buffer2[];
extern	char		*mbs_ptr;
extern	char		*mbs_ptr2;
extern	Boolean		no_action_was_taken;
extern	int		mtool_msgs_fd;
extern	Boolean		no_archconf;
extern	Boolean		no_parallel;
#ifdef SGE_SUPPORT
extern	Boolean		grid;
#endif
extern	Name		no_parallel_name;
extern	Boolean		notparallel;
extern	Name		notparallel_name;
extern	Name		not_auto;
extern	Boolean		only_parallel;
extern	Boolean		parallel;
extern	Name		parallel_name;
extern	Name		localhost_name;
extern	int		parallel_process_cnt;
extern	Percent		percent_list;
extern	Dyntarget	dyntarget_list;
extern	Name		plus;
extern	Name		pmake_machinesfile;
extern	Name		phony;
extern	Name		precious;
extern	Name		primary_makefile;
extern	Boolean		quest;
extern	short		read_trace_level;
extern	Boolean		reading_dependencies;
extern	int		recursion_level;
extern	Name		recursive_name;
extern	short		report_dependencies_level;
extern	Boolean		report_pwd;
extern	Boolean		rewrite_statefile;
extern	Running		running_list;
extern	char		*sccs_dir_path;
extern	Name		sccs_get_name;
extern	Name		sccs_get_posix_name;
extern	Cmd_line	sccs_get_rule;
extern	Cmd_line	sccs_get_org_rule;
extern	Cmd_line	sccs_get_posix_rule;
extern	Name		get_name;
extern	Name		get_posix_name;
extern	Cmd_line	get_rule;
extern	Cmd_line	get_posix_rule;
extern	Boolean		send_mtool_msgs;
extern	Boolean		all_precious;
extern	Boolean		report_cwd;
extern	Boolean		silent_all;
extern	Boolean		silent;
extern	Name		silent_name;
extern	char		*stderr_file;
extern	char		*stdout_file;
#ifdef SGE_SUPPORT
extern	char		script_file[];
#endif
extern	Boolean		stdout_stderr_same;
extern	Name		sunpro_make_name;
extern	Dependency	suffixes;
extern	Name		suffixes_name;
extern	Name		sunpro_dependencies;
extern	Boolean		target_variants;
extern	char		*make_run_dir;
extern	char		*tmpdir;
extern	char		*temp_file_directory;
extern	Name		temp_file_name;
extern	short		temp_file_number;
extern  wchar_t		*top_level_target;
extern	Boolean		touch;
extern	Boolean		trace_reader;
extern	Boolean		build_unconditional;
extern	pathpt		vroot_path;
extern	Name		wait_name;
extern	wchar_t		wcs_buffer2[];
extern	wchar_t		*wcs_ptr;
extern	wchar_t		*wcs_ptr2;
extern	long int	hostid;
extern	Boolean		current_path_reset;

/*
 * Declarations of system defined variables
 */
#if !defined(HAVE_STRSIGNAL)
#ifdef	HAVE_SYS_SIGLIST
#ifndef	HAVE_SYS_SIGLIST_DEF
extern	char		*sys_siglist[];
#endif
#endif
#endif

/*
 * Declarations of system supplied functions
 */
extern	int		file_lock(char *, char *, int *, int);

/*
 * Declarations of functions declared and used by make
 */
extern	void		add_pending(Name target, int recursion_level, Boolean do_get, Boolean implicit, Boolean redo);
extern	void		add_running(Name target, Name true_target, Property command, int recursion_level, int auto_count, Name *automatics, Boolean do_get, Boolean implicit);
extern	void		add_serial(Name target, int recursion_level, Boolean do_get, Boolean implicit);
extern	void		add_subtree(Name target, int recursion_level, Boolean do_get, Boolean implicit);
extern  void 		append_or_replace_macro_in_dyn_array(ASCII_Dyn_Array *Ar, char *macro);
#ifdef DISTRIBUTED
extern	Doname		await_dist(Boolean waitflg);
#endif
#if defined(TEAMWARE_MAKE_CMN) || defined(PMAKE)
extern	void		await_parallel(Boolean waitflg);
#endif
extern	void		build_suffix_list(Name target_suffix);
extern	Boolean		check_auto_dependencies(Name target, int auto_count, Name *automatics);
extern	void		check_state(Name temp_file_name);
extern	void		cond_macros_into_string(Name np, String_rec *buffer);
extern	void		construct_target_string();
extern	void		create_xdrs_ptr(void);
extern	void		depvar_add_to_list (Name name, Boolean cmdline);
#ifdef DISTRIBUTED
extern	void		distribute_rxm(Avo_DoJobMsg *dmake_job_msg);
extern	int 		getRxmMessage(void);
extern	Avo_JobResultMsg* getJobResultMsg(void);
extern	Avo_AcknowledgeMsg* getAcknowledgeMsg(void);
#endif
extern	Doname		doname(Name target, Boolean do_get, Boolean implicit, Boolean automatic = false);
extern	Doname		doname_check(Name target, Boolean do_get, Boolean implicit, Boolean automatic);
extern	Doname		doname_parallel(Name target, Boolean do_get, Boolean implicit);
extern	Doname		dosys(Name command, Boolean ignore_error, Boolean call_make, Boolean silent_error, Boolean always_exec, Name target, Boolean redirect_out_err);
#if defined(TEAMWARE_MAKE_CMN) || defined(PMAKE)
extern	void		job_adjust_posix(void);
#endif
extern	void		dump_make_state(void);
extern	void		dump_target_list(void);
extern	void		enter_conditional(Name target, Name name, Name value, Boolean append);
extern	void		enter_dependencies(Name target, Chain target_group, Name_vector depes, Cmd_line command, Separator separator);
extern	void		enter_dependency(Property line, Name depe, Boolean automatic);
extern	void		enter_equal(Name name, Name value, Boolean append, Separator Separator);
extern  Percent		enter_percent(Name target, Chain target_group, Name_vector depes, Cmd_line command);
extern	Dyntarget	enter_dyntarget(Name target);
extern	Name_vector	enter_name(String string, Boolean tail_present, wchar_t *string_start, wchar_t *string_end, Name_vector current_names, Name_vector *extra_names, Boolean *target_group_seen);
extern	Boolean		exec_vp(char *name, char **argv, char **envp, Boolean ignore_error);
extern	Doname		execute_parallel(Property line, Boolean waitflg, Boolean local = false);
extern	Doname		execute_serial(Property line);
extern	timestruc_t&  	exists(Name target);
extern	void		fatal(const char *, ...);
extern	void		fatal_reader(const char *, ...);
extern	Doname		find_ar_suffix_rule(Name target, Name true_target, Property *command, Boolean rechecking);
extern	Doname		find_double_suffix_rule(Name target, Property *command, Boolean rechecking);
extern	Doname		find_percent_rule(Name target, Property *command, Boolean rechecking);
extern	int		find_run_directory (char *cmd, char *cwd, char *dir, char **pgm, char **run, char *path);
extern	Doname		find_suffix_rule(Name target, Name target_body, Name target_suffix, Property *command, Boolean rechecking);
extern	Chain		find_target_groups(Name_vector target_list, int i, Boolean reset);
extern	void		finish_children(Boolean docheck);
extern	void		finish_running(void);
extern	void		free_chain(Name_vector ptr);
extern  void		gather_recursive_deps(void);
extern	char		*get_current_path(void);
extern	int		get_job_msg_id(void);
extern	FILE		*get_mtool_msgs_fp(void);
#ifdef DISTRIBUTED
extern	Boolean		get_dmake_group_specified(void);
extern	Boolean		get_dmake_max_jobs_specified(void);
extern	Boolean		get_dmake_mode_specified(void);
extern	Boolean		get_dmake_odir_specified(void);
extern	Boolean		get_dmake_rcfile_specified(void);
extern	Boolean		get_pmake_machinesfile_specified(void);
#endif
extern	char		*find_run_dir(void);
#if defined(DISTRIBUTED) || defined(MAKETOOL) /* tolik */
extern	XDR		*get_xdrs_ptr(void);
#endif
extern	wchar_t		*getmem_wc(int size);
extern	void		handle_interrupt(int);
extern	Boolean		is_running(Name target);
extern	void		load_cached_names(void);
extern	void		define_var(const char *name, const char *value);
extern	char		*get_var(const char *name);
extern	void		setup_arch(void);
extern	Boolean		parallel_ok(Name target, Boolean line_prop_must_exists);
extern	void		print_dependencies(Name target, Property line);
extern	void		send_job_start_msg(Property line);
extern	void		send_rsrc_info_msg(int max_jobs, char *hostname, char *username);
extern	void		print_value(Name value, Daemon daemon);
extern	timestruc_t&	read_archive(Name target);
extern	int		read_dir(Name dir, wchar_t *pattern, Property line, wchar_t *library);
extern	void		read_directory_of_file(Name file);
extern	int		read_make_machines(Name make_machines_name);
extern	Boolean		read_simple_file(Name makefile_name, Boolean chase_path, Boolean doname_it, Boolean complain, Boolean must_exist, Boolean report_file, Boolean lock_makefile, Boolean is_include = false);
extern	void		remove_recursive_dep(Name target);
extern	void		report_recursive_dep(Name target, wchar_t *line);
extern	void		report_recursive_done(void);
extern	void		report_recursive_init(void);
extern	Recursive_make	find_recursive_target(Name target);
extern	void		reset_locals(Name target, Property old_locals, Property conditional, int index);
extern	void		set_locals(Name target, Property old_locals);
extern	void		setvar_append(Name name, Name value);
#ifdef DISTRIBUTED
extern	void		setvar_envvar(Avo_DoJobMsg *dmake_job_msg);
#else
extern	void		setvar_envvar(void);
#endif
extern	void		special_reader(Name target, Name_vector depes, Cmd_line command, Separator separator);
extern	void		startup_rxm();
extern	Doname		target_can_be_built(Name target);
extern	char		*time_to_string(const timestruc_t &time);
extern	void		update_target(Property line, Doname result);
extern	void		warning(char *, ...);
extern	void		write_state_file(int report_recursive, Boolean exiting);
extern	Name		vpath_translation(Name cmd);

#define DEPINFO_FMT_VERSION "VERS2$"
#define VER_LEN strlen(DEPINFO_FMT_VERSION)

#ifdef NSE

/* 
 *  NSE version for depinfo format
 */
extern Boolean		nse;
extern Name		nse_backquote_seen;
extern Boolean		nse_did_recursion;
extern Name		nse_shell_var_used;
extern Boolean		nse_watch_vars;
extern wchar_t		current_makefile[MAXPATHLEN];
extern Boolean		nse_depinfo_locked;
extern char		nse_depinfo_lockfile[MAXPATHLEN];
extern Name		derived_src;

extern void		depvar_dep_macro_used(Name);
extern void		depvar_rule_macro_used(Name);
extern Boolean		nse_backquotes(wchar_t *);
extern void		nse_check_cd(Property);
extern void		nse_check_derived_src(Name, wchar_t *, Cmd_line);
extern void		nse_check_file_backquotes(wchar_t *);
extern void		nse_check_no_deps_no_rule(Name, Property, Property);
extern void		nse_check_sccs(wchar_t *, wchar_t *);
extern void		nse_dep_cmdmacro(wchar_t *);
extern int		nse_exit_status(void);
extern void		nse_init_source_suffixes(void);
extern void		nse_no_makefile(Name);
extern void		nse_rule_cmdmacro(wchar_t *);
extern void		nse_wildcard(wchar_t *, wchar_t *);
#endif

#endif
