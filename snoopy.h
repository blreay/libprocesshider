/* $Id: snoopy.h,v 1.4 2010/03/08 22:54:12 ksb Exp $
 *
 * Tune for the snoopy trace module.
 */

/* Log only the actions running under uid 0, set 1 to enable
 */
#if !defined(SNOOPY_ROOT_ONLY)
#define SNOOPY_ROOT_ONLY 1
#endif

/* Maximum size of any argument.  Remember syslog might limit a log
 * message to 8k due to the domain socket message limit.
 */
#if !defined(SNOOPY_MAX_ARG_LENGTH)
#define SNOOPY_MAX_ARG_LENGTH 4096
#endif

/* Trace every process under this process
 */
#if !defined(SNOOPY_DEERTAG)
#define SNOOPY_DEERTAG	0
#endif

/* Any execution of a setuid (setgid) binary drops the LD_PRELOAD in
 * the environment.  Under the deer-tag hook (for op or sudo or at) we
 * don't want the target to go thtough a setuid shell to get out of
 * our trace spell.
 *	"/usr/local/sbin/suexec",	Apache maybe
 * We put a file from a directory in, then any other basenames in
 * that directory.  The code in snoopy.c puts in the sential (char *)0.
 * Without "pt_chown" script won't run, for example.
 */
#if !defined(SNOOPY_NOT_SHELL)
#define SNOOPY_NOT_SHELL			\
	"/usr/bin/lock", "lpr", "lprm", "lpq",	\
	"/usr/libexec/pt_chown",		\
	"/usr/local/etc/installus",		\
	"/usr/local/sbin/installus",		\
	"/usr/local/lib/entomb",		\
	"/usr/sbin/timedc",			\
	"/usr/sbin/traceroute", "traceroute6"
#endif
