
/* SCCSID%W% */
#include "sis/util/util.h"
#include "struct.h"
#include "user.h"

STATE **states;        /* global variable: array of pointers to states */
EDGE **edges;          /* global variable: array of pointers to edges  */
int num_pi;            /* number of primary inputs */
int num_po;            /* number of primary outputs*/
int num_product;       /* number of product terms  */
int num_st;            /* number of states         */
char FSM_fileName[50]; /* the input state machine file name */
char b_file[50];
struct u user;
long t_start;

static usage(char *);

main(argc, argv) char **argv;
{
  FILE *fp_input; /* open file for the state transition table */
  int step, clique;
  int c;

  int (**do_work)();

  int say_solution();

  extern int (*method1[])();

  /* initialize some parameters for command line options */

  FSM_fileName[0] = '\0';
  user.level = 8;

  /* parse command line arguments */

  b_file[0] = 0;
  user.oname = NIL(char);
  user.opt.hmap = 4;
  user.opt.solution = 0;
  user.opt.verbose = 0;
  user.cmd.merge = 0;
  user.cmd.shrink = 0;
  user.cmd.trans = 0;

  while ((c = util_getopt(argc, argv, "tcrhSRCMPs:m:b:v:o:")) != EOF) {
    switch (c) {
    case 's':
      if ((user.opt.solution = atoi(util_optarg)) > 3)
        user.opt.solution = 1;
      break;
    case 't':
      user.cmd.trans = 1;
      break;
    case 'h':
      usage(argv[0]);
      break;
    case 'o':
      if (!(user.oname = ALLOC(char, strlen(util_optarg) + 1)))
        panic("main");
      (void)strcpy(user.oname, util_optarg /*,strlen(util_optarg)*/);
      break;
    case 'r':
      user.cmd.merge = 1;
      break;
    case 'S':
      user.cmd.shrink = 1;
      break;
    case 'm':
      if ((user.opt.hmap = atoi(util_optarg)) > 4)
        user.opt.hmap = 0;
      break;
    case 'b':
      (void)strcpy(b_file, util_optarg);
      break;
    case 'v':
      user.opt.verbose = atoi(util_optarg);
      break;
    case 'R':
      user.level = 7;
      method1[8] = say_solution;
      break;
    case 'M':
      user.level = 5;
      method1[5] = (int (*)())0;
      break;
    case 'P':
      user.level = 6;
      method1[6] = (int (*)())0;
      break;
    case 'C':
      method1[1] = (int (*)())0;
      break;
    default:
      usage(argv[0]);
      break;
    }
  }

  if (argc - util_optind == 0) {
    fp_input = stdin;
  } else if (argc - util_optind == 1) {
    (void)strcpy(FSM_fileName, argv[util_optind]);
    user.fname = FSM_fileName;
    /***
     *** read the FSM state transition table and build the flow table
     ***/
    if ((fp_input = fopen(FSM_fileName, "r")) == NULL) {
      (void)fprintf(stderr, "XCould not open %s \n", FSM_fileName);
      exit(1);
    }
  } else {
    usage(argv[0]);
  }

  read_fsm(fp_input);

  t_start = util_cpu_time();

  step = 0;

  for (do_work = method1; *do_work; do_work++) {
    user.ltime[step++] =

        util_cpu_time();

    (**do_work)();
  }
  return 0;
}

#ifdef DEBUG
dump_flow_table() {
  register i;
  EDGE *next_edge;

  (void)printf("***** FLOW TABLE *****\n");
  for (i = 0; i < num_st; i++) {
    (void)printf("%d %s ", i, states[i]->state_name);
    next_edge = states[i]->edge;
    while (next_edge != NIL(EDGE)) {
      if (next_edge->n_star)
        (void)printf("Input %s Output %s Next State *\n", next_edge->input,
                     next_edge->output);
      else
        (void)printf("Input %s Output %s Next State %s\n", next_edge->input,
                     next_edge->output, next_edge->n_state->state_name);
      next_edge = next_edge->next;
    }
  }
}
#endif

static usage(prog) char *prog;
{
  /*
      (void) fprintf(stderr, "   -CMPR\tCompatible,Maximal,Prime,Reduce\n");
      (void) fprintf(stderr, "   -b bfile\tsave binate covering matrix\n");
  */
  (void)fprintf(stderr, "Usage: %s [options] [file]\n", prog);
  (void)fprintf(stderr, "   -s n\t\tminimization heuristics(1,2,3)\n");
  (void)fprintf(stderr, "   -m n\t\tmapping heuristics(1,2,3,4)\n");
  (void)fprintf(stderr, "   -v n\t\tset verbose level to 'n' (e.g., 5)\n");
  (void)fprintf(stderr, "   -o filename\tset output file name\n");
  (void)fprintf(stderr, "   -h\t\tprint this message\n");
  exit(0);
}

panic(msg) char *msg;
{
  (void)fprintf(stderr, "Panic: %s in %s\n", msg, FSM_fileName);
  exit(1);
}
