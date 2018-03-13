/*
 * Revision Control Information
 *
 * $Source: /users/pchong/CVS/sis/nova/options.c,v $
 * $Author: pchong $
 * $Revision: 1.1.1.1 $
 * $Date: 2004/02/07 10:14:10 $
 *
 */
#include "inc/nova.h"

/**********************************************************************/
/*              Here is the command_line routine                      */
/*                                                                    */
/**********************************************************************/

options(argc, argv)
int argc;
char *argv[];
{
    BOOLEAN encoding_algo = FALSE;
    register int i,k;
    register int infile = 0;
    void usage(); 

    for (i = 1; i < argc; i++) {

        if (!(strcmp(argv[i],"-i"))) {
            i++;
            if (i >= argc) {
                fprintf(stderr,"no argument for input code length!\n");
                exit(-1);
            }
            inp_codelength = atoi(argv[i]);
	    IBITS_DEF = FALSE;
        }

        else if (!(strcmp(argv[i],"-s"))) {
            i++;
            if (i >= argc) {
                fprintf(stderr,"no argument for state code length!\n");
                exit(-1);
            }
            st_codelength = atoi(argv[i]);
	    SBITS_DEF = FALSE;
        }

        else if (!(strcmp(argv[i],"-o"))) {
            i++;
            if (i >= argc) {
                /*fprintf(stderr,"no argument for output code length!\n");
                exit(-1);*/
            }
            /*out_codelength = atoi(argv[i]);
	    OBITS_DEF = FALSE;*/
        }


        else if (!(strcmp(argv[i],"-c"))) {
            i++;
            if (i >= argc) {
                fprintf(stderr,"no argument for cost function in sa!\n");
                exit(-1);
            }
            cost_function = atoi(argv[i]);
			if (cost_function < 0 || cost_function > 1) {
                fprintf(stderr,"Cost function of sa may be only 0 or 1!\n");
                exit(-1);
            }
        }

        else if (!(strcmp(argv[i],"-m"))) {
            i++;
            if (i >= argc) {
                fprintf(stderr,"no argument for # of moves in sa!\n");
                exit(-1);
            }
            num_moves = atoi(argv[i]);
			if (num_moves < 1) {
                fprintf(stderr,"Invalid # of moves in sa (non-pos. int.)!\n");
                exit(-1);
            }
        }

        else if (!(strcmp(argv[i],"-n"))) {
            i++;
            if (i >= argc) {
                fprintf(stderr,"no argument for # of random trials!\n");
                exit(-1);
            }
            rand_trials = atoi(argv[i]);
			if (rand_trials < 1) {
                fprintf(stderr,"Invalid # of random trials!\n");
                exit(-1);
            }
        }

        else if (!(strcmp(argv[i],"-e"))) {
            if (encoding_algo == FALSE) {
                i++;
                if (!(strcmp(argv[i],"ig"))) {
		    I_GREEDY = TRUE;
                    encoding_algo = TRUE;
		}
                else if (!(strcmp(argv[i],"ih"))) {
		    I_HYBRID = TRUE;
                    encoding_algo = TRUE;
		}
                else if (!(strcmp(argv[i],"ie"))) {
		    I_EXACT = TRUE;
                    encoding_algo = TRUE;
		}
                else if (!(strcmp(argv[i],"ioh"))) {
		    IO_HYBRID = TRUE;
                    encoding_algo = TRUE;
		}
                else if (!(strcmp(argv[i],"iov"))) {
		    IO_VARIANT = TRUE;
                    encoding_algo = TRUE;
		}
                else if (!(strcmp(argv[i],"h"))) {
		    ONEHOT = TRUE;
                    encoding_algo = TRUE;
		}
                else if (!(strcmp(argv[i],"r"))) {
		    RANDOM = TRUE;
                    encoding_algo = TRUE;
		}
                else if (!(strcmp(argv[i],"u"))) {
		    USER = TRUE;
                    encoding_algo = TRUE;
		}
                else if (!(strcmp(argv[i],"ia"))) {
		    I_ANNEAL = TRUE;
                    encoding_algo = TRUE;
		}
            }
            else {
                fprintf(stderr,"More than one encoding_algo!\n");
                exit(-1);
            }
	    if (encoding_algo == FALSE) usage(1);
        }

        else if (!(strcmp(argv[i],"-z"))) {
            i++;
            if (i >= argc) {
                fprintf(stderr,"no argument for -z!\n");
                exit(-1);
            }
            strcpy(zero_state,argv[i]);
            ZERO_FL = TRUE;
        }

        else if (!(strcmp(argv[i],"-help")) || !(strcmp(argv[i],"h")))
            usage(0);

        else if (argv[i][0] == '-') {
            k = 1;
            while (argv[i][k] != '\0') {
                switch(argv[i][k]) {
                    case 'p': POW2CONSTR = TRUE; break;
                    case 'r': COMPLEMENT = TRUE; break;
                    case 't': PLA_OUTPUT = TRUE; break;
                    case 'j': OUT_ALL = TRUE; break;
                    case 'y': OUT_ONLY = TRUE; break;
                    default : usage(1); break;
                }
                k++;
            }
        }
        else if (infile == 0) {
            strcpy(file_name,argv[i]);
	    getname(argv[i]);
            infile++;
        }
        else usage(1);
    }
    /*if (infile == 0) usage();*/
    fprintf(stdout,"#");
    for (i = 0; i < argc; i++) {
        fprintf(stdout," %s",argv[i]);
    }
    fprintf(stdout,"\n");
    return(infile);
}/* end of command_line_proc */



