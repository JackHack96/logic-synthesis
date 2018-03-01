/* -------------------------------------------------------------------------- *\
   fakesis.c -- simple test program for debugging xsis

	$Revision: 1.1.1.1 $
	$Date: 2004/02/07 10:14:14 $
	$Author: pchong $
	$Source: /users/pchong/CVS/sis/xsis/fakesis.c,v $

   Copyright 1991 by the Regents of the University of California.

   All rights reserved.  Permission to use, copy, modify and distribute
   this software is hereby granted, provided that the above copyright
   notice and this permission notice appear in all copies.  This software
   is made available as is, with no warranties.

   Since the interface to xsis is so trivial, it is often easier to debug
   the communications using this "stub" sis program.  Compile this program,
   and run as "xsis -debug -sis fakesis".  Special commands that fakesis
   recognizes:

	plot		- open an empty network plot window
	cat <file>	- write contents of file to stdout.
	slo <file>	- write contents 1 char/sec to stress-test.
	quit		- exit the program

   Otherwise it just prints a message to you for every line typed.
   The most common problem is how sis cleans up when xsis exits unexpectedly.
   To test this, run xsis with the -debug option, then enter "bailout" which
   forces xsis to do a _exit (this only works in debug mode).  Then you can
   monitor what happens to the orphaned (fake)sis process.  The most
   civilized behavior is for fakesis to receive the SIGHUP signal, which is
   caught and noted in file fakesis.log.  The most barbaric behavior is for
   fakesis to be stranded in an infinite loop, eating CPU time.

   As necessary, other messages to the log can be added to help debug xsis
   as it is ported to new systems.
\* -------------------------------------------------------------------------- */

#include <stdio.h>
#include <strings.h>
#include <signal.h>
#include "xsis.h"

static FILE *fakesis_log;	/* For diagnostic messages. */

static void catch_sighup ()
{
    if(fakesis_log)fprintf(fakesis_log,"Caught SIGHUP\n");
    exit(-1);
}

int main (argc,argv)
int argc;
char** argv;
{
    char buf[1000], *p;
    extern int errno;
    FILE *fin;
    int i, c;
    int slow;

    signal(SIGHUP,catch_sighup);

    /* This opens the main xsis command window. */
    fputs(COM_GRAPHICS_MSG_START,stdout);
    puts("cmd\tnew\tcmd");
    puts(".version\tFakeSIS 1.0");
    fputs(COM_GRAPHICS_MSG_END,stdout);

    fakesis_log = fopen("fakesis.log","w");
    if (fakesis_log == NULL) {
	printf("failed to open fakesis.log.\n");
    } else {
	printf("Log is in file fakesis.log.\n");
    }

#if defined(SYSV)
    printf("SYSV defined: system-V.\n");
#endif
#if defined(__osf__)
    printf("OSF System\n");
#endif
#if defined(SABER)
    printf("Saber-C\n");
#endif

#if defined(sun) && defined(sparc)
    printf("Sun SPARC\n");
#elif defined(_IBMR2) && defined(_AIX)
    printf("IBM RS\n");
#elif defined(ultrix) && defined(mips)
    printf("DECstation\n");
#else
    printf("Generic POSIX.\n");
#endif

    printf("argv: ");
    for (i=0; i < argc; i++) printf(" %s",argv[i]);
    printf("\n");

    printf("Go ahead and type, I'm connected to %s.\n",ttyname(1));
    fputs("sis> ",stdout);

    /*	This emulates the sis command loop in sis/command/source.c.  If
	the sis loop is changed, this should also be modified to match. */

    while (fgets(buf,sizeof(buf),stdin) != NULL) {
	if (fakesis_log) {fprintf(fakesis_log,"%s",buf);fflush(fakesis_log);}
	if (!strcmp(buf,"plot\n")) {
	    fputs(COM_GRAPHICS_MSG_START,stdout);
	    puts("blif\tnew\tsample");
	    puts(COM_GRAPHICS_MSG_END);
	} else if (!strcmp(buf,"quit\n")) {
	    if (fakesis_log) {fprintf(fakesis_log,"<EOF>");fflush(fakesis_log);}
	    sleep (10);
	    if (fakesis_log) {fprintf(fakesis_log,"exit");fflush(fakesis_log);}
	    return 0;
	} else if (!strncmp(buf,"cat ",4) || (slow=!strncmp(buf,"slo ",4))) {
	    /* Try to copy a file to the output. */
	    p = strchr(buf,'\n'); if (p) *p = '\0';
	    if ((fin=fopen(buf+4,"r")) != NULL) {
		while ((c=fgetc(fin)) != EOF) {
		    fputc (c,stdout);
		    if (slow) {fflush(stdout);sleep(1);}
		}
		fclose (fin);
	    } else {
		fprintf(stdout,"No such file.\n");
	    }
	} else {
	    fprintf(stdout,"Would be doing command\n");
	}
	fputs("sis> ",stdout);
    }

    if (fakesis_log) {
	fprintf(fakesis_log,"fgets == NULL, feof=%d, ferror=%d, errno=%d\n",
		feof(stdin), ferror(stdin), errno);
	fflush(fakesis_log);
	fclose (fakesis_log);
    }

    return 0;
}
