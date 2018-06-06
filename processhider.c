#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

typedef struct psinfo {
	char pid[32];;
	char pname[1024];
	char owner[128];
} PSINFO;
/*
 * Every process with this name will be excluded
"QQ.exe",
"WeChat.exe",
"WeChatWeb.exe",
"explorer.exe",
"services.exe",
"winedevice.exe",
"plugplay.exe",
"winedevice.exe",
"rundll32.exe",
 */
static char* g_showall = NULL;
static int g_iShowAll = -1;
static const char* process_to_filter[] = {
"wineserver",
"xpra",
"Xorg",
"Xorg-nosuid",
NULL
};

/* Find the first occurrence in S of any character in ACCEPT.  */
static char * _my_strrpbrk (s, accept)
     const char *s;
     const char *accept;
{
	char* base=s;
	int baselen=strlen(base);
  while (*s != '\0') {
	  const char *rs = base+baselen - (s-base);
      const char *a = accept;
      while (*a != '\0') if (*a++ == *rs) return (char *) (rs+1);
      ++s;
    }
  return NULL;
}

/*
 * Get a directory name given a DIR* handle
 */
static int get_dir_name(DIR* dirp, char* buf, size_t size)
{
    int fd = dirfd(dirp);
    if(fd == -1) {
        return 0;
    }

    char tmp[256];
    snprintf(tmp, sizeof(tmp), "/proc/self/fd/%d", fd);
    ssize_t ret = readlink(tmp, buf, size);
    if(ret == -1) {
        return 0;
    }

    buf[ret] = 0;
    return 1;
}

static void DEBUG(char *fmt, ...) {
    char buf[1024 * 2] = { '\0' };
    char *jestrace;
    static int trace_init = -1;
    va_list args;

    if (trace_init == -1) {
        if ((jestrace = getenv("MYDBG_SHOWDBG")) != NULL && strcmp(jestrace, "DEBUG") == 0) {
            trace_init = 1;
        } else {
            trace_init = 0;
        }
    }
    if (trace_init == 1) {
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args); 
        fprintf(stderr, "[glibc]:%s\n", buf);
    }
}


/*
 * check if hide
 */
static int isToHide(PSINFO* ps)
{
	DEBUG("isToHide: dirname_in=%s", ps->pname);
	//if (NULL != g_showall && strcmp(g_showall, "yes") == 0 ) return 0;
	// .exe or .com return directly
	int n=strlen(ps->pname);
	if (n >=5) {
		if (0 == strcasecmp(ps->pname+n-4, ".exe") || 0 == strcasecmp(ps->pname+n-4, ".com")) {
			DEBUG("-%s %s", ps->pid, ps->pname);
			return 1;
		}
	}

	// search array
	int i=0;
	while (process_to_filter[i] != NULL) {
		//DEBUG("ps->pname=%s process_to_filter[%d]=%s", ps->pname, i, process_to_filter[i]);
		if (strcmp(ps->pname, process_to_filter[i]) == 0 ) {
			DEBUG("%s %s", ps->pid, ps->pname);
			return 1;
		}
		i++;
	}
    return 0;
}
/*
 * Get a process name given its pid
 */
static int get_process_info(char* pid, PSINFO* ps)
{
    if(strspn(pid, "0123456789") != strlen(pid)) {
        return 0;
    }

    char tmp[4096]; 
	//snprintf(tmp, sizeof(tmp), "/proc/%s/stat", pid);
    snprintf(tmp, sizeof(tmp), "/proc/%s/cmdline", pid);
 
    FILE* f = fopen(tmp, "r");
    if(f == NULL) {
        return 0;
    }

    if(fgets(tmp, sizeof(tmp), f) == NULL) {
        fclose(f);
        return 0;
    }

    fclose(f);
	DEBUG("read buf: %s", tmp);

    int unused;
    //sscanf(tmp, "%d (%[^)]s", &unused, ps->pname);
	// fine the last slash or backslash 
	char* p=_my_strrpbrk(tmp, "/\\");
	DEBUG("read buf p1: %s", p);
	strcpy(ps->pname, p==NULL?tmp:p);
	strcpy(ps->pid, pid);
	DEBUG("read buf p: %s", p==NULL?tmp:p);
    return 1;
}

#define DECLARE_READDIR(dirent, readdir)                                \
static struct dirent* (*original_##readdir)(DIR*) = NULL;               \
                                                                        \
struct dirent* readdir(DIR *dirp)                                       \
{                                                                       \
	if (-1 == g_iShowAll) {                                            \
		g_showall=getenv("MYDBG_SHOWALL");                             \
	    g_iShowAll=(NULL != g_showall && strcmp(g_showall, "yes") == 0 )?1:0;       \
	}                                                                   \
    if(original_##readdir == NULL) {                                    \
        original_##readdir = dlsym(RTLD_NEXT, "readdir");               \
        if(original_##readdir == NULL) {                                \
            fprintf(stderr, "Error in dlsym: %s\n", dlerror());         \
        }                                                               \
    }                                                                   \
                                                                        \
    struct dirent* dir;                                                 \
                                                                        \
    while(1)                                                            \
    {                                                                   \
        dir = original_##readdir(dirp);                                 \
        if(g_iShowAll != 1 && dir) {                                    \
            char dir_name[4096];                                        \
			PSINFO ps;                                                  \
			memset(&ps, sizeof(ps), 0x0);                               \
            if(get_dir_name(dirp, dir_name, sizeof(dir_name)) &&        \
                strcmp(dir_name, "/proc") == 0 &&                       \
                get_process_info(dir->d_name, &ps) &&                   \
                isToHide(&ps) == 1) {                                   \
                continue;                                               \
            }                                                           \
        }                                                               \
        break;                                                          \
    }                                                                   \
    return dir;                                                         \
}

DECLARE_READDIR(dirent64, readdir64);
DECLARE_READDIR(dirent, readdir);