void
usage(exit_flag)
int exit_flag;
{
    fprintf(stderr,
    "\nNOVA, Version 3.2, Date: january 1990\n");
    fprintf(stderr, "nova [options] [infile] [> outfile]\n\n");
    fprintf(stderr, "infile\tspecify the input file.\n");
    fprintf(stderr, "outfile\tspecify the output file.\n\n");
    fprintf(stderr,"Please read the manual page to understand the options\n\n");
    fprintf(stderr, "Options allowed are:\n");
    fprintf(stderr, "-h\tDisplay the available command options.\n");
    fprintf(stderr, "-e ig\tencoding-algo. inp.constr.+ greedy (default)\n");
    fprintf(stderr, "-e ih\tencoding-algo. inp.constr.+ hybrid\n");
    fprintf(stderr, "-e ioh\tencoding-algo. inp./out.constr.+ hybrid\n");
    fprintf(stderr, "-e iov\tencoding-algo. inp./out.constr.+ hybrid\n");
    fprintf(stderr, "-e ie\tencoding-algo. inp.constr.+ exact\n");
    fprintf(stderr, "-e ia\tencoding-algo. inp.constr.+ sim.annealing\n");
    fprintf(stderr, "-e h\tencoding-algo. onehot\n");
    fprintf(stderr, "-e r\tencoding-algo. random\n");
    fprintf(stderr, "-e u\tencoding-algo. user\n");
    fprintf(stderr, "-i\tspecify code-length of inputs\n");
    fprintf(stderr, "-s\tspecify code-length of states\n");
    fprintf(stderr, "-o\tspecify code-length of outputs\n");
    fprintf(stderr, "-n\tspecify number of random encodings\n");
    fprintf(stderr, "-c\tcost function of sim-anneal: 0=faces, 1=literals\n");
    fprintf(stderr, "-m\tspecify number of moves of sim-anneal\n");
    fprintf(stderr, "-p\t-e ig on power-of-2 input constraints\n");
    fprintf(stderr, "-r\ttry all rotations\n");
    fprintf(stderr, "-t\toutput two-level minimized fsm\n");
    fprintf(stderr, "-j\t-e ioh on all output constraints\n");
    fprintf(stderr, "-y\t-e ioh on only output constraints\n");
    fprintf(stderr, "-z\tspecify state to be assigned all zeroes code\n");
    fprintf(stderr, "\n");
    if (exit_flag == 0) exit(0); 
        else exit(-1);
} /* end of usage() */
