
/* LINTLIBRARY */

#include <stdio.h>
#include "util.h"

#ifdef BSD_SIS
#include <pwd.h>
#endif


char *
util_tilde_expand(fname)
char *fname;
{
#ifdef BSD_SIS
    struct passwd *userRecord;
    char username[256], *filename, *dir;
    register int i, j;

    filename = ALLOC(char, strlen(fname) + 256);

    /* Clear the return string */
    i = 0;
    filename[0] = '\0';

    /* Tilde? */
    if (fname[0] == '~') {
	j = 0;
	i = 1;
	while ((fname[i] != '\0') && (fname[i] != '/')) {
	    username[j++] = fname[i++];
	}
	username[j] = '\0';
	dir = (char *)0;
	if (username[0] == '\0') {
	    /* ~/ resolves to home directory of current user */
	    userRecord = getpwuid(getuid());
	    if (userRecord) dir = userRecord->pw_dir;
	} else {
	    /* Special check for ~octtools */
	    if (!strcmp(username,"octtools"))
	        dir = getenv("OCTTOOLS");
	    /* ~user/ resolves to home directory of 'user' */
	    if (!dir) {
	        userRecord = getpwnam(username);
		if (userRecord) dir = userRecord->pw_dir;
	    }
	}
	if (dir) (void) strcat(filename, dir);
	else i = 0;	/* leave fname as-is */
    } /* if tilde */

    /* Concantenate remaining portion of file name */
    (void) strcat(filename, fname + i);
    return filename;
#else
    return util_strsav(fname);
#endif
}
