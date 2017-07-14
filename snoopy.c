/* snoopy.c -- execve() logging wrapper
 * Copyright (c) 2000 marius@linux.com,mbm@linux.com
 *
 * $Id: snoopy.c,v 1.9 2010/03/15 14:21:55 ksb Exp $
 *
 * Part hacked on flight KL 0617, 30,000 ft or so over the Atlantic :)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Compile(Tagged): ${gcc:-gcc} -DSNOOPY_ROOT_ONLY=0 -DSNOOPY_PATH=\\"/usr/local/lib/snoopy-tagged.so\\" -DSNOOPY_DEERTAG=1 -shared -O3 -fomit-frame-pointer -fPIC %f -o %F-tagged.so -ldl
 * $Compile: ${gcc:-gcc} -shared -O3 -fomit-frame-pointer -fPIC %f -o %F.so -ldl
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>
#include <sysexits.h>
#include "snoopy.h"

#if !defined(SNOOPY_PATH)
#  define SNOOPY_PATH	"/usr/local/lib/snoopy.so"
#endif

#if defined(RTLD_NEXT)
#  define REAL_LIBC RTLD_NEXT
#else
#  define REAL_LIBC ((void *) -1L)
#endif

static inline int
ExecOK(struct stat *pstExec, const char **ppcList)
{
	auto struct stat stCheck;
	auto char acPath[PATH_MAX+4];
	register const char *pc;
	register char *pcTail;

	strcpy(acPath, "./");
	while ((char *)0 != (pc = *ppcList++)) {
		if ('/' == *pc || (char *)0 == (pcTail = strrchr(acPath, '/')))
			strncpy(acPath, pc, sizeof(acPath));
		else
			strncat(pcTail+1, pc, sizeof(acPath)+(pcTail-acPath));
		if (-1 == stat(acPath, &stCheck))
			continue;
		if (pstExec->st_dev == stCheck.st_dev && pstExec->st_ino == stCheck.st_ino)
			return 1;
	}
	return 0;
}

/* The common log we want for all process creations:		(mbm + ksb)
 * output a session line for any change in the process env, so we can
 * trace actions (ppid, pwd, session-id, uid, tty).  Trace each process
 * executed, we need to know the pwd to know which "ls" was run.  We
 * can't trace shell built-in (like echo) but that's usually not a big
 * issue.  This is way better than nothing.
 */
