#include "sis.h"

#include <sys/stat.h>

#include <readline/readline.h>
#include <readline/history.h>

extern void init_sis();
extern void end_sis();

static int main_has_restarted = 0;
static network_t *network; /* allows for restart ... */

static int source_sisrc(network)
	network_t **network; {
	char *cmdline;
	char *lib_name;
	char *homefile;
	struct stat home, cur;
	int s1, s2; /* Flags for checking the stat() call */
	int status0, status1, status2, status3, status4, status5;

	lib_name = sis_library();
	cmdline = ALLOC(char, strlen(lib_name) + 20);

	(void) sprintf(cmdline, "source -s %s/.misrc", lib_name);
	status0 = com_execute(network, cmdline);
	(void) sprintf(cmdline, "source -s %s/.sisrc", lib_name);
	status1 = com_execute(network, cmdline);
	status3 = com_execute(network, "source -s ~/.sisrc");

	homefile = util_tilde_expand("~/.misrc");
	s1 = stat(homefile, &home);
	s2 = stat(".misrc", &cur);
	status2 = status4 = TRUE;
	if ((s1 == 0) && (s2 == 0) && (home.st_ino == cur.st_ino)) {
		/* ~/.misrc == .misrc : Source the file only once */
		status2 = com_execute(network, "source -s ~/.misrc");
	} else {
		if (s1 == 0) { /* ~/.misrc exists and can be opened */
			status2 = com_execute(network, "source -s ~/.misrc");
		}
		if (s2 == 0) { /* ./.misrc exists and can be opened */
			status4 = com_execute(network, "source -s .misrc");
		}
	}
	FREE(homefile);

	homefile = util_tilde_expand("~/.sisrc");
	s1 = stat(homefile, &home);
	s2 = stat(".sisrc", &cur);
	status3 = status5 = TRUE;
	if ((s1 == 0) && (s2 == 0) && (home.st_ino == cur.st_ino)) {
		/* ~/.sisrc == .sisrc : Source the file only once */
		status3 = com_execute(network, "source -s ~/.sisrc");
	} else {
		if (s1 == 0) {
			status3 = com_execute(network, "source -s ~/.sisrc");
		}
		if (s2 == 0) {
			status5 = com_execute(network, "source -s .sisrc");
		}
	}
	FREE(homefile);

	FREE(cmdline);
	FREE(lib_name);
	return status0 && status1 && status2 && status3 && status4 && status5;
}

static void usage(prog)
	char *prog; {
	char *lib_name;

	(void) fprintf(miserr, "%s\n", sis_version());
	(void) fprintf(miserr,
			"usage: %s [-sx] [-c cmd] [-f script] [-o file] [-t type] [-T type] [file]\n",
			prog);
#ifdef SIS
	(void) fprintf(miserr, "    -c cmd\texecute SIS commands `cmd'\n");
	(void) fprintf(miserr, "    -f file\texecute SIS commands from a file\n");
#else
	(void) fprintf(miserr,
			"    -c cmd\texecute MIS commands `cmd'\n");
	(void) fprintf(miserr,
			"    -f file\texecute MIS commands from a file\n");
#endif
	(void) fprintf(miserr,
			"    -o file\tspecify output filename (default is -)\n");
	lib_name = sis_library();
#ifdef SIS
	(void) fprintf(miserr, "    -s\t\tsuppress initial 'source %s/.sisrc'\n",
			sis_library());
#else
	(void) fprintf(miserr,
			"    -s\t\tsuppress initial 'source %s/.misrc'\n", sis_library());
#endif
	FREE(lib_name);
#ifdef SIS
	(void) fprintf(miserr,
			"    -t type\tspecify input type (blif, eqn, kiss, oct, pla, slif, or none)\n");
	(void) fprintf(miserr,
			"    -T type\tspecify output type (blif, eqn, kiss, oct, pla, slif, or none)\n");
#else
	(void) fprintf(miserr,
			"    -t type\tspecify input type (bdnet, blif, eqn, oct, pla, or none)\n");
	(void) fprintf(miserr,
			"    -T type\tspecify output type (bdnet, blif, eqn, oct, pla, or none)\n");
#endif
	(void) fprintf(miserr, "    -x\t\tequivalent to '-t none -T none'\n");
	exit(2);
}

static int check_type(s)
	char *s; {
	if (strcmp(s, "bdnet") == 0) {
		return 1;
	} else if (strcmp(s, "blif") == 0) {
		return 1;
	} else if (strcmp(s, "eqn") == 0) {
		return 1;
#ifdef SIS
	} else if (strcmp(s, "kiss") == 0) {
		return 1;
#endif
	} else if (strcmp(s, "oct") == 0) {
		return 1;
	} else if (strcmp(s, "pla") == 0) {
		return 1;
#ifdef SIS
	} else if (strcmp(s, "slif") == 0) {
		return 1;
#endif
	} else if (strcmp(s, "none") == 0) {
		return 1;
	} else {
		(void) fprintf(miserr, "unknown type %s\n", s);
		return 0;
	}
}

