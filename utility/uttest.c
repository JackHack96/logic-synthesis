/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/utility/uttest.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:13 $
 *
 */
#include "copyright.h"
#include "port.h"
#include "utility.h"

/*ARGSUSED*/
main(argc, argv)
int argc;
char **argv;
{
    FILE *in, *out;
    char results[8][4096];
    char *args[8];
    char *file, *str;
    int dummy, i, j;
    long time;
    int pid;

    /* test tilde expander */
    if (strcmp(util_tilde_expand("/tmp"), "/tmp") != 0) {
	fprintf(stderr, "expansion of /tmp failed\n");
	exit(-1);
    }
    if (strcmp(util_tilde_expand("fred"), "fred") != 0) {
	fprintf(stderr, "expansion of fred failed\n");
	exit(-1);
    }
    if (strcmp(util_tilde_expand("./fred"), "./fred") != 0) {
	fprintf(stderr, "expansion of ./fred failed\n");
	exit(-1);
    }
    if (strcmp(util_tilde_expand("~bigwilly"), "~bigwilly") != 0) {
	fprintf(stderr, "expansion of ~bigwilly failed\n");
	exit(-1);
    }
    if (strcmp(util_tilde_expand("~root"), "/") != 0) {
	fprintf(stderr, "expansion of ~root failed\n");
	exit(-1);
    }
    if (strcmp(util_tilde_compress(util_tilde_expand("~octtools/.cshrc")),
	       "~octtools/.cshrc") != 0) {
	fprintf(stderr, "compression of ~octtools failed\n");
	exit(-1);
    }


    util_register_user("non-existent-user", "/non-existent-location");
    if (strcmp(util_tilde_expand("~non-existent-user/.cshrc"),
	       "/non-existent-location/.cshrc") != 0) {
	fprintf(stderr, "expansion of ~non-existent-user failed\n");
	exit(-1);
    }
    if (strcmp(util_tilde_compress(util_tilde_expand("~non-existent-user/.cshrc")),
	       "~non-existent-user/.cshrc") != 0) {
	fprintf(stderr, "compression of ~non-existent-user failed\n");
	exit(-1);
    }

    /* check out OCTTOOLS-TRANSLATIONS */
    if (strcmp(util_tilde_expand("~first-translation/.cshrc"),
	       "/first/.cshrc") != 0) {
	fprintf(stderr, "expansion of ~first failed\n");
	exit(-1);
    }
    if (strcmp(util_tilde_expand("~second-translation/.cshrc"),
	       "/second/.cshrc") != 0) {
	fprintf(stderr, "expansion of ~second failed\n");
	exit(-1);
    }
    if (strcmp(util_tilde_expand("~third-translation/.cshrc"),
	       "/third/.cshrc") != 0) {
	fprintf(stderr, "expansion of ~third failed\n");
	exit(-1);
    }
    if (strcmp(str = util_tilde_compress(util_tilde_expand("~third-translation/.cshrc")),
	       "~third-translation/.cshrc") != 0) {
	fprintf(stderr, "compression of ~third failed:\n");
	fprintf(stderr, "`%s' != `%s'\n", str, "~third-translation/.cshrc");
	exit(-1);
    }

    /* verify that multiple calls can be made to util_tilde_expand */
    j = 0;
    (void) strcpy(results[j++], util_tilde_expand("~/fred"));
    if (results[j-1][0] == '~') {
	fprintf(stderr, "expansion of ~/fred failed\n");
	exit(-1);
    }
    (void) strcpy(results[j++], util_tilde_expand("~octtools"));
    if (results[j-1][0] == '~') {
	fprintf(stderr, "expansion of ~octtools failed\n");
	exit(-1);
    }
    (void) strcpy(results[j++], util_tilde_expand("~ricks/.login"));

    for (i = 0; i < j; i++) {
	fprintf(stderr, "\t%s\n", results[i]);
    }

    (void) util_csystem("who");

    if ((file = util_file_search("uttest.c", "/tmp:..:../utility:.:/usr/tmp", "r")) == NIL(char)) {
	fprintf(stderr, "could not find uttest.c\n");
	exit(-1);
    }
    fprintf(stderr, "file is %s\n", file);
    if (util_file_search("BigWillyWasHere", "/usr/tmp/:/tmp:.", "r") != NIL(char)) {
	fprintf(stderr, "found BigWillyWasHere\n");
	exit(-1);
    }
    if ((file = util_path_search("make")) == NIL(char)) {
	fprintf(stderr, "could not find make\n");
	exit(-1);
    }
    fprintf(stderr, "program is %s\n", file);

    /* test pipefork */
    args[0] = util_strsav("sort");
    args[1] = util_strsav("-n");
    args[2] = 0;

    if (util_pipefork(args, &in, &out, &pid) != 1) {
	(void) fprintf(stderr, "bad return from util_pipefork\n");
	exit(-1);
    }

    for (i = 20; i >= 0; i--) {
	(void) fprintf(in, "%d\n", i);
    }
    (void) fclose(in);

    i = 0;
    while (fscanf(out, "%d", &dummy) == 1) {
	if (i != dummy) {
	    (void) fprintf(stderr, "util_pipefork sort failed\n");
	    exit(-1);
	}
	i++;
    }
    if (i != 21) {
	(void) fprintf(stderr, "util_pipefork short count (%d)\n", i);
	exit(-1);
    }

    args[0] = util_strsav("betterNotExist");
    args[1] = 0;

    if (util_pipefork(args, &in, &out, &pid) == 1) {
	(void) fprintf(stderr, "successfull return from util_pipefork, should have been a failure\n");
	exit(-1);
    }

    time = util_cpu_time();
    fprintf(stderr, "time is %d / %s\n", time, util_print_time(time));

    exit(0);
}