static inline void
snoopy_log(const char *filename, char *const *argv)
{
	static const char acNone[] = ".";
	static const char acEnvName[] = "LD_PRELOAD";
	static const char acSnoopy[] = SNOOPY_PATH;
	register int argc = 0;
#if SNOOPY_DEERTAG
	auto struct stat stExec;
#endif

	/* I never use the root only code because it doesn't help.
	 * I want to trace an escalated shell and all the process started
	 * from it, even if it runs as "bin" which is just as bad as root.
	 */
#if SNOOPY_ROOT_ONLY
	if (0 != geteuid() && 0 != getuid()) {
		return;
	}
#endif

	/* Count number of arguments
	 */
	for (argc = 0; (char *)0 != argv[argc]; ++argc) {
		/* count them */
	}
	openlog("snoopy", LOG_PID, LOG_AUTHPRIV);

#if SNOOPY_DEERTAG
	/* If they are trying to setuid (setgid) out we can stop them. This
	 * prevents the target from running "netstat" and "atrm", which is
	 * a bummer, unless add them a white-list here, since anything that
	 * masks a shell is bad.  See comments in snoopy.h at /NOT_SHELL/
	 */
	{
	static const char *apcOK[] = {
		SNOOPY_NOT_SHELL, (char *)0
	};

	if (-1 == stat(filename, &stExec)) {
		fprintf(stderr, "snoopy.so: stat: %s: %s\n", filename, strerror(errno));
		exit(EX_UNAVAILABLE);
	}
	if (0 != ((S_ISUID|S_ISGID)&stExec.st_mode) && !ExecOK(&stExec, apcOK)) {
		fprintf(stderr, "snoopy.so: %s: %s\n", filename, strerror(EPERM));
		exit(EX_PROTOCOL);
	}
	}
#endif

	/* Get current working directory and session, if they have changed
	 * then we need to log the new session.
	 */
	{
	static char *oldCwd = (char *)0;
	static int oldSession = -7;	// never a valid session
	register int iSesson;
	auto const char *getCwdRet = (const char *)0;
	auto char acCwd[PATH_MAX+4];

	if ((char *)0 == (getCwdRet = getcwd(acCwd, PATH_MAX+1))) {
		getCwdRet = acNone;
	}
	iSesson = getsid(0);
	if ((char *)0 == oldCwd || 0 != strcmp(oldCwd, getCwdRet) || oldSession != iSesson) {
		auto char acTty[PATH_MAX+4];

		if ((char *)0 != oldCwd) {
			free((void *)oldCwd);
		}
		oldCwd = strdup(getCwdRet);
		oldSession = iSesson;
		if (0 != ttyname_r(0, acTty, sizeof(acTty)) && 0 != ttyname_r(2, acTty, sizeof(acTty)) && 0 != ttyname_r(1, acTty, sizeof(acTty))) {
			(void)strcpy(acTty, acNone);
		}
		syslog(LOG_INFO, "ppid=%d uid=%d sid=%d tty=%s cwd=%s", getppid(), getuid(), iSesson, acTty, getCwdRet);
	}
	}

	/* Allocate memory for logString, each param + 1 space
	 * if we overflowed the argument limit note it in the log.
	 * Then Log it and free the buffer.
	 */
	{
	register int cOver = '+';
	register int i = 0;
	register size_t argLength = 0;
	auto size_t logLength = 0;
	auto char *logString = NULL;

	logLength = strlen(filename)+2;
	for (i = 0; i < argc; ++i) {
		argLength = strlen(argv[i]);
		if (SNOOPY_MAX_ARG_LENGTH < argLength) {
			cOver = '*';
			argLength = SNOOPY_MAX_ARG_LENGTH;
		}
		logLength += argLength+1;
	}
	logString = malloc(logLength);

	/* Fill that buffer with the args we promised
	 */
	logString[0] = '\000';
	strcat(logString, filename);
	for (i = 0; i < argc; ++i) {
		strcat(logString, " ");
		argLength = strlen(argv[i]);
		if (SNOOPY_MAX_ARG_LENGTH < argLength) {
			argLength = SNOOPY_MAX_ARG_LENGTH;
		}
		(void)strncat(logString, argv[i], argLength);
	}

#if SNOOPY_DEERTAG
	/* Set cOver to '!' if the command being run is a white-listed
	 * set{u,g}id file.
	 */
	if (0 != ((S_ISUID|S_ISGID)&stExec.st_mode)) {
		cOver = '!';
	}
#endif

	syslog(LOG_INFO, "%c%s", cOver, logString);
	free((void *)logString);
	}

	/* Tag the next process to do the same thing, we are about
	 * to exec a process: make sure we are the first pre-load.
	 * If the process is setuid we might loose, but that's a
	 * violation of policy we could trap above.
	 */
	{
	register char *pcMem, *pcEnv;

	if ((char *)0 == (pcEnv = getenv(acEnvName))) {
		setenv(acEnvName, acSnoopy, 1);
	} else if (0 == strncmp(pcEnv, acSnoopy, sizeof(acSnoopy)-1) && ('\000' == pcEnv[sizeof(acSnoopy)] || ':' == pcEnv[sizeof(acSnoopy)])) {
		/* nada */
	} else if ((char *)0 != (pcMem = malloc(strlen(pcEnv)+1+sizeof(acSnoopy)+1))) {
		strcpy(pcMem, acSnoopy);
		strcat(pcMem, ":");
		strcat(pcMem, pcEnv);
		(void)setenv(acEnvName, pcMem, 1);
	}
	}
}

/* The generic "find the real one" code
 */
#define FN(ptr,type,name,args)	(ptr = (type (*)args)dlsym(REAL_LIBC, name))

/* Our execve logs, then runs the real one				(mbm)
 */
int execve(const char *filename, char *const argv[], char *const envp[])
{
	static int (*func)(const char *, char **, char **);

	FN(func,int,"execve",(const char *, char **const, char **const));
	snoopy_log(filename, argv);

	return (*func)(filename, (char**) argv, (char **) envp);
}


/* Our execv logs, then runs the real one, same pattern as above	(mbm)
 */
int execv(const char *filename, char *const argv[]) {
	static int (*func)(const char *, char **);

	FN(func,int,"execv",(const char *, char **const));
	snoopy_log(filename, argv);

	return (*func)(filename, (char **) argv);
}
