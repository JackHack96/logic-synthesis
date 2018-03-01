/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/port/autoconf.h,v $
 * $Author: pchong $
 * $Revision: 1.3 $
 * $Date: 2004/03/27 11:01:36 $
 *
 */
#ifndef AUTOCONF_H
#define AUTOCONF_H

#include "config.h"

#include <sys/types.h>

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined(_POSIX_VERSION)
#define RETWAITTYPE int
#undef wait3
#define wait3(x,y,z) waitpid(0,(x),(y));
#else
#define RETWAITTYPE union wait
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif

#if defined(_POSIX_VERSION) || defined(__CYGWIN__)
#define USE_TERMIO
#else
#undef USE_TERMIO
#endif

#endif /* AUTOCONF_H */
