
/*
 * Symbolic encoding program for compiling a symbolic
 * description into a binary representation.  Target
 * is multi-level logic synthesis
 *
 * History:
 *
 * Bill Lin
 * University of California, Berkeley
 * Comments to billlin@ic.Berkeley.EDU
 *
 * Copyright (c) 1989 Bill Lin, UC Berkeley CAD Group
 *     Permission is granted to do anything with this
 *     code except sell it or remove this message.
 */

#include "inc/jedi.h"
#include <stdio.h>

int parse_options(); /* forward declaration */

extern FILE *infp;  /* jedi.c */
extern FILE *outfp; /* jedi.c */

usage() {
  (void)fprintf(stderr, "usage: jedi [options] [input] \n");
  (void)fprintf(stderr, "   -%s\t\t%s\n", "h", "print help message");
  (void)fprintf(stderr, "   -%s %s\t%s\n", "e", "option",
                "specify encoding option");
  (void)fprintf(stderr, "\t\t%s\n", "   r:  random encoding");

  (void)fprintf(stderr, "\t\t%s\n", "   h:  one hot encoding");

  (void)fprintf(stderr, "\t\t%s\n", "   d:  dynamic random encoding");
  (void)fprintf(stderr, "\t\t%s\n", "   s:  straightforward mapping");
  (void)fprintf(stderr, "\t\t%s\n", "   i:  input dominant algorithm");
  (void)fprintf(stderr, "\t\t%s\n",
                "   o:  output dominant algorithm (default)");
  (void)fprintf(stderr, "\t\t%s\n", "   c:  coupled dominant algorithm");
  (void)fprintf(stderr, "\t\t%s\n",
                "   y:  modified output dominant algorithm");
  (void)fprintf(stderr, "   -%s %s\t%s\n", "s", "length",
                "code length for state assignment");
  (void)fprintf(stderr, "   -%s\t\t%s\n", "x", "expand state code");
  (void)fprintf(stderr, "   -%s\t\t%s\n", "c",
                "use group based embedding algorithm");
  (void)fprintf(stderr, "   -%s\t\t%s\n", "p", "output in PLA format");
  (void)fprintf(stderr, "   -%s\t\t%s\n", "g",
                "general symbolic encoding format");
  (void)fprintf(stderr, "\n");
  (void)fprintf(stderr, "   [%s]\t%s\n", "input",
                "file to be processed (default stdin)");
  (void)fprintf(stderr, "\n");
  (void)fprintf(stderr, "%s\n", HEADER);
  (void)fprintf(stderr, "\n");
  (void)exit(-1);
}

parse_options(argc, argv) int argc;
char **argv;
{
  extern int optind;
  extern char *optarg;
  int c;

  /*
   * set defaults
   */
  beginningStates = 1;
  endingStates = 10;
  startingTemperature = 1000;
  maximumTemperature = INFINITY;

  bitsFlag = FALSE;
  addDontCareFlag = TRUE;
  kissFlag = TRUE;
  verboseFlag = FALSE;
  sequentialFlag = FALSE;
  clusterFlag = FALSE;
  srandomFlag = FALSE;
  drandomFlag = FALSE;
  variationFlag = TRUE;
  oneplaneFlag = FALSE;
  hotFlag = FALSE;
  expandFlag = FALSE;
  plaFlag = FALSE;

  weightType = OUTPUT;

  /*
   * parse options
   */
  while ((c = getopt(argc, argv, "hxcpge:s:")) != EOF) {
    switch (c) {
    case 'h':

      usage();

      break;
    case 'e':
      if (!strcmp(optarg, "s")) {
        sequentialFlag = TRUE;
        break;
      } else if (!strcmp(optarg, "r")) {
        srandomFlag = TRUE;
        break;
      } else if (!strcmp(optarg, "d")) {
        drandomFlag = TRUE;
        break;
      } else if (!strcmp(optarg, "h")) {
        hotFlag = TRUE;
        break;
      } else if (!strcmp(optarg, "i")) {
        weightType = INPUT;
        variationFlag = FALSE;
        break;
      } else if (!strcmp(optarg, "o")) {
        weightType = OUTPUT;
        variationFlag = TRUE;
        break;
      } else if (!strcmp(optarg, "c")) {
        weightType = COUPLED;
        variationFlag = TRUE;
        break;
      } else if (!strcmp(optarg, "y")) {
        weightType = OUTPUT;
        variationFlag = FALSE;
        break;
      } else {
        usage();

        break;
      }
    case 's':
      bitsFlag = TRUE;
      code_length = atoi(optarg);
      break;
    case 'x':
      expandFlag = TRUE;
      break;
    case 'c':
      clusterFlag = TRUE;
      break;
    case 'p':
      plaFlag = TRUE;
      break;
    case 'g':
      kissFlag = FALSE;
      break;
    default:

      usage();
    }
  }

  /*
   * check for sufficient arguments
   */
  if (optind != argc && optind != argc - 1) {
    usage();
  }

  if (optind == argc) {
    infp = stdin;
  } else {
    infp = my_fopen(argv[argc - 1], "r");
  }
  outfp = stdout;
} /* end of parse_options */