int main(argc, argv)
	int argc;char **argv; {
	int c, status, batch, initial_source, initial_read, final_write;
	int quit_flag;
	char readcmd[20], writecmd[20];
	char *dummy, *cmdline, *cmdline1, *infile, *outfile;

	program_name = argv[0];

	quit_flag = -1; /* Quick quit */
	util_getopt_reset();

	if (main_has_restarted) {
		(void) fprintf(stderr, "Restarting frozen image ...\n");
	} else {
		main_has_restarted = 1;
		init_sis();
		network = network_alloc();
	}

	cmdline = util_strsav("");
	(void) strcpy(readcmd, "read_blif");
	(void) strcpy(writecmd, "write_blif");
	infile = "-";
	outfile = "-";
	command_hist = array_alloc(char *, 0);
	initial_source = 1;
	initial_read = 1;
	final_write = 1;
	batch = 0;
	util_getopt_reset();
	while ((c = util_getopt(argc, argv, "c:f:o:st:T:xX:")) != EOF) {
		switch (c) {
		case 'c':
			FREE(cmdline);
			cmdline = util_strsav(util_optarg);
			batch = 1;
			break;

		case 'f':
			FREE(cmdline);
			cmdline = ALLOC(char, strlen(util_optarg) + 20);
			(void) sprintf(cmdline, "source %s", util_optarg);
			batch = 1;
			break;

		case 'o':
			outfile = util_optarg;
			break;

		case 's':
			initial_source = 0;
			break;

		case 't':
			if (check_type(util_optarg)) {
				if (strcmp(util_optarg, "none") == 0) {
					initial_read = 0;
				} else {
					(void) sprintf(readcmd, "read_%s", util_optarg);
				}
			} else {
				usage(argv[0]);
			}
			batch = 1;
			break;

		case 'T':
			if (check_type(util_optarg)) {
				if (strcmp(util_optarg, "none") == 0) {
					final_write = 0;
				} else {
					(void) sprintf(writecmd, "write_%s", util_optarg);
				}
			} else {
				usage(argv[0]);
			}
			batch = 1;
			break;

		case 'x':
			final_write = 0;
			initial_read = 0;
			batch = 1;
			break;

		case 'X':
			/* Handled in previous option loop. */
			break;

		default:
			usage(argv[0]);
		}
	}

	if (!batch) {
		/* interactive use ... */
		if (argc - util_optind != 0) {
			(void) fprintf(miserr, "warning -- trailing arguments ignored\n");
		}

		(void) fprintf(misout, "%s\n", sis_version());
		if (initial_source) {
			(void) source_sisrc(&network);
		}
		//char *line;
		do{
			cmdline=readline("sis> ");
			if(cmdline){
				add_history(cmdline);
				quit_flag=com_execute(&network, cmdline);
			}
		}while(quit_flag>=0);
		//while ((quit_flag = com_execute(&network, "source -ip -")) >= 0);
		status = 0;

	} else {

		/* read initial network */
		if (argc - util_optind == 0) {
			infile = "-";
		} else if (argc - util_optind == 1) {
			infile = argv[util_optind];
		} else {
			usage(argv[0]);
		}

		if (initial_source) {
			(void) source_sisrc(&network);
		}

		status = 0;
		if (initial_read) {
			cmdline1 = ALLOC(char, strlen(infile) + 20);
			(void) sprintf(cmdline1, "%s %s", readcmd, infile);
			status = com_execute(&network, cmdline1);
			FREE(cmdline1);
		}

		if (status == 0) {
			status = com_execute(&network, cmdline);
			if ((status == 0 || status == -1) && final_write) {
				cmdline1 = ALLOC(char, strlen(outfile) + 20);
				(void) sprintf(cmdline1, "%s %s", writecmd, outfile);
				status = com_execute(&network, cmdline1);
				FREE(cmdline1);
			}
		}

	}

	FREE(cmdline);
	for (c = array_n(command_hist); c-- > 0;) {
		dummy = array_fetch(char *, command_hist, c);
		FREE(dummy);
	}
	array_free(command_hist);
	/* Value of "quit_flag" is determined by the "quit" command */
	if (quit_flag == -1 || quit_flag == -2) {
		status = 0;
	}
	if (quit_flag == -2) {
		network_free(network);
		end_sis();
	}
	exit(status);
}
