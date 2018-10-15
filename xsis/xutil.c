/* -------------------------------------------------------------------------- *\
   xutil.c -- xsis utilities not available in the sis util package.

        $Revision: 1.2 $
        $Date: 2005/03/08 01:07:23 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/xutil.c,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.

   The primary utilities in here are some for supporting file completion in
   xsis.  It is based on the POSIX calls to read a directory, and to get
   information for a file.  See directory(3) and stat(2).
\* -------------------------------------------------------------------------- */

#include "xsis.h"
#ifndef MAKE_DEPEND
#include <sys/stat.h>
#include <dirent.h>
#endif


String xsis_string_cat (s1,s2)
String s1;
String s2;
{
    /*	Reallocate s1 as necessary to append s2. */
    String s;

    if (s2 == NULL || *s2 == '\0') {
	s = s1;
    } else if (s1 == NULL) {
	s = XtNewString (s2);
    } else {
	int len = strlen(s1) + strlen(s2) + 1;
	s = strcat(xrealloc(char,s1,len), s2);
    }
    return s;
}

int xsis_perror (s)
String s;
{
    /*	Similar to perror but print errno value also for debugging. */

    fprintf(stderr,"%s: %s (errno=%d)\n",s,strerror(errno),errno);
    return (-1);
}

String xsis_string_trim (s)
String s;
{
    /*  Modify s to remove all trailing whitespace. */

    String p = s + strlen(s);
    if (p > s) {
        for (p--; p > s; p--) if (!isspace(*p)) break;
        *(p+1) = '\0';
    }
    return s;
}


/* ---------------------- Utilities for File Completion --------------------- */

static int xsis_cmp_pathnames (s1,s2)
char** s1;
char** s2;
{
    /*	Used to sort pathnames before returning them. */
    return (strcmp(*s1,*s2));
}


xsis_filec* xsis_file_completion (partial_path,hidden)
char* partial_path;
Bool hidden;
{
    /*	Given a partial pathname, it returns a structure containing: the
	directory name, the partial file name, and an array of possible
	completions of this partial filename.  If hidden is true, then
	filenames which start with a dot are only considered when the
	partial filename also starts with a dot.  Returns NULL if an error
	occurred.  Call xsis_free_filec to dispose of the structure. */

    String filename, suffix;
    xsis_filec *filec;
    struct dirent *dp;
    DIR *dir;
    int flen;

    filec = XtNew(xsis_filec);			/* Initialize return value. */
    filec->filenames = NULL;
    filec->dirname = NULL;
    filec->partial = NULL;
    filec->statpath = NULL;

    filename = strrchr(partial_path,'/');	/* Find file and dir names. */
    if (filename == NULL && partial_path[0] == '~') {
	/* This is a special case when networks are used, so we punt. */
	xsis_free_filec (filec);
	return NULL;
    } else if (filename == NULL) {
	filename = partial_path;
	filec->dirname  = util_strsav(".");
    } else {
	filename++;
	filec->dirname = util_tilde_expand(partial_path);
	suffix = strrchr(filec->dirname,'/');
	if (suffix != NULL) *suffix = '\0';
    }
    filec->partial = XtNewString(filename);

    if ((dir=opendir(filec->dirname)) != NULL) {

	filec->filenames = array_alloc (char*, 10);
	filec->statpath = xalloc(char,MAXNAMLEN);
	strcat(strcpy(filec->statpath,filec->dirname), "/");
	filec->suffix = filec->statpath + strlen(filec->statpath);
	flen = strlen(filec->partial);

	while ((dp=readdir(dir)) != NULL) {
	    if ((flen != 0 || hidden || dp->d_name[0] != '.')
		    && dp->d_reclen >= flen
			&& strncmp(filename,dp->d_name,flen) == 0) {
		filename = XtNewString(dp->d_name);
		array_insert_last (String, filec->filenames, filename);
	    }
	}

	closedir (dir);
	array_sort (filec->filenames,xsis_cmp_pathnames);
    }
    else {
	xsis_free_filec(filec);
	filec = NULL;
    }

    return filec;
}

String xsis_file_type (filec,file_n)
xsis_filec* filec;
int file_n;
{
    /*	Return a string which identifies the type of a file, e.g. / for
	a directory, * for an executable. */

    struct stat srec;
    String type = "";

    strcpy(filec->suffix,array_fetch(String,filec->filenames,file_n));

    if (stat(filec->statpath,&srec) == 0) {
	if ((srec.st_mode&S_IFMT) == S_IFDIR) {
	    type = "/";
	} else if ((srec.st_mode & 0111) != 0) {
	    type = "*";
	}
    }
    return type;
}

void xsis_free_string_array (strings)
array_t* strings;
{
    /*	Free an array of strings which were allocated with malloc. */

    char *one_string;
    int i;

    if (strings == NULL) return;
    for (i=0; i < array_n(strings); i++) {
	one_string = array_fetch (char*, strings, i);
	xfree (one_string);
    }
    array_free(strings);
}

void xsis_free_filec (filec)
xsis_filec* filec;
{
    if (filec == NULL) return;
    FREE (filec->dirname);
    xfree (filec->statpath);
    xfree (filec->partial);
    xsis_free_string_array (filec->filenames);
    xfree (filec);
}

int xsis_string_array_len (strings)
array_t* strings;
{
    /*	Return the length of the longest string in an array of strings. */

    int i, len, max_len = 0;

    for (i=0; i < array_n(strings); i++) {
	len = strlen(array_fetch (char*, strings, i));
	if (len > max_len) max_len = len;
    }
    return max_len;
}

int xsis_string_array_prefix (names)
array_t* names;
{
    /*	Return the length of the longest common prefix of all the strings
	in the array. */

    char *name0, *name;
    int max_prefix = 0, i = array_n(names), j;

    if (i-- != 0) {
	name0 = array_fetch (char*, names, i);
	max_prefix = strlen(name0);
	while (i--) {
	    name = array_fetch (char*, names, i);
	    for (j=max_prefix,max_prefix=0; j--; max_prefix++) {
		if (name[max_prefix] == '\0') break;
		if (name0[max_prefix] != name[max_prefix]) break;
	    }
	}
    }

    return max_prefix;
}
