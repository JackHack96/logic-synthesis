
/* file @(#)com_genlib.c	1.1 */
/* last modified on 7/2/91 at 01:34:28 */
#include "sis.h"
#include "genlib_int.h"

static int
com_genlib_print(network, argc, argv)
network_t **network;
int argc;
char **argv;
{
  int c, use_nand;
  char *infile, *real_infile;
  char *outfile, *real_outfile;
  FILE *fp, *outfp;

  use_nand = 0; /* default is nor */  
  outfile = NIL(char);
  
  util_getopt_reset();
  while ((c = util_getopt(argc, argv, "do:")) != EOF) {
    switch (c) {
    case 'd':
      use_nand = 1;
      break;
    case 'o':
      outfile = util_optarg;
      break;
    default:
      goto usage;
    }
  }
  if (argc - util_optind != 1) goto usage;

  infile = argv[util_optind];
  fp = com_open_file(infile, "r", &real_infile, 0);
  if (fp == NULL) goto error_return;

  if (outfile == NIL(char)) {
    outfp = sisout;
  } else {
    outfp = com_open_file(outfile, "w", &real_outfile, 0);
    if (outfp == NULL) {
      FREE(real_outfile);
      goto error_return;
    }
  }
  if (! genlib_parse_library(fp, real_infile, outfp, ! use_nand)) {
    (void)fprintf(siserr, "%s", error_string());
    (void)fclose(fp);
    goto error_return;
  }

  FREE(real_infile);
  if (outfp != sisout) {
    FREE(real_outfile);
    (void) fclose(outfp);
  }
  return 0;

 usage:
  (void) fprintf(siserr, "_genlib_print [-d] [-o outfile] lib.genlib\n");
  (void) fprintf(siserr, "\t-d : use nand gates\n");
  (void) fprintf(siserr, "\t-o outfile : outputs to outfile instead of sisout\n");
  return 1;

 error_return:
  FREE(real_infile);
  return 1;
}

init_genlib()
{
  com_add_command("_genlib_print", com_genlib_print, /* changes_network */ 0);
}

end_genlib()
{
}


