/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/sis/util/pathsearch.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:53 $
 *
 */
/* LINTLIBRARY */

#include <stdio.h>
#ifdef UNIX
#include <sys/file.h>
#include <sys/stat.h>
#endif
#include "util.h"


static int
check_file(filename, mode)
char *filename;
char *mode;
{
#ifdef UNIX
    struct stat stat_rec;
    int access_char = mode[0];
    int access_mode = R_OK;

    /* First check that the file is a regular file. */

    if (stat(filename,&stat_rec) == 0
		    && (stat_rec.st_mode&S_IFMT) == S_IFREG) {
	if (access_char == 'w') {
	    access_mode = W_OK;
	} else if (access_char == 'x') {
	    access_mode = X_OK;
	}
	return access(filename,access_mode) == 0;
    }
    return 0;
#else
    FILE *fp;
    int got_file;

    if (strcmp(mode, "x") == 0) {
	mode = "r";
    }
    fp = fopen(filename, mode);
    got_file = (fp != 0);
    if (fp != 0) {
	(void) fclose(fp);
    }
    return got_file;
#endif
}


char *
util_path_search(prog)
char *prog;
{
#ifdef UNIX
    return util_file_search(prog, getenv("PATH"), "x");
#else
    return util_file_search(prog, NIL(char), "x");
#endif
}


char *
util_file_search(file, path, mode)
char *file;			/* file we're looking for */
char *path;			/* search path, colon separated */
char *mode;			/* "r", "w", or "x" */
{
    int quit;
    char *buffer, *filename, *save_path, *cp;

    if (path == 0 || strcmp(path, "") == 0) {
	path = ".";		/* just look in the current directory */
    }

    save_path = path = util_strsav(path);
    quit = 0;
    do {
	cp = strchr(path, ':');
	if (cp != 0) {
	    *cp = '\0';
	} else {
	    quit = 1;
	}

	/* cons up the filename out of the path and file name */
	if (strcmp(path, ".") == 0) {
	    buffer = util_strsav(file);
	} else {
	    buffer = ALLOC(char, strlen(path) + strlen(file) + 4);
	    (void) sprintf(buffer, "%s/%s", path, file);
	}
	filename = util_tilde_expand(buffer);
	FREE(buffer);

	/* see if we can access it */
	if (check_file(filename, mode)) {
	    FREE(save_path);
	    return filename;
	}
	FREE(filename);
	path = ++cp;
    } while (! quit); 

    FREE(save_path);
    return 0;
}
